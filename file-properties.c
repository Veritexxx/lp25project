#include <file-properties.h>

#include <sys/stat.h>
#include <dirent.h>
#include <openssl/evp.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <defines.h>
#include <fcntl.h>
#include <stdio.h>
#include <utility.h>
#include <stdbool.h>

/*!
 * @brief get_file_stats gets all of the required information for a file (inc. directories)
 * @param the files list entry
 * You must get:
 * - for files:
 *   - mode (permissions)
 *   - mtime (in nanoseconds)
 *   - size
 *   - entry type (FICHIER)
 *   - MD5 sum
 * - for directories:
 *   - mode
 *   - entry type (DOSSIER)
 * @return -1 in case of error, 0 else
 */
int get_file_stats(files_list_entry_t *entry) {
    struct stat file_info;
    if (stat(entry->path, &file_info) == -1) {
        perror("Error: stat");
        return -1;
    }
    entry->mode = file_info.st_mode;
    entry->size = file_info.st_size;
    entry->mtime = file_info.st_mtime;
    if (S_ISDIR(file_info.st_mode)) {
        entry->type = DOSSIER;
    } else {
        entry->type = FICHIER;
    }
    if (entry->type == FICHIER) {
        if (compute_file_md5(entry) == -1) {
            return -1;
        }
    }
    return 0;
}

/*!
 * @brief compute_file_md5 computes a file's MD5 sum
 * @param the pointer to the files list entry
 * @return -1 in case of error, 0 else
 * Use libcrypto functions from openssl/evp.h
 */
int compute_file_md5(files_list_entry_t *entry) {
    unsigned char md5[MD5_DIGEST_LENGTH];
    FILE *file = fopen(entry->path, "rb");
    if (file == NULL) {
        perror("Error: fopen");
        return -1;
    }
    MD5_CTX md5_context;
    MD5_Init(&md5_context);
    unsigned char data[1024];
    int bytes;
    while ((bytes = fread(data, 1, 1024, file)) != 0) {
        MD5_Update(&md5_context, data, bytes);
    }
    MD5_Final(md5, &md5_context);
    fclose(file);
    char md5_string[MD5_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(md5_string + i * 2, "%02x", md5[i]);
    }
    strcpy(entry->md5sum, md5_string);
    return 0;
}

/*!
 * @brief directory_exists tests the existence of a directory
 * @path_to_dir a string with the path to the directory
 * @return true if directory exists, false else
 */
bool directory_exists(char *path_to_dir) {
    DIR *dir = opendir(path_to_dir);

    if (dir != NULL) {
      closedir(dir);
      return true;
    } else {
      return false;
    }
}

/*!
 * @brief is_directory_writable tests if a directory is writable
 * @param path_to_dir the path to the directory to test
 * @return true if dir is writable, false else
 * Hint: try to open a file in write mode in the target directory.
 */
bool is_directory_writable(char *path_to_dir) {
    const char *temp_file_name = ".test_writable_file";
    char temp_file_path[256];

    snprintf(temp_file_path, sizeof(temp_file_path), "%s/%s", path_to_dir, temp_file_name);

    FILE *temp_file = fopen(temp_file_path, "w");

    if (temp_file != NULL) {
        fclose(temp_file);
        remove(temp_file_path);
        return true;
    } else {
        return false;
    }
}
