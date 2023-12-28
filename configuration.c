#include <configuration.h>
#include <stddef.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

typedef enum {DATE_SIZE_ONLY, NO_PARALLEL} long_opt_values;

/*!
 * @brief function display_help displays a brief manual for the program usage
 * @param my_name is the name of the binary file
 * This function is provided with its code, you don't have to implement nor modify it.
 * iii
 */
void display_help(char *my_name) {
    printf("%s [options] source_dir destination_dir\n", my_name);
    printf("Options: \t-n <processes count>\tnumber of processes for file calculations\n");
    printf("         \t-h display help (this text)\n");
    printf("         \t--date_size_only disables MD5 calculation for files\n");
    printf("         \t--no-parallel disables parallel computing (cancels values of option -n)\n");
    printf("         \t--dry-run lists the changes that would need to be synchronized but doesn't perform them\n");
    printf("         \t-v enables verbose mode\n");
}

/*!
 * @brief init_configuration initializes the configuration with default values
 * @param the_config is a pointer to the configuration to be initialized
 */
void init_configuration(configuration_t *the_config) {
    the_config -> processes_count = 2;
    the_config -> is_parallel = true;
    the_config -> is_dry_run = false;
    the_config -> is_verbose = false;
    the_config -> uses_md5 = true;
}

/*!
 * @brief set_configuration updates a configuration based on options and parameters passed to the program CLI
 * @param the_config is a pointer to the configuration to update
 * @param argc is the number of arguments to be processed
 * @param argv is an array of strings with the program parameters
 * @return -1 if configuration cannot succeed, 0 when ok
 */
int set_configuration(configuration_t *the_config, int argc, char *argv[]) {
    DIR *source = opendir(argv[argc-2]);
    DIR *destination = opendir(argv[argc-1]);
    if (access(argv[argc-2], R_OK) && (access(argv[argc-1], W_OK) ||  mkdir(argv[argc-1], 0764))){
        int opt = 0;
        struct option my_opts[] = {
                {.name="date-size-only",.has_arg=0,.flag=0,.val='m'},
                {.name="no-parallel",.has_arg=0,.flag=0,.val='p'},
                {.name="dry-run",.has_arg=0,.flag=0,.val='d'},
                {.name=0,.has_arg=0,.flag=0,.val=0},
        };
        while((opt = getopt_long(argc, argv, "vn:", my_opts, NULL)) != -1) {
            switch (opt) {
                case 'v':
                    the_config -> is_verbose = true;
                    break;
                case 'n':
                    the_config -> processes_count = atoi(optarg);
                    break;
                case 'm':
                    the_config -> uses_md5 = false;
                    break;
                case 'p':
                    the_config -> is_parallel = false;
                    break;
                case 'd':
                    the_config -> is_dry_run = true;
                    break;
            }
        }
        return 0;
    }else{
        display_help(argv[0]);
        return -1;
    }
}
