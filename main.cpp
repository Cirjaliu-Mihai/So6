#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <time.h>

typedef struct {
    sem_t semafor;
    int numar_curent;
} SharedData;

#define SHARE_SIZE sizeof(SharedData)
#define LIMITA_MAXIMA 1000
#define NUMAR_PROCESE_COPIL 2

void run_process(SharedData *shared_mem, int process_id) {
    int valoare_noua;
    int roll;

    srand(getpid());
    
    while (1) {
        if (sem_wait(&shared_mem->semafor) == -1) {
            perror("sem_wait eroare");
            exit(EXIT_FAILURE);
        }

        valoare_noua = shared_mem->numar_curent;
        
        if (valoare_noua >= LIMITA_MAXIMA) {
            sem_post(&shared_mem->semafor); 
            break; 
        }
        
        do {
            roll = (rand() % 2) + 1; 
            
            if (roll == 2 && valoare_noua < LIMITA_MAXIMA) {
                valoare_noua++;
                shared_mem->numar_curent = valoare_noua;
                if (valoare_noua % 250 == 0 || valoare_noua == 1) {
                     printf("Procesul %d: Scrie %d\n", process_id, valoare_noua);
                }
            }
            
        } while (roll == 2 && valoare_noua < LIMITA_MAXIMA);
        
        if (sem_post(&shared_mem->semafor) == -1) {
            perror("sem_post eroare");
            exit(EXIT_FAILURE);
        }

        usleep(100); 
    }
}

int main() {
    SharedData *shared_mem;
    pid_t pid;
    int i;

    shared_mem = mmap(NULL, SHARE_SIZE, PROT_READ | PROT_WRITE, 
                      MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (shared_mem == MAP_FAILED) {
        perror("mmap eroare");
        return EXIT_FAILURE;
    }

    shared_mem->numar_curent = 0;
    
    if (sem_init(&shared_mem->semafor, 1, 1) == -1) {
        perror("sem_init eroare");
        munmap(shared_mem, SHARE_SIZE);
        return EXIT_FAILURE;
    }

    for (i = 1; i <= NUMAR_PROCESE_COPIL; i++) {
        pid = fork();
        
        if (pid == -1) {
            perror("fork eroare");
            sem_destroy(&shared_mem->semafor);
            munmap(shared_mem, SHARE_SIZE);
            exit(EXIT_FAILURE); 
        }

        if (pid == 0) {
            run_process(shared_mem, i);
            exit(EXIT_SUCCESS); 
        }
    }
    
    for (i = 0; i < NUMAR_PROCESE_COPIL; i++) {
        wait(NULL);
    }
    
    printf("Valoarea finală din memorie: %d\n", shared_mem->numar_curent);

    sem_destroy(&shared_mem->semafor);
    munmap(shared_mem, SHARE_SIZE);

    return EXIT_SUCCESS;
}