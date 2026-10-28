#ifndef STORAGE_H
#define STORAGE_H

#include <stddef.h>

int ensure_log_directory(void);
int get_log_file_path(char *path, size_t path_size);
int write_operation_log(const char *message);
int get_local_backup_directory(char *directory, size_t directory_size);
int build_backup_file_path(const char *source_path, char *backup_path,
                          size_t backup_path_size);
int remove_backup_files(const char *source_path);

#endif