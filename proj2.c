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
#include <stdarg.h>

#define MAX_WAIT_TIME 10000
#define MAX_BREAK_TIME 100
#define MAX_CLOSE_TIME 10000

#define WRONG_ARGS_NUM 1
#define WRONG_FORMAT 2
#define WRONG_MAX_WAIT 3
#define WRONG_MAX_BREAK 4
#define WRONG_POST_OFFICE_CLOSE 5
#define FILE_OPEN_ERROR 6

typedef struct Arguments {
    int client_amount; // pocet zakazniku - NZ
    int officials_amount; // pocet uradniku - NU
    int max_waiting_time; // maximalni cas v ms, po ktery zakaznik ceka, nez vejde na postu - TZ
    int max_break_time; // maximalni delka prestavky urednika - TU
    int post_office_close; // cas v ms, po kterem se posta zavre - F
}arg_t;

typedef struct Shared{
    int action_counter;
    int close_time;
    sem_t mutex;
    sem_t client_queue[3];
    sem_t official_break;
    FILE *file;
}shared_t;

int parse_args(int argc, char *argv[], arg_t *args) {
    if (argc != 6) {
        return WRONG_ARGS_NUM;
    }
    char *rest;
    for(int i = 1; i < argc; i++) {
        switch(i) {
            case 1:
                args->client_amount = strtol(argv[i], &rest, 10);
                if(rest[0] != '\0'){
                    return WRONG_FORMAT;
                }
                break;
            case 2:
                args->officials_amount = strtol(argv[i], &rest, 10);
                if(rest[0] != '\0'){
                    return WRONG_FORMAT;
                }
                break;
            case 3:
                args->max_waiting_time = strtol(argv[i], &rest, 10);
                if (rest[0] != '\0') {
                    return WRONG_FORMAT;
                }
                if (args->max_waiting_time < 0 || args->max_waiting_time > MAX_WAIT_TIME) {
                    return WRONG_MAX_WAIT;
                }
                break;
            case 4:
                args->max_break_time = strtol(argv[i], &rest, 10);
                if (rest[0] != '\0') {
                    return WRONG_FORMAT;
                }
                if (args->max_break_time < 0 || args->max_break_time > MAX_BREAK_TIME) {
                    return WRONG_MAX_BREAK;
                }
                break;
            case 5:
                args->post_office_close = strtol(argv[i], &rest, 10);
                if (rest[0] != '\0') {
                    return WRONG_FORMAT;
                }
                if (args->post_office_close < 0 || args->post_office_close > MAX_CLOSE_TIME) {
                    return WRONG_POST_OFFICE_CLOSE;
                }
                break;
        }
    }
    return 0;
}

void error_print(int error){
    switch (error) {
        case 0:
            break;
        case WRONG_ARGS_NUM:
            fprintf(stderr, "Wrong number of arguments\n");
            break;
        case WRONG_FORMAT:
            fprintf(stderr, "Wrong format of arguments\n");
            break;
        case WRONG_MAX_WAIT:
            fprintf(stderr, "Wrong max waiting time\n");
            break;
        case WRONG_MAX_BREAK:
            fprintf(stderr, "Wrong max break time\n");
            break;
        case WRONG_POST_OFFICE_CLOSE:
            fprintf(stderr, "Wrong post office close time\n");
            break;
        case FILE_OPEN_ERROR:
            fprintf(stderr, "Do not pissible to open file\n");
            break;
    }
}

void open_file(shared_t *Shared){
    Shared->file = fopen("prog2.out", "a");
    if(Shared->file == NULL){
        error_print(FILE_OPEN_ERROR);
        exit(1);
    }
}

void init_semaphores(shared_t *shared){
    sem_init(&shared->mutex, 1, 1);
    sem_init(&shared->official_break, 1, 0);
    for(int i = 0; i < 3; i++){
        sem_init(&shared->client_queue[i], 1, 0);
    }
}

void print_action(FILE *file, shared_t *shared, const char *format, ...) {
    va_list args;
    va_start(args, format);

    sem_wait(&shared->mutex);
    fprintf(file, "%d: ", ++shared->action_counter);
    vfprintf(file, format, args);
    fflush(file);
    sem_post(&shared->mutex);

    va_end(args);
}

int main(int argc, char *argv[]){
    arg_t args;
    shared_t *shared = mmap(NULL, sizeof(shared_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    shared->file = NULL;
    shared->action_counter = 0;
    shared->close_time = 0;


    int error = parse_args(argc, argv, &args);
    if(error != 0){
        error_print(error);
        return 1;
    }

    srand(time(NULL));

    open_file(shared);
    fclose(shared->file);

    init_semaphores(shared);

    for(int i = 1; i <= args.client_amount; ++i){
        pid_t pid = fork();
        if(pid == 0){
            // TODO: client_process(i, shared, args.max_waiting_time);
        }
    }

    for(int i = 1; i <= args.officials_amount; ++i){
        pid_t pid = fork();
        if(pid == 0){
            // TODO: official_process(i, shared, args.max_break_time);
        }
    }

    usleep((args.post_office_close / 2 + rand() % (args.post_office_close / 2 + 1)) * 1000);

    open_file(shared);
    print_action(shared->file, shared, "A: closing\n");
    fclose(shared->file);












    return 0;
}
