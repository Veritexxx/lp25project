#include <file-properties.h>


#include <dirent.h>
#include <openssl/evp.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <defines.h>
#include <fcntl.h>

#include <utility.h>
#include <stdbool.h>
// compute md5
#include <sys/stat.h>
#include <sys/types.h>
#include <openssl/md5.h>
#include <stdio.h>



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
    MD5_CTX mdContext;
    int bytes;
    unsigned char data[1024];

    if (inFile == NULL) {
        printf("%s can't be opened.\n", entry->path_and_name);
        return -1;
    }

    MD5_Init(&mdContext);
    while ((bytes = fread(data, 1, 1024, inFile)) != 0)
        MD5_Update(&mdContext, data, bytes);
    MD5_Final(entry->md5sum, &mdContext);

    fclose(inFile);
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
