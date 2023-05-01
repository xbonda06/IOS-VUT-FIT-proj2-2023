/*
 * IOS – projekt 2 (synchronizace)
 *
 * Autor: Andrii Bondarenko (xbonda06)
 *
 * Datum: 30.04.2023
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <time.h>
#include <semaphore.h>
#include <stdarg.h>

//Definice konstant
#define MAX_WAIT_TIME 10000
#define MAX_BREAK_TIME 100
#define MAX_CLOSE_TIME 10000

#define WRONG_ARGS_NUM 1
#define WRONG_FORMAT 2
#define WRONG_MAX_WAIT 3
#define WRONG_MAX_BREAK 4
#define WRONG_POST_OFFICE_CLOSE 5
#define FILE_OPEN_ERROR 6
#define WRONG_WORKERS_AMOUNT 7

// Striktura, ktera obsahuje argumenty
typedef struct Arguments {
    int client_amount; // pocet zakazniku - NZ
    volatile int workers_amount; // pocet uradniku - NU
    int max_waiting_time; // maximalni cas v ms, po ktery zakaznik ceka, nez vejde na postu - TZ
    int max_break_time; // maximalni delka prestavky urednika - TU
    int post_office_close; // cas v ms, po kterem se posta zavre - F
}arg_t;

// Struktura sdilene pameti
typedef struct {
    int action_counter;
    int office_closed;
    sem_t mutex;
    sem_t customers_in_queue[4];
    sem_t workers_lock;
} shared_t;

// Funkce pro zpracovani argumentu a identifikaci chyb
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
                    return WRONG_FORMAT; // pokud neni argument cislo
                }
                if(args->client_amount <= 0){
                    return WRONG_FORMAT; // pokud je pocet zakazniku mensi nez 0
                }
                break;
            case 2:
                args->workers_amount = strtol(argv[i], &rest, 10);
                if(rest[0] != '\0'){
                    return WRONG_FORMAT;
                }
                if(args->workers_amount <= 0){
                    return WRONG_WORKERS_AMOUNT;
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

// Funkce pro vypis chybovych hlaseni na stderr
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
        case WRONG_WORKERS_AMOUNT:
            fprintf(stderr, "Number of workers must be more than 0\n");
            break;
    }
}

// Funkce pro vypis akci do souboru
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

// Funkce pro inicializaci semaforu
void semaphores_init(shared_t *shared_data){
    sem_init(&shared_data->mutex, 1, 1);
    sem_init(&shared_data->workers_lock, 1, 0);
    for (int i = 1; i <= 3; ++i) {
        sem_init(&shared_data->customers_in_queue[i], 1, 0);
    }
}

// Funkce pro zruseni semaforu
void destroy_semaphores(shared_t *shared_data){
    sem_destroy(&shared_data->mutex);
    sem_destroy(&shared_data->workers_lock);
    for (int i = 1; i <= 3; ++i) {
        sem_destroy(&shared_data->customers_in_queue[i]);
    }
}

// Process pro zakaznika
void client_process(int id, shared_t *shared_data, int max_waiting_time) {
    srand(time(NULL) ^ (getpid() << 16)); // inicializace generatoru nahodnych cisel

    FILE *file = fopen("proj2.out", "a");
    if (file == NULL) {
        error_print(FILE_OPEN_ERROR);
        exit(1);
    }

    print_action(file, shared_data, "Z %d: started\n", id); // vypis zacatku procesu

    usleep(rand() % (max_waiting_time + 1)); // cekani na prichod do posty

    sem_wait(&shared_data->mutex); // pristup k sdilenym promennym

    if (shared_data->office_closed) { // pokud je posta uzavrena
        sem_post(&shared_data->mutex);
        print_action(file, shared_data, "Z %d: going home\n", id); // zakaznik jde domu (zitra je take den)
        fclose(file);
        exit(0);
    } else {
        sem_post (&shared_data->mutex); // uvolneni pristupu k sdilenym promennym

        int service = rand() % 3 + 1;

        print_action(file, shared_data, "Z %d: entering office for a service %d\n", id, service); // vypis vstupu do posty

        sem_post(&shared_data->customers_in_queue[service]); // vstup do fronty na urcitu sluzbu

        sem_wait(&shared_data->workers_lock); // cekani na volneho pracovnika

        print_action(file, shared_data, "Z %d: called by office worker\n", id); // vypis volani zakaznika pracovnikem

        usleep(rand() % 11); // cekani na dokonceni sluzby

        print_action(file, shared_data, "Z %d: going home\n", id); // vypis odchodu zakaznika
    }

    fclose(file);
    exit(0);
}

void worker_process(int id, shared_t *shared_data, int TU) {
    FILE *file = fopen("proj2.out", "a");
    if (file == NULL) {
        error_print(FILE_OPEN_ERROR);
        exit(1);
    }

    print_action(file, shared_data, "U %d: started\n", id); // vypis zacatku procesu

    while (1) { // nekonecna smycka
        int service = rand() % 3 + 1; // vyber sluzby pro pracovnika

        if (sem_trywait(&shared_data->customers_in_queue[service]) == 0) { // pokud je fronta na sluzbu neprazdna
            print_action(file, shared_data, "U %d: serving a service of type %d\n", id, service); // vypis zacatku obsluhy zakaznika

            usleep(rand() % 11); // cekani na dokonceni sluzby

            print_action(file, shared_data, "U %d: service finished\n", id); // vypis dokonceni obsluhy zakaznika

            sem_post(&shared_data->workers_lock); // uvolneni pracovnika
        } else {

            print_action(file, shared_data, "U %d: taking break\n", id); // vypis zacatku prestavky

            usleep(rand() % (TU + 1)); // cekani na dokonceni prestavky

            print_action(file, shared_data, "U %d: break finished\n", id); // vypis dokonceni prestavky

            if(shared_data->office_closed){ // pokud je posta uzavrena
                break; // ukonceni smycky
            }
        }
    }

    print_action(file, shared_data, "U %d: going home\n", id); // vypis odchodu pracovnika

    fclose(file);
    exit(0);
}

int main(int argc, char *argv[]) {
    arg_t args; // struktura pro ulozeni argumentu prikazove radky
    int error = parse_args(argc, argv, &args); // zpracovani argumentu prikazove radky a ulozeni do struktury
    if(error != 0){ // pokud nastala chyba
        error_print(error); // vypis chyboveho hlaseni
        exit(1);
    }

    srand(time(NULL)); // inicializace generatoru nahodnych cisel

    FILE *file = fopen("proj2.out", "w"); // vytvoreni souboru pro vypis
    if(file == NULL){
        error_print(FILE_OPEN_ERROR);
        exit(1);
    }
    fclose(file);

    shared_t *shared_data = mmap(NULL, sizeof(shared_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0); // vytvoreni sdilene pameti
    shared_data->action_counter = 0;
    shared_data->office_closed = 0;

    semaphores_init(shared_data);

    for (int i = 1; i <= args.client_amount; ++i) { // vytvoreni procesu zakazniku
        int pid = fork();
        if (pid == 0) { // pokud je proces potomek
            client_process(i, shared_data, args.max_waiting_time); // spusteni procesu zakaznika
        }
    }

    for (int i = 1; i <= args.workers_amount; ++i) { // vytvoreni procesu pracovniku
        int pid = fork();
        if (pid == 0) { // pokud je proces potomek
            worker_process(i, shared_data, args.max_break_time); // spusteni procesu pracovnika
        }
    }

    usleep((args.post_office_close / 2 + rand() % (args.post_office_close / 2 + 1)) * 1000); // cekani na nahodny cas do uzavreni posty

    file = fopen("proj2.out", "a");
    print_action(file, shared_data, "closing\n"); // vypis uzavreni posty
    fclose(file);

    shared_data->office_closed = 1; // nastaveni promenne pro ukonceni procesu

    usleep(args.max_waiting_time * 1000); // cekani na dokonceni vsech zakazniku

    int status;
    int pid;
    int finished = 0;
    int clients_finished = 0;
    while ((finished < args.client_amount + args.workers_amount) && (pid = waitpid(-1, &status, WNOHANG)) > 0) { // cekani na dokonceni vsech procesu
        finished++;

        if (pid >= 1 && pid <= args.client_amount) { // pokud se jedna o proces zakaznika
            clients_finished++;
        }

        if (clients_finished == args.client_amount) { // pokud jsou dokonceni vsichni zakaznici
            shared_data->office_closed = 1; // nastaveni promenne pro ukonceni procesu
        }

        usleep(1000);
    }

    destroy_semaphores(shared_data);

    munmap(shared_data, sizeof(shared_t));

    exit(0);
}
