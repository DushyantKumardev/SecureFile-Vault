/**
 * @file storage.c
 * @brief Application data persistence, backup storage, and audit logging.
 *
 * Manages Windows AppData environment path resolution, recursive directory
 * creation, isolated backup storage for safety rollbacks, and atomic activity logging.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#include "file_manager.h"
#include "storage.h"

/* ============================================================================
 * PATH CONFIGURATION CONSTANTS
 * ============================================================================
 * Rationale for %APPDATA%\SecureFileVault:
 * - Windows standard guidelines mandate storing application logs and user backups
 *   in the user's Roaming AppData folder.
 * - Unlike Program Files, %APPDATA% does not require administrative UAC elevation
 *   and ensures normal user accounts can write logs and backups without permission errors.
 * - Storing backups in AppData keeps user working directories clutter-free and avoids
 *   exposing temporary backup files on the user's desktop or shared drives.
 * ============================================================================ */
#define VAULT_BASE_DIR        "%APPDATA%\\SecureFileVault"
#define VAULT_LOG_DIR         "%APPDATA%\\SecureFileVault\\logs"
#define VAULT_LOG_FILE        "%APPDATA%\\SecureFileVault\\logs\\activity.log"
#define VAULT_BACKUP_DIR      "%APPDATA%\\SecureFileVault\\backups"

/**
 * Resolves a configured path string into an absolute, normalized Windows filesystem path.
 *
 * Rationale:
 * - Expands Windows environment variables (e.g., %APPDATA%, %LOCALAPPDATA%) dynamically
 *   to support roaming profiles, custom drive mappings, and localized Windows installations.
 * - Identifies drive-qualified ("C:\") and UNC ("\\server\share") paths directly to prevent
 *   unnecessary concatenation with the current working directory.
 * - Uses GetFullPathNameA to resolve relative paths and eliminate redundant "." or ".." segments.
 *
 * @param configured_path Raw path string containing optional environment variables or relative segments.
 * @param resolved_path Output buffer to store the normalized absolute path.
 * @param resolved_size Capacity of resolved_path buffer.
 * @return 1 on successful resolution, 0 on invalid arguments or buffer overflow.
 */
static int resolve_configured_path(const char *configured_path, char *resolved_path, size_t resolved_size)
{
    char expanded[MAX_PATH];
    char raw_resolved[MAX_PATH];
    char base_dir[MAX_PATH] = "";

    if (configured_path == NULL || resolved_path == NULL || resolved_size == 0)
    {
        return 0;
    }

    /* Expand environment variables like %APPDATA%, %LOCALAPPDATA%, %USERPROFILE% dynamically. */
    if (strchr(configured_path, '%') != NULL)
    {
        if (ExpandEnvironmentStringsA(configured_path, expanded, sizeof(expanded)) > 0)
        {
            if (GetFullPathNameA(expanded, (DWORD)resolved_size, resolved_path, NULL) > 0)
            {
                return 1;
            }
            snprintf(resolved_path, resolved_size, "%s", expanded);
            return 1;
        }
    }

    /* Check if configured_path is already absolute (drive letter or UNC network path). */
    if ((configured_path[0] != '\0' && configured_path[1] == ':') ||
        (configured_path[0] == '\\' && configured_path[1] == '\\'))
    {
        snprintf(resolved_path, resolved_size, "%s", configured_path);
        return 1;
    }

    /* Resolve relative paths against the active process working directory. */
    if (GetCurrentDirectoryA(sizeof(base_dir), base_dir) > 0)
    {
        snprintf(raw_resolved, sizeof(raw_resolved), "%s\\%s", base_dir, configured_path);
        if (GetFullPathNameA(raw_resolved, (DWORD)resolved_size, resolved_path, NULL) > 0)
        {
            return 1;
        }
        snprintf(resolved_path, resolved_size, "%s", raw_resolved);
        return 1;
    }

    snprintf(resolved_path, resolved_size, "%s", configured_path);
    return 1;
}

/**
 * Recursively creates directories along the specified path if they do not already exist.
 *
 * Rationale:
 * - Windows CreateDirectoryA is non-recursive; it fails with ERROR_PATH_NOT_FOUND
 *   if any intermediate parent folder does not exist.
 * - Walking component-by-component and calling CreateDirectoryA incrementally guarantees
 *   deep paths like "C:\Users\...\AppData\Roaming\SecureFileVault\backups" are established safely.
 * - Skips drive letters ("C:\") and UNC prefixes ("\\server\share") to avoid calling
 *   CreateDirectory on drive roots or network server names, which Windows rejects.
 *
 * @param dir_path Directory hierarchy path to create.
 * @return 1 if directory exists or was created, 0 on creation error.
 */
static int ensure_directory_exists(const char *dir_path)
{
    char temp[MAX_PATH];
    char *p = NULL;
    size_t len;

    if (dir_path == NULL || dir_path[0] == '\0')
    {
        return 0;
    }

    snprintf(temp, sizeof(temp), "%s", dir_path);
    len = strlen(temp);
    if (len > 0 && (temp[len - 1] == '\\' || temp[len - 1] == '/'))
    {
        temp[len - 1] = '\0';
    }

    /* Skip drive letter (e.g. "C:\") or UNC network prefix (e.g. "\\server\share\"). */
    p = temp;
    if (temp[0] != '\0' && temp[1] == ':' && (temp[2] == '\\' || temp[2] == '/'))
    {
        p = temp + 3;
    }
    else if (temp[0] == '\\' && temp[1] == '\\')
    {
        p = strchr(temp + 2, '\\');
        if (p != NULL)
        {
            p = strchr(p + 1, '\\');
            if (p != NULL)
            {
                p++;
            }
        }
    }

    if (p == NULL)
    {
        p = temp;
    }

    /* Create each subdirectory level progressively. */
    for (; *p != '\0'; ++p)
    {
        if (*p == '\\' || *p == '/')
        {
            char delimiter = *p;
            *p = '\0';
            if (strlen(temp) > 0 && GetFileAttributesA(temp) == INVALID_FILE_ATTRIBUTES)
            {
                if (!CreateDirectoryA(temp, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
                {
                    *p = delimiter;
                    return 0;
                }
            }
            *p = delimiter;
        }
    }

    /* Create the final leaf directory. */
    if (GetFileAttributesA(temp) == INVALID_FILE_ATTRIBUTES)
    {
        if (!CreateDirectoryA(temp, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
        {
            return 0;
        }
    }

    return 1;
}

/**
 * Ensures the activity log directory exists on disk.
 *
 * @return 1 on success, 0 on failure.
 */
int ensure_log_directory(void)
{
    char logs_directory[MAX_PATH];

    if (!resolve_configured_path(VAULT_LOG_DIR, logs_directory, sizeof(logs_directory)))
    {
        return 0;
    }

    return ensure_directory_exists(logs_directory);
}

/**
 * Resolves the absolute path to the application's activity audit log file.
 *
 * @param path Output buffer to receive the log path.
 * @param path_size Capacity of path buffer.
 * @return 1 on success, 0 on failure.
 */
int get_log_file_path(char *path, size_t path_size)
{
    if (path == NULL || path_size == 0)
    {
        return 0;
    }

    return resolve_configured_path(VAULT_LOG_FILE, path, path_size);
}

/**
 * Resolves and ensures the existence of the vault backup directory.
 *
 * @param directory Output buffer to store the backup directory path.
 * @param directory_size Capacity of directory buffer.
 * @return 1 on success, 0 on failure.
 */
int get_local_backup_directory(char *directory, size_t directory_size)
{
    char backups_directory[MAX_PATH];

    if (directory == NULL || directory_size == 0)
    {
        return 0;
    }

    if (!resolve_configured_path(VAULT_BACKUP_DIR, backups_directory, sizeof(backups_directory)))
    {
        return 0;
    }

    if (!ensure_directory_exists(backups_directory))
    {
        return 0;
    }

    snprintf(directory, directory_size, "%s", backups_directory);
    return 1;
}

/**
 * Constructs an isolated, non-colliding backup file path inside the vault backup directory.
 *
 * Rationale:
 * - Backups are stored in %APPDATA%\SecureFileVault\backups rather than next to the source
 *   file to keep user directories clean and prevent clutter or accidental deletion.
 * - Extracts only the filename component from source_path to construct a flat backup name.
 * - Iterates up to 999 numbered suffixes (.backup, .backup_1, ...) to avoid overwriting
 *   an existing safety backup from an earlier session.
 *
 * @param source_path Original file being processed.
 * @param backup_path Output buffer to receive the generated backup path.
 * @param backup_path_size Capacity of backup_path buffer.
 * @return 1 on success, 0 on failure.
 */
int build_backup_file_path(const char *source_path, char *backup_path,
                           size_t backup_path_size)
{
    const char *base_name;
    char backup_directory[MAX_PATH];
    char candidate[4096];
    int attempt = 0;

    if (source_path == NULL || backup_path == NULL || backup_path_size == 0)
    {
        return 0;
    }

    if (!get_local_backup_directory(backup_directory, sizeof(backup_directory)))
    {
        return 0;
    }

    /* Extract trailing filename component regardless of forward or backward slashes. */
    base_name = strrchr(source_path, '\\');
    if (base_name == NULL)
    {
        base_name = strrchr(source_path, '/');
    }
    base_name = (base_name == NULL) ? source_path : base_name + 1;

    snprintf(candidate, sizeof(candidate), "%s\\%s.backup", backup_directory, base_name);
    if (GetFileAttributesA(candidate) == INVALID_FILE_ATTRIBUTES)
    {
        snprintf(backup_path, backup_path_size, "%s", candidate);
        return 1;
    }

    /* Search for an available indexed candidate to prevent overwriting prior backups. */
    while (attempt < 999)
    {
        ++attempt;
        snprintf(candidate, sizeof(candidate), "%s\\%s.backup_%d", backup_directory, base_name, attempt);
        if (GetFileAttributesA(candidate) == INVALID_FILE_ATTRIBUTES)
        {
            snprintf(backup_path, backup_path_size, "%s", candidate);
            return 1;
        }
    }

    return 0;
}

/**
 * Deletes all backup files associated with a given source file name.
 *
 * Cleans up primary (.backup) and numbered (.backup_1, .backup_2, ...) files
 * once an operation has successfully concluded, preventing stale backup accumulation.
 *
 * @param source_path Original file whose backups should be removed.
 * @return 1 on success, 0 on invalid parameters or directory resolution failure.
 */
int remove_backup_files(const char *source_path)
{
    char backup_directory[MAX_PATH];
    char candidate[4096];
    const char *base_name;
    int attempt = 0;

    if (source_path == NULL)
    {
        return 0;
    }

    if (!get_local_backup_directory(backup_directory, sizeof(backup_directory)))
    {
        return 0;
    }

    base_name = strrchr(source_path, '\\');
    if (base_name == NULL)
    {
        base_name = strrchr(source_path, '/');
    }
    base_name = (base_name == NULL) ? source_path : base_name + 1;

    /* Delete primary backup */
    snprintf(candidate, sizeof(candidate), "%s\\%s.backup", backup_directory, base_name);
    delete_file_if_exists(candidate);

    /* Delete numbered backup variations sequentially until no further files exist. */
    for (attempt = 1; attempt < 999; ++attempt)
    {
        snprintf(candidate, sizeof(candidate), "%s\\%s.backup_%d", backup_directory, base_name, attempt);
        if (GetFileAttributesA(candidate) == INVALID_FILE_ATTRIBUTES)
        {
            break;
        }
        delete_file_if_exists(candidate);
    }

    return 1;
}

/**
 * Appends a timestamped entry to the persistent application audit log file.
 *
 * Rationale:
 * - Timestamps are formatted as YYYY-MM-DD HH:MM:SS (ISO-8601 extended) to provide
 *   unambiguous chronological audit history across regional locales.
 * - File is opened in "a" (append) mode to guarantee thread-safe, atomic tail writes
 *   without risking truncation of existing audit entries.
 *
 * @param message Informational log text describing the completed operation.
 * @return 1 on successful log write, 0 on file open or directory creation failure.
 */
int write_operation_log(const char *message)
{
    char log_path[MAX_PATH];
    FILE *log_file;
    time_t current_time;
    struct tm *local_time_info;
    char timestamp[64];

    if (message == NULL)
    {
        return 0;
    }

    if (!ensure_log_directory())
    {
        return 0;
    }

    if (!get_log_file_path(log_path, sizeof(log_path)))
    {
        return 0;
    }

    log_file = fopen(log_path, "a");
    if (log_file == NULL)
    {
        return 0;
    }

    current_time = time(NULL);
    local_time_info = localtime(&current_time);
    if (local_time_info != NULL)
    {
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", local_time_info);
    }
    else
    {
        snprintf(timestamp, sizeof(timestamp), "UNKNOWN");
    }

    fprintf(log_file, "[%s] %s\n", timestamp, message);
    fclose(log_file);
    return 1;
}