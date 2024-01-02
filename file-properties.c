// File includes
#include <file-properties.h>
#include <dirent.h>
#include <fcntl.h>
#include <utility.h>
#include <defines.h>

// Librabry includes
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
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
    struct stat fileStat;
    if(stat(entry->path_and_name, &fileStat) < 0)    
        return -1;

    entry->mode = fileStat.st_mode;
    entry->mtime = fileStat.st_mtim;

    if(S_ISDIR(fileStat.st_mode))
        entry->entry_type = DOSSIER;
    else {
        entry->entry_type = FICHIER;
        entry->size = fileStat.st_size;

        if(compute_file_md5(entry) != 0)
            return -1;
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
    FILE *inFile = fopen(entry->path_and_name, "rb");
    EVP_MD_CTX *mdctx;
    unsigned char data[1024];
    int bytes;
    unsigned int md_len;

    if (inFile == NULL) {
        printf("%s can't be opened.\n", entry->path_and_name);
        return -1;
    }

    if((mdctx = EVP_MD_CTX_new()) == NULL)
        return -1;

    if(1 != EVP_DigestInit_ex(mdctx, EVP_md5(), NULL)) {
        EVP_MD_CTX_free(mdctx);
        return -1;
    }

    while ((bytes = fread(data, 1, 1024, inFile)) != 0) {
        if(1 != EVP_DigestUpdate(mdctx, data, bytes)) {
            EVP_MD_CTX_free(mdctx);
            return -1;
        }
    }

    if(1 != EVP_DigestFinal_ex(mdctx, entry->md5sum, &md_len)) {
        EVP_MD_CTX_free(mdctx);
        return -1;
    }

    EVP_MD_CTX_free(mdctx);
    fclose(inFile);
    return 0;
}
/*!
 * @brief directory_exists tests the existence of a directory
 * @path_to_dir a string with the path to the directory
 * @return true if directory exists, false else
 */
bool directory_exists(char *path_to_dir) {
    if (path_to_dir == NULL) {
        return false;
    }
    
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
    if (path_to_dir == NULL) {
        return false;
    }
    
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
