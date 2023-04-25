// IOS – projekt 2 (synchronizace)
// Autor: Andrii Bondarenko (xbonda06)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>

typedef struct Arguments {
    int client_amount; // pocet zakazniku - NZ
    int officials_amount; // pocet uradniku - NU
    int max_waiting_time; // maximalni cas v ms, po ktery zakaznik ceka, nez vejde na postu - TZ
    int max_break_time; // maximalni delka prestavky urednika - TU
    int post_office_close; // cas v ms, po kterem se posta zavre - F
}arg_t;

int parse_args(int argc, char *argv[], arg_t *args) {
    if (argc != 6) {
        fprintf(stderr, "Wrong number of arguments\n");
        return 1;
    }
    char *rest;
    for(int i = 1; i < argc; i++) {
        switch(i) {
            case 1:
                args->client_amount = strtol(argv[i], &rest, 10);
                if(rest[0] != '\0'){
                    fprintf(stderr, "Wrong format of arguments\n");
                    return 1;
                }
                break;
            case 2:
                args->officials_amount = strtol(argv[i], &rest, 10);
                if(rest[0] != '\0'){
                    fprintf(stderr, "Wrong format of arguments\n");
                    return 1;
                }
                break;
            case 3:
                args->max_waiting_time = strtol(argv[i], &rest, 10);
                if (rest[0] != '\0') {
                    fprintf(stderr, "Wrong format of arguments\n");
                    return 1;
                }
                if (args->max_waiting_time < 0 || args->max_waiting_time > 10000) {
                    fprintf(stderr, "Wrong max waiting time\n");
                    return 1;
                }
                break;
            case 4:
                args->max_break_time = strtol(argv[i], &rest, 10);
                if (rest[0] != '\0') {
                    fprintf(stderr, "Wrong format of arguments\n");
                    return 1;
                }
                if (args->max_break_time < 0 || args->max_break_time > 100) {
                    fprintf(stderr, "Wrong max break time\n");
                    return 1;
                }
                break;
            case 5:
                args->post_office_close = strtol(argv[i], &rest, 10);
                if (rest[0] != '\0') {
                    fprintf(stderr, "Wrong format of arguments\n");
                    return 1;
                }
                if (args->post_office_close < 0 || args->post_office_close > 10000) {
                    fprintf(stderr, "Wrong post office close time\n");
                    return 1;
                }
                break;
        }
    }


    return 0;
}

int main(int argc, char *argv[]){
    arg_t args;
    parse_args(argc, argv, &args);




    return 0;
}
