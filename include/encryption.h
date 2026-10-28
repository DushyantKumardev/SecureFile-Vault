#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <stddef.h>

int encrypt_file(const char *source_path, const char *password,
                 char *output_path, size_t output_size);
int decrypt_file(const char *source_path, const char *password,
                 char *output_path, size_t output_size);

#endif
