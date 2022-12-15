#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "./so_scheduler.h"
#include "so_functions.h"

/*De la linia 195 incep functiile cerute, restul sunt functii ajutatoare create de mine*/

list_t *initList(void)         //functie de initializare lista
{
    list_t *new_list = malloc(sizeof(list_t));                      //aloc noua lista dorita
    DIE(new_list == NULL, "malloc");
    new_list->head = NULL;
    new_list->tail = NULL;
    new_list->len = 0;
    return new_list;                                                //o returnez
}

void destroy_list(list_t *list)         //functie de distrugere a listei
{
    node_t *aux;
    while (list->len > 0) {
        aux = list->tail;
        list->tail = list->tail->prev;
        free(aux->thread);
        free(aux);
        list->len--;
    }
    free(list);
}

void init_schaduler(unsigned int time_quantum, unsigned int io)     //functie de initializare scheduler
{
    quantum_sch = time_quantum;
    io_sch = io;
    thread_in_running = NULL;
    list_threads = initList();                                      //aici initializez listele folosite
    ready_threads = initList();
}

void pthread_join_end_list_threads(void)        //functie pentru terminare a thredului + distrugerea semafoarelor
{   int return_val;
    node_t *aux = list_threads->head;
    while (aux != NULL) {                                           //luam toate threadurile create la rand
        return_val = pthread_join(aux->thread->tid, NULL);          //asteptam terminarea thredului curent
        DIE(return_val != 0, "pthread_join");
        return_val = sem_destroy(&aux->thread->sem_thread);         //ii distrugem semaforul
        DIE(return_val != 0, "sem_destroy");
        aux = aux->next;
    }
}

void add_list_threads(thread_t *thread)         //functie de adaugare a threadului in lista care le contine pe toate
{                                               //adaugam toate nodurile in ordine
    node_t *nod = malloc(sizeof(node_t));       //aici alocam nodul
    DIE(nod == NULL, "malloc");
    nod->thread = thread;
    nod->next = NULL;
    nod->prev = NULL;
    if (list_threads->len == 0) {               //cazul in care lista e goala
        list_threads->head = nod;
        list_threads->tail = nod;
    } else {                                    //mereu adaugam nodul la final
        nod->prev = list_threads->tail;
        list_threads->tail->next = nod;
        list_threads->tail = nod;
    }
    list_threads->len++;
}

void add_ready_threads(thread_t *thread)            //functie de adaugare a threadului primit in lista ready_threads
{
    thread->status = READY;
    node_t *nod = malloc(sizeof(node_t));           //alocam nodul care va contine threadul
    DIE(nod == NULL, "malloc");
    nod->thread = thread;
    nod->next = NULL;
    nod->prev = NULL;
    if (ready_threads->len == 0) {                  //daca nu este nimic in lista
        ready_threads->head = nod;
        ready_threads->tail = nod;
    } else if (ready_threads->head->thread->priority < nod->thread->priority) {     //daca prioritatea threadului introdus este
        nod->next = ready_threads->head;                                                //mai mare ca cele deja prezente in lista
        ready_threads->head->prev = nod;                                                //adaugam nodul la inceputul listei
        ready_threads->head = nod;
    } else if (ready_threads->tail->thread->priority >= nod->thread->priority) {    //daca prioritatea thredului introdus este mai mica 
        nod->prev = ready_threads->tail;                                                //sau egala cu cea a ultimului din lista
        ready_threads->tail->next = nod;                                                //adaugam nodul la finalul listei
        ready_threads->tail = nod;
    } else {                                                                        //caz general: cautam unde trebuie introdus threadul
        node_t *r = ready_threads->tail;                                                //si adaugam nodul care il contine
        while (r->thread->priority < thread->priority)
            r = r->prev;
        nod->next = r->next;
        nod->prev = r;
        r->next->prev = nod;
        r->next = nod;
    }
    ready_threads->len++;
}

void pop_ready_threads(void)        //functie de eliminare a primului thread din lista ready_threads
{
    node_t *node = ready_threads->head;
    if (ready_threads->len == 1) {              //cazul in care lista are doar un element
        node->thread = NULL;
    } else {                                    //cazul in care sunt mai multe elemente in lista
        ready_threads->head = node->next;
        ready_threads->head->prev = NULL;
        node->thread = NULL;
    }
    free(node);                                 //eliberam nodul
    ready_threads->len--;
}

void running_next_thread(void)                   //functie care are grija ca in starea de RUNNING sa ajunga threadul care trebuie
{                                                   
    int return_val;
    if (ready_threads->len == 0 && thread_in_running != NULL) {
        return_val = sem_post(&thread_in_running->sem_thread);              //in acest caz doar deblocam semaforul threadului din RUNNING
        DIE(return_val != 0, "sem_post");
        return;
    } else if (thread_in_running == NULL || thread_in_running->status == WAITING
                                            || thread_in_running->status == TERMINATED) {
        thread_in_running = ready_threads->head->thread;                    //aici primul thread din running_threads trebuie sa ajunga in RUNNING
        pop_ready_threads();                                                //scoatem acest thread din lista
        thread_in_running->status = RUNNING;                                //ii schimbam starea si ii resetam quanta
        thread_in_running->quantum_left = quantum_sch;
        return_val = sem_post(&thread_in_running->sem_thread);              //ii deblocam semaforul
        DIE(return_val != 0, "sem_post");
        return;
    } else if (thread_in_running->priority < ready_threads->head->thread->priority) {
        add_ready_threads(thread_in_running);                               //in acest caz pe langa pasii pe care i-am facut la cazul anterior
        thread_in_running = ready_threads->head->thread;                        //mai si readaugam threadul care a fost in RUNNING inapoi in
        pop_ready_threads();                                                    //lista de ready_threads
        thread_in_running->status = RUNNING;
        thread_in_running->quantum_left = quantum_sch;
        return_val = sem_post(&thread_in_running->sem_thread);
        DIE(return_val != 0, "sem_post");
        return;
    } else if (thread_in_running->quantum_left <= 0) {                     //caul in care cuanta a expirat
        if (thread_in_running->priority == ready_threads->head->thread->priority) { //daca am gasit unul care are aceeasi prioritate cu cel din RUNNING
            thread_t *th = thread_in_running;                              //retinem threadul din running
            thread_in_running = ready_threads->head->thread;               //si repetam pasii de la cele doua cazuri de mai sus, dar avem grija sa
            pop_ready_threads();
            add_ready_threads(th);                                         //adaugam threadul retinut inapoi in coada
            thread_in_running->status = RUNNING;
            thread_in_running->quantum_left = quantum_sch;
            return_val = sem_post(&thread_in_running->sem_thread);
            DIE(return_val != 0, "sem_post");
            return;
        }
        thread_in_running->quantum_left = quantum_sch;                      //daca nu se gaseste un thread cu aceeasi prioritate doar resetam quanta
    }
    return_val = sem_post(&thread_in_running->sem_thread);                  //ii deblocam semaforul
    DIE(return_val != 0, "sem_post");
}

void pt_aflare_pid(void *args)          //functie dupa modelul "start_thread" din cerinta care ne va da contextul in care
{                                           //se va executa threadul
    thread_t *thread = (thread_t *)args;
    int return_val = sem_wait(&thread->sem_thread);
    DIE(return_val != 0, "sem_wait");
    thread->handler(thread->priority);
    thread->status = TERMINATED;
    running_next_thread();
}
thread_t *init_thread(so_handler *func, unsigned int priority)          //functie de initializare a threadului
{
    thread_t *thread = malloc(sizeof(thread_t));                                                //aloc threadul
    DIE(thread == NULL, "malloc");
    thread->priority = priority;                                                                //atribuim valorile care trebuie
    thread->quantum_left = quantum_sch;
    thread->status = NEW;
    thread->io_thread = SO_MAX_NUM_EVENTS;
    thread->handler = func;
    int return_val = sem_init(&thread->sem_thread, 0, 0);                                       //ii initializam semaforul
    DIE(return_val != 0, "sem_init");
    return_val = pthread_create(&thread->tid, NULL, (void *)pt_aflare_pid, (void *)thread);     //ii aflam pid-ul
    DIE(return_val != 0, "pthred_create");
    return thread;                                                                              //returnam threadul creat
}

/*Pana aici sunt functiile ajutatoare create*/

int so_init(unsigned int time_quantum, unsigned int io)
{
    if (scheduler_initializat == 1 || time_quantum == 0 || io > SO_MAX_NUM_EVENTS)              //daca am primit parametrii prosti
        return -1;
    scheduler_initializat = 1;                                      //marcam imitializarea schedulerului
    init_schaduler(time_quantum, io);                               //folosim functia de initializare a lui
    return 0;
}

tid_t so_fork(so_handler *func, unsigned int priority)
{
    if (func == NULL ||  priority > SO_MAX_PRIO)                    //daca am primit parametrii prosti
        return INVALID_TID;
    thread_t *thread = init_thread(func, priority);                 //initializam threadul cu functia de initializare a lui
    add_list_threads(thread);                                       //adugam threadul in lista care le contine pe toate (list_thtreads)
    add_ready_threads(thread);                                      //si in lista cu threaduri in stare READY (ready_threads)
    if (thread_in_running == NULL)                                  //daca nu avem thread care e in RUNNING folosim:
        running_next_thread();
    else
        so_exec();                                                  //daca avem thread in RUNNING apelam exec
    return thread->tid;
}

int so_wait(unsigned int io)
{
    if (io < 0 || io >= io_sch)                                     //daca numarul de evenimente nu este bun
        return -1;
    thread_in_running->status = WAITING;                            //trecem threadul care e in RUNNING in starea WAITING
    thread_in_running->io_thread = io;                              //ii dam numarul nou de dispozitive I/O
    so_exec();                                                      //apelam exec
    return 0;
}

int so_signal(unsigned int io)
{
    if (io < 0 || io >= io_sch)                                     //daca numarul de evenimente nu este bun
        return -1;
    node_t *nod = list_threads->head;
    int nr_w_threads = 0;
    while (nod != NULL) {                                           //trecem prin toate threadurile 
        if (nod->thread->io_thread == io && nod->thread->status == WAITING) {  //verificam cate threaduri sunt in WAITING si asteapta io-ul dat
            nod->thread->io_thread = SO_MAX_NUM_EVENTS;
            add_ready_threads(nod->thread);
            nr_w_threads++;                                         //le contorizam
        }
        nod = nod->next;
    }
    so_exec();                                                      //apelam iar exec
    return nr_w_threads;                                            //returnam cate am gasit
}

void so_exec(void)
{
    thread_t *thread = thread_in_running;
    thread_in_running->quantum_left--;                              //scadem din cuante threadului din RUNNING
    running_next_thread();                                           //folosim:   pentru a verifica daca isi continua rularea sau nu
    int return_val = sem_wait(&thread->sem_thread);                 //ii punem semaforul in Wait
    DIE(return_val != 0, "sem_wait");  
}

void so_end(void)
{
    if (scheduler_initializat == 0)                                 //daca nu a fost initializat schedulerul
        return;
    pthread_join_end_list_threads();                                //functia pentru asteptare terminare threads si distrugere semafoare
    destroy_list(list_threads);                                     //distrugem lista cu toate threadurile
    free(ready_threads);                                            //eliberam lista cu threaduri in READY 
    scheduler_initializat = 0;                                      //anulam marcarea schedulerului (ne mai fiind initializat)
}