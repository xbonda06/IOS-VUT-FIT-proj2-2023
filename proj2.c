#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <time.h>
#include <semaphore.h>
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
    int workers_amount; // pocet uradniku - NU
    int max_waiting_time; // maximalni cas v ms, po ktery zakaznik ceka, nez vejde na postu - TZ
    int max_break_time; // maximalni delka prestavky urednika - TU
    int post_office_close; // cas v ms, po kterem se posta zavre - F
}arg_t;

typedef struct {
    int action_counter;
    int office_closed;
    sem_t mutex;
    sem_t customers_in_queue[4];
    sem_t workers_lock;
} shared_t;

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
                args->workers_amount = strtol(argv[i], &rest, 10);
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

void print_action(FILE *file, shared_t *shared_data, const char *format, ...) {
    va_list args;
    va_start(args, format);

    sem_wait(&shared_data->mutex);
    fprintf(file, "%d: ", ++shared_data->action_counter);
    vfprintf(file, format, args);
    fflush(file);
    sem_post(&shared_data->mutex);

    va_end(args);
}

void semaphores_init(shared_t *shared_data){
    sem_init(&shared_data->mutex, 1, 1);
    sem_init(&shared_data->workers_lock, 1, 0);
    for (int i = 1; i <= 3; ++i) {
        sem_init(&shared_data->customers_in_queue[i], 1, 0);
    }
}

void destroy_semaphores(shared_t *shared_data){
    sem_destroy(&shared_data->mutex);
    sem_destroy(&shared_data->workers_lock);
    for (int i = 1; i <= 3; ++i) {
        sem_destroy(&shared_data->customers_in_queue[i]);
    }
}

void client_process(int id, shared_t *shared_data, int TZ) {
    FILE *file = fopen("proj2.out", "a");
    if (file == NULL) {
        exit(FILE_OPEN_ERROR);
        exit(1);
    }

    print_action(file, shared_data, "Z %d: started\n", id);

    usleep(rand() % (TZ + 1));

    if (shared_data->office_closed) {
        print_action(file, shared_data, "Z %d: going home\n", id);
    } else {
        int service = rand() % 3 + 1;

        print_action(file, shared_data, "Z %d: entering office for a service %d\n", id, service);

        sem_post(&shared_data->customers_in_queue[service]);

        sem_wait(&shared_data->workers_lock);

        print_action(file, shared_data, "Z %d: called by office worker\n", id);

        usleep(rand() % 11);

        print_action(file, shared_data, "Z %d: going home\n", id);
    }

    fclose(file);
    exit(0);
}

void worker_process(int id, shared_t *shared_data, int TU) {
    FILE *file = fopen("proj2.out", "a");
    if (file == NULL) {
        exit(FILE_OPEN_ERROR);
        exit(1);
    }

    print_action(file, shared_data, "U %d: started\n", id);

    while (1) {
        int service = rand() % 3 + 1;

        if (sem_trywait(&shared_data->customers_in_queue[service]) == 0) {
            print_action(file, shared_data, "U %d: serving a service of type %d\n", id, service);

            usleep(rand() % 11);

            print_action(file, shared_data, "U %d: service finished\n", id);

            sem_post(&shared_data->workers_lock);
        } else {
            if (shared_data->office_closed) {
                break;
            }

            print_action(file, shared_data, "U %d: taking break\n", id);

            usleep(rand() % (TU + 1));

            if (shared_data->office_closed) {
                break;
            }

            print_action(file, shared_data, "U %d: break finished\n", id);
        }
    }

    print_action(file, shared_data, "U %d: going home\n", id);

    fclose(file);
    exit(0);
}

int main(int argc, char *argv[]) {
    arg_t args;
    int error = parse_args(argc, argv, &args);
    if(error != 0){
        error_print(error);
        exit(1);
    }

    srand(time(NULL));

    FILE *file = fopen("proj2.out", "w");
    if(file == NULL){
        error_print(FILE_OPEN_ERROR);
        exit(1);
    }
    fclose(file);

    shared_t *shared_data = mmap(NULL, sizeof(shared_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    shared_data->action_counter = 0;
    shared_data->office_closed = 0;

    semaphores_init(shared_data);

    for (int i = 1; i <= args.client_amount; ++i) {
        int pid = fork();
        if (pid == 0) {
            client_process(i, shared_data, args.max_waiting_time);
        }
    }

    for (int i = 1; i <= args.workers_amount; ++i) {
        int pid = fork();
        if (pid == 0) {
            worker_process(i, shared_data, args.max_break_time);
        }
    }

    usleep((args.post_office_close / 2 + rand() % (args.post_office_close / 2 + 1)) * 1000);

    file = fopen("proj2.out", "a");
    print_action(file, shared_data, "closing\n");
    fclose(file);

    shared_data->office_closed = 1;

    int status;
    while (wait(&status) > 0);

    destroy_semaphores(shared_data);

    munmap(shared_data, sizeof(shared_t));

    return 0;
}
