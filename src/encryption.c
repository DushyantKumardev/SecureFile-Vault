/**
 * @file encryption.c
 * @brief Cryptographic streaming transformation engine.
 *
 * Implements a symmetric position-dependent stream cipher using an offset-keyed
 * FNV-1a pseudo-random keystream generator. Supports in-place file transformation
 * with automatic backup creation and transactional rollback on failure.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "encryption.h"
#include "file_manager.h"
#include "storage.h"

/**
 * 8KB streaming buffer size.
 * Balances memory footprint within CPU L1 cache limits while maximizing
 * sequential read/write throughput over standard filesystem cluster boundaries.
 */
#define CHUNK_SIZE 8192U

/**
 * Generates a deterministic, position-dependent pseudo-random keystream block.
 *
 * Rationale:
 * - Uses 32-bit FNV-1a (Fowler-Noll-Vo) parameters:
 *   - Offset basis: 2166136261u
 *   - Prime multiplier: 16777619u
 * - First pass hashes the entire password into an initial seed state.
 * - Second pass mixes the absolute stream byte offset (offset + index + 1)
 *   and cycling password characters into each byte calculation.
 * - Absolute offset mixing guarantees that identical plaintext blocks at different
 *   file positions yield distinct ciphertext outputs (mitigating pattern analysis).
 * - Extracts `(state >> 16) & 0xFFu` (middle 8 bits) because the middle bits of
 *   multiplicative state engines exhibit greater entropy and cycle length than lower bits.
 *
 * @param password Secret key used for seeding.
 * @param block_size Number of keystream bytes to generate for the current chunk.
 * @param offset Absolute byte offset of this chunk from the start of the file.
 * @param stream Output buffer populated with the generated keystream bytes.
 */
static void generate_stream(const char *password, size_t block_size,
                            size_t offset, unsigned char *stream)
{
    uint32_t state = 2166136261u;
    size_t index;
    size_t password_length = strlen(password);

    if (password_length == 0)
    {
        memset(stream, 0, block_size);
        return;
    }

    /* Seed the generator state with the full password string. */
    for (index = 0; index < password_length; ++index)
    {
        state ^= (unsigned char)password[index];
        state *= 16777619u;
    }

    /* Generate position-dependent keystream bytes for the current block. */
    for (index = 0; index < block_size; ++index)
    {
        state ^= (uint32_t)(offset + index + 1u);
        state ^= (uint32_t)password[(offset + index) % password_length];
        state *= 16777619u;

        /* Extract middle bits for superior statistical randomness. */
        stream[index] = (unsigned char)((state >> 16) & 0xFFu);
    }
}

/**
 * Transforms a file in place using symmetric XOR with the generated keystream.
 *
 * Rationale for in-place transformation:
 * - Modifying the file directly avoids doubling disk space requirements,
 *   which is essential when processing multi-gigabyte media or archive files.
 * - XOR is strictly symmetric: `(Data ^ Key) ^ Key == Data`. Thus, this
 *   identical function performs both encryption and decryption.
 *
 * @param file_path Path of the file to transform.
 * @param password Secret key for keystream derivation.
 * @return 1 on complete success, 0 on I/O failure or permission error.
 */
static int transform_in_place(const char *file_path, const char *password)
{
    FILE *file = NULL;
    unsigned char buffer[CHUNK_SIZE];
    unsigned char stream[CHUNK_SIZE];
    size_t bytes_read;
    size_t offset = 0;

    if (file_path == NULL || password == NULL || password[0] == '\0')
    {
        return 0;
    }

    /* "r+b" enables read and write access on binary files without truncation. */
    file = fopen(file_path, "r+b");
    if (file == NULL)
    {
        return 0;
    }

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        generate_stream(password, bytes_read, offset, stream);

        for (size_t index = 0; index < bytes_read; ++index)
        {
            buffer[index] ^= stream[index];
        }

        /*
         * In-place overwrite sequence:
         * 1. Seek backwards by the chunk size just read to align with the chunk start.
         * 2. Write the ciphered bytes over the existing file contents.
         * 3. Flush buffered bytes to the underlying OS cache.
         * 4. Call fseek(file, 0, SEEK_CUR) per ISO C standard (§7.21.5.3), which requires
         *    an intervening file-positioning call between write and subsequent read operations
         *    on update streams.
         */
        if (fseek(file, -(long)bytes_read, SEEK_CUR) != 0 ||
            fwrite(buffer, 1, bytes_read, file) != bytes_read ||
            fflush(file) != 0 ||
            fseek(file, 0, SEEK_CUR) != 0)
        {
            fclose(file);
            return 0;
        }

        offset += bytes_read;
    }

    fclose(file);
    return 1;
}

/**
 * Encrypts a target file in place with automatic safety backup and transactional rollback.
 *
 * Rationale:
 * - Because in-place transformation modifies the source directly, unexpected
 *   aborts (power loss, OS crash) could corrupt the file.
 * - To guarantee safety, an isolated backup is created in the vault backup directory
 *   before transformation starts. If transformation fails, the backup is restored.
 *
 * @param source_path Absolute path to the file to encrypt.
 * @param password Secret encryption key.
 * @param output_path Buffer populated with the final file path.
 * @param output_size Capacity of output_path.
 * @return 1 on successful encryption, 0 on failure (with original file restored).
 */
int encrypt_file(const char *source_path, const char *password, char *output_path, size_t output_size)
{
    char backup_path[4096];
    char log_message[8192];

    if (source_path == NULL || password == NULL || output_path == NULL || output_size == 0)
    {
        return 0;
    }

    if (!build_backup_file_path(source_path, backup_path, sizeof(backup_path)))
    {
        return 0;
    }

    /* Create safety backup prior to initiating destructive in-place write. */
    if (!copy_file(source_path, backup_path))
    {
        return 0;
    }

    /* Transform file in place; on error, restore from backup to prevent file loss. */
    if (!transform_in_place(source_path, password))
    {
        copy_file(backup_path, source_path);
        snprintf(log_message, sizeof(log_message), "Encryption failed for %s", source_path);
        write_operation_log(log_message);
        return 0;
    }

    snprintf(output_path, output_size, "%s", source_path);
    snprintf(log_message, sizeof(log_message), "Encrypted %s and stored a backup at %s", source_path, backup_path);
    write_operation_log(log_message);
    return 1;
}

/**
 * Decrypts a target file in place with automatic safety backup and transactional rollback.
 *
 * Upon successful decryption, temporary safety backups are automatically purged
 * to prevent plaintext data from lingering in the backups directory.
 *
 * @param source_path Absolute path to the file to decrypt.
 * @param password Secret decryption key.
 * @param output_path Buffer populated with the final file path.
 * @param output_size Capacity of output_path.
 * @return 1 on successful decryption, 0 on failure (with original file restored).
 */
int decrypt_file(const char *source_path, const char *password, char *output_path, size_t output_size)
{
    char backup_path[4096];
    char log_message[8192];

    if (source_path == NULL || password == NULL || output_path == NULL || output_size == 0)
    {
        return 0;
    }

    if (!build_backup_file_path(source_path, backup_path, sizeof(backup_path)))
    {
        return 0;
    }

    /* Create temporary safety backup before running in-place decryption. */
    if (!copy_file(source_path, backup_path))
    {
        return 0;
    }

    /* Transform file in place; restore from backup if stream processing fails. */
    if (!transform_in_place(source_path, password))
    {
        copy_file(backup_path, source_path);
        snprintf(log_message, sizeof(log_message), "Decryption failed for %s", source_path);
        write_operation_log(log_message);
        return 0;
    }

    /* Decryption succeeded: remove temporary safety backup files to preserve privacy. */
    remove_backup_files(source_path);

    snprintf(output_path, output_size, "%s", source_path);
    snprintf(log_message, sizeof(log_message), "Decrypted %s and removed temporary backup", source_path);
    write_operation_log(log_message);
    return 1;
}
