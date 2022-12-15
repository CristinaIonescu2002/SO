#ifndef SO_FUNCTIONS_H_
#define SO_FUNCTIONS_H_

#define DIE(assertion, call_description)                \
    do {                                \
        if (assertion) {                    \
            fprintf(stderr, "(%s, %d): ",            \
                    __FILE__, __LINE__);        \
            perror(call_description);            \
            exit(EXIT_FAILURE);                \
        }                            \
    } while (0)

#define NEW 1                       //aici avem starile pe care le poate avea un thread
#define READY 2
#define RUNNING 3
#define WAITING 4
#define TERMINATED 5

typedef struct thread {             //structura pentru thread
    int priority;
    int quantum_left;
    int status;
    int io_thread;
    so_handler *handler;
    tid_t tid;
    sem_t sem_thread;
} thread_t;

typedef struct node_list {          //structura pentru nodul din lista
    thread_t *thread;
    struct node_list *next;
    struct node_list *prev;
} node_t;

typedef struct thread_list {        //structura pentru lista
    node_t *head;
    node_t *tail;
    int len;
} list_t;

static int scheduler_initializat;   //toate astea vor contine informatiile schedulerului
static int quantum_sch;
static int io_sch;
static thread_t *thread_in_running;
static list_t *list_threads;        //aici vom retine toate threadurile in ordine crearii
static list_t *ready_threads;       //aici vom retine threadurile care sunt in READY in ordinea
                                        //descrescatoare a prioritatii lor

list_t *initList(void);

void destroy_list(list_t *list);

#endif