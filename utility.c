#include <defines.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*!
 * @brief concat_path concatenates suffix to prefix into result
 * It checks if prefix ends by / and adds this token if necessary
 * It also checks that result will fit into PATH_SIZE length
 * @param result the result of the concatenation
 * @param prefix the first part of the resulting path
 * @param suffix the second part of the resulting path
 * @return a pointer to the resulting path, NULL when concatenation failed
 */
char *concat_path(char *result, char *prefix, char *suffix) {
  
  if (result == NULL || prefix == NULL || suffix == NULL) {
    fprintf(stderr, "Erreur : Paramètres d'entrée non valides\n");
    return NULL;
  }
  
  if (snprintf(result, PATH_SIZE, "%s/%s", prefix, suffix) >= PATH_SIZE) {
    fprintf(stderr, "Erreur : Le chemin résultant dépasse la taille maximale\n");
    return NULL;
  }
  
  return result;
}
