#include <sync.h>
#include <dirent.h>
#include <string.h>
#include <processes.h>
#include <utility.h>
#include <messages.h>
#include <file-properties.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <sys/msg.h>

#include <stdio.h>

/*!
 * @brief synchronize is the main function for synchronization
 * It will build the lists (source and destination), then make a third list with differences, and apply differences to the destination
 * It must adapt to the parallel or not operation of the program.
 * @param the_config is a pointer to the configuration
 * @param p_context is a pointer to the processes context
 */
void synchronize(configuration_t *the_config, process_context_t *p_context) {

  // création des listes de fichier
  files_list_t *liste_source, *liste_dest;
  make_files_list(source_list, the_config->source);
  make_files_list(dest_list, the_config->destination);

  // pointeur vers les entrées courantes
  files_list_entry_t *current_entry = liste_source->head;
  files_list_entry_t *current_dest;

  // parcourir toutes les entrées du repertoire source
  while (current_entry != NULL) {

    // trouver l'entrée correspondante dans le repertoire de destination et la copier si besoin
    files_list_entry_t *current_dest = find_entry_by_name(liste_dest, current_entry->path_and_name, 0 ,0);

    // si l'entrée n'est pas trouvé
    if (current_dest == NULL) {
      copy_entry_to_destination(current_entry, the_config);
    } else {
      // si il y a une différence
      if (mismatch(current_entry, current_dest, the_config->uses_md5)) {
        copy_entry_to_destination(current_entry, the_config);
      }
    }
    current_entry = current_entry->next;
  }
}

/*!
 * @brief mismatch tests if two files with the same name (one in source, one in destination) are equal
 * @param lhd a files list entry from the source
 * @param rhd a files list entry from the destination
 * @has_md5 a value to enable or disable MD5 sum check
 * @return true if both files are not equal, false else
 */
bool mismatch(files_list_entry_t *lhd, files_list_entry_t *rhd, bool has_md5) {

  // compare les sommes MD5 si activer
  if (has_md5 == true) {
    for (int i = 0; i < 16; i++) {
      if (lhd->md5sum[i] != rhd->md5sum[i]) {
        return true;
      }
    }
  }

  // compare les autres attributs des fichiers
  if (lhd->size != rhd->size || lhd->mtime.tv_nsec != rhd->mtime.tv_nsec || lhd->mtime.tv_sec != rhd->mtime.tv_sec || lhd->mode != rhd->mode) {
    return true;
  } else {
    return false;
  }
}

/*!
 * @brief make_files_list buils a files list in no parallel mode
 * @param list is a pointer to the list that will be built
 * @param target_path is the path whose files to list
 */
void make_files_list(files_list_t *list, char *target_path) {
  //test erreur argument
  if (list == NULL || target_path == NULL) {
    fprintf(stderr, "Error: Invalid input parameters.\n");
    return;
  }

  make_list(list, target_path);

  // parcourt la liste et obtient les statistiques de chaque fichier
  files_list_entry_t *p_entry = list->head;
  while (p_entry != NULL) {
    get_file_stats(p_entry);
    p_entry = p_entry->next;
  }
}

/*!
 * @brief make_files_lists_parallel makes both (src and dest) files list with parallel processing
 * @param src_list is a pointer to the source list to build
 * @param dst_list is a pointer to the destination list to build
 * @param the_config is a pointer to the program configuration
 * @param msg_queue is the id of the MQ used for communication
 */
void make_files_lists_parallel(files_list_t *src_list, files_list_t *dst_list, configuration_t *the_config, int msg_queue) {
}

/*!
 * @brief copy_entry_to_destination copies a file from the source to the destination
 * It keeps access modes and mtime (@see utimensat)
 * Pay attention to the path so that the prefixes are not repeated from the source to the destination
 * Use sendfile to copy the file, mkdir to create the directory
 */
void copy_entry_to_destination(files_list_entry_t *source_entry, configuration_t *the_config) {
  //test erreur argument
  if (source_entry == NULL || the_config == NULL) {
    fprintf(stderr, "Invalid arguments to copy_entry_to_destination\n");
    return;
  }

  //initialisation
  char source[1024];
  strcpy(source, the_config->source);
  char destination[1024];
  strcpy(destination, the_config->destination);

  //si dossier, créer dossier destination
  if (source_entry->entry_type == DOSSIER){
    char path[PATH_SIZE];   
    concat_path(path, destination, source_entry->path_and_name + strlen(the_config->source) + 1);
    mkdir(path, source_entry->mode);
  } else {
    //si fichier, copie le fichier
    off_t offset = 0;
    char source_file[PATH_SIZE];
    char destination_file[PATH_SIZE];

    //construit les chemins des fichiers
    concat_path(source_file, source, source_entry->path_and_name);
    concat_path(destination_file, destination, source_entry->path_and_name + strlen(the_config->source) + 1);

    //ouvre les fichiers
    int fd_source, fd_destination;
    fd_source = open(source_entry->path_and_name, O_RDONLY);
    fd_destination = open(destination_file, O_WRONLY | O_CREAT | O_TRUNC, source_entry->mode);
    sendfile(fd_destination, fd_source, &offset, source_entry->size);

    //ferme les fichiers
    close(fd_source);
    close(fd_destination);
  }
  return;
}

/*!
 * @brief make_list lists files in a location (it recurses in directories)
 * It doesn't get files properties, only a list of paths
 * This function is used by make_files_list and make_files_list_parallel
 * @param list is a pointer to the list that will be built
 * @param target is the target dir whose content must be listed
 */
void make_list(files_list_t *list, char *target) {
  //test erreur argument
  if (list == NULL || target == NULL) {
    fprintf(stderr, "Error: Invalid input parameters.\n");
    return;
  }

  //ouvre le dossier
  DIR *dir = opendir(target);
  if (dir == NULL) {
    perror("Error opening directory");
    return;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    // Skip "." and ".."
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    // construit le chemin
    char file_path[4096];
    snprintf(file_path, sizeof(file_path), "%s/%s", target, entry->d_name);

    // ajoute le fichier dans la liste
    add_file_entry(list, file_path);
  }
  closedir(dir);
}

/*!
 * @brief open_dir opens a dir
 * @param path is the path to the dir
 * @return a pointer to a dir, NULL if it cannot be opened
 */
DIR *open_dir(char *path) {
  DIR *dir = opendir(path);
  
  if (dir == NULL) {
    perror("Error opening directory");
  }

  return dir;
}

/*!
 * @brief get_next_entry returns the next entry in an already opened dir
 * @param dir is a pointer to the dir (as a result of opendir, @see open_dir)
 * @return a struct dirent pointer to the next relevant entry, NULL if none found (use it to stop iterating)
 * Relevant entries are all regular files and dir, except . and ..
 */
struct dirent *get_next_entry(DIR *dir) {
  if (dir == NULL) {
    return NULL;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    // Check if the entry is relevant (not . or .. and is a regular file or directory)
    if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 && (entry->d_type == DT_REG || entry->d_type == DT_DIR)) {
      return entry; // Relevant entry found
    }
  }
  
  return NULL;
}
