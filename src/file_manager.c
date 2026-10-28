/**
 * @file file_manager.c
 * @brief Filesystem utility functions and path management.
 *
 * Implements safe, collision-resistant file operations, stream copying,
 * and encrypted/decrypted file path generation for the vault engine.
 */

#include <stdio.h>
#include <string.h>

#include "file_manager.h"

/**
 * Checks whether the given string ends with the specified suffix.
 *
 * Suffix matching is case-sensitive to adhere to standard POSIX and
 * cryptographic naming conventions (e.g., lowercase ".enc").
 *
 * @param text The input string to inspect.
 * @param suffix The suffix string to check for.
 * @return 1 if text ends with suffix, 0 otherwise.
 */
static int ends_with(const char *text, const char *suffix)
{
    size_t text_length = strlen(text);
    size_t suffix_length = strlen(suffix);

    /* Guard against underflow in pointer arithmetic when suffix exceeds text length. */
    if (suffix_length > text_length)
    {
        return 0;
    }

    return strcmp(text + text_length - suffix_length, suffix) == 0;
}

/**
 * Generates an available file path by testing collisions and appending
 * sequential numeric suffixes (_1, _2, ...) if the primary target exists.
 *
 * This prevents accidental data loss or silent file overwrites when
 * multiple encryption or decryption passes occur in the same directory.
 *
 * @param base_path Destination base file path prefix.
 * @param suffix File extension suffix to append (e.g., ".enc").
 * @param output_path Buffer to receive the final collision-free path.
 * @param output_size Maximum capacity of output_path.
 * @return 1 on success, 0 if all attempts (up to 999) collided or parameters were invalid.
 */
static int create_unique_path_with_suffix(const char *base_path,
                                          const char *suffix,
                                          char *output_path,
                                          size_t output_size)
{
    /* 4096 bytes accommodates extended Windows paths and UTF-8 multi-byte sequences. */
    char candidate[4096];
    int attempt = 0;

    if (base_path == NULL || output_path == NULL || output_size == 0)
    {
        return 0;
    }

    /* Try the default unadorned path first before attempting numbered variations. */
    snprintf(candidate, sizeof(candidate), "%s%s", base_path, suffix);
    if (!file_exists(candidate))
    {
        snprintf(output_path, output_size, "%s", candidate);
        return 1;
    }

    /*
     * Iterate sequentially up to 999 to find an unused filename.
     * This bounded search guarantees termination while providing ample collision headroom.
     */
    while (attempt < 999)
    {
        ++attempt;
        snprintf(candidate, sizeof(candidate), "%s_%d%s", base_path, attempt, suffix);
        if (!file_exists(candidate))
        {
            snprintf(output_path, output_size, "%s", candidate);
            return 1;
        }
    }

    return 0;
}

/**
 * Determines if a file exists and is accessible for reading.
 *
 * Uses binary read mode ("rb") to avoid triggering OS-level file locking
 * or translation layers. The handle is closed immediately to release resources.
 *
 * @param path Path of the file to verify.
 * @return 1 if the file exists and can be opened, 0 otherwise.
 */
int file_exists(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL)
    {
        return 0;
    }
    fclose(file);
    return 1;
}

/**
 * Safely removes a file from disk if it currently exists.
 *
 * Returns 1 (success) if the file is already absent to ensure idempotent
 * cleanup behavior across retry loops and teardown sequences.
 *
 * @param path Path to the file to delete.
 * @return 1 on successful deletion or if absent, 0 on removal failure.
 */
int delete_file_if_exists(const char *path)
{
    if (path == NULL || path[0] == '\0')
    {
        return 0;
    }
    if (!file_exists(path))
    {
        return 1;
    }
    return remove(path) == 0;
}

/**
 * Copies a file byte-for-byte from source to destination.
 *
 * Uses binary I/O ("rb" and "wb") to prevent Windows C runtime from
 * translating LF (0x0A) into CRLF (0x0D 0x0A), which would corrupt binary payloads.
 *
 * @param source_path Path of existing file to copy.
 * @param destination_path Path of target file to create or overwrite.
 * @return 1 on complete copy, 0 on I/O error or missing file.
 */
int copy_file(const char *source_path, const char *destination_path)
{
    FILE *source_file = NULL;
    FILE *destination_file = NULL;

    /* 8KB buffer matches typical modern filesystem cluster sizes for optimal throughput. */
    unsigned char buffer[8192];
    size_t bytes_read;
    int success = 1;

    if (source_path == NULL || destination_path == NULL)
    {
        return 0;
    }

    source_file = fopen(source_path, "rb");
    if (source_file == NULL)
    {
        return 0;
    }

    destination_file = fopen(destination_path, "wb");
    if (destination_file == NULL)
    {
        fclose(source_file);
        return 0;
    }

    /* Stream chunks continuously until EOF or write failure. */
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), source_file)) > 0)
    {
        /* Detect disk full or permission failures immediately rather than silently truncating. */
        if (fwrite(buffer, 1, bytes_read, destination_file) != bytes_read)
        {
            success = 0;
            break;
        }
    }

    fclose(source_file);
    fclose(destination_file);
    return success;
}

/**
 * Constructs a unique, non-colliding destination path for an encrypted file.
 *
 * If the input file already carries a ".enc" extension, strips the extension
 * first before generating a unique candidate to prevent awkward filenames like "file.enc.enc".
 *
 * @param source_path Path to the source file.
 * @param output_path Buffer to receive the generated target path.
 * @param output_size Capacity of output_path.
 * @return 1 on success, 0 on invalid arguments or path truncation.
 */
int build_encrypted_output_path(const char *source_path, char *output_path, size_t output_size)
{
    char base_path[4096];
    size_t length;

    if (source_path == NULL || output_path == NULL || output_size == 0)
    {
        return 0;
    }

    if (ends_with(source_path, ".enc"))
    {
        length = strlen(source_path) - 4;
        if (length >= sizeof(base_path))
        {
            return 0;
        }
        memcpy(base_path, source_path, length);
        base_path[length] = '\0';
        return create_unique_path_with_suffix(base_path, ".enc", output_path, output_size);
    }

    return create_unique_path_with_suffix(source_path, ".enc", output_path, output_size);
}

/**
 * Constructs a unique, non-colliding destination path for a decrypted file.
 *
 * Strips the trailing ".enc" extension if present to restore the original filename.
 * If the file did not have a ".enc" suffix, appends ".decrypted" to prevent
 * overwriting the source file in place during standalone export operations.
 *
 * @param source_path Path to the encrypted file.
 * @param output_path Buffer to receive the generated target path.
 * @param output_size Capacity of output_path.
 * @return 1 on success, 0 on invalid arguments or path truncation.
 */
int build_decrypted_output_path(const char *source_path, char *output_path, size_t output_size)
{
    char base_path[4096];
    size_t length;

    if (source_path == NULL || output_path == NULL || output_size == 0)
    {
        return 0;
    }

    if (ends_with(source_path, ".enc"))
    {
        length = strlen(source_path) - 4;
        if (length >= sizeof(base_path))
        {
            return 0;
        }
        memcpy(base_path, source_path, length);
        base_path[length] = '\0';
        return create_unique_path_with_suffix(base_path, "", output_path, output_size);
    }

    return create_unique_path_with_suffix(source_path, ".decrypted", output_path, output_size);
}
