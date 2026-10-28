#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <stddef.h>

int file_exists(const char *path);
int delete_file_if_exists(const char *path);
int copy_file(const char *source_path, const char *destination_path);
int build_encrypted_output_path(const char *source_path, char *output_path,
                                size_t output_size);
int build_decrypted_output_path(const char *source_path, char *output_path,
                                size_t output_size);

#endif
