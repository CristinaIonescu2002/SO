# Planificator de threaduri
- Ionescu Cristina Grupa 323CC

    Am ales sa implementez acest planificator de threaduri astfel:
- informatiile deste handler vor fi retinute in niste variabile globale, statice
denumite intuitiv
- [scheduler_initializat]   -   0 => nu e initializat
                            -   1 => e initializat
- ca structuri vom avea: - pentru lista
                         - pentru nodul listei
                         - pentru thread
- vor fi doua liste "list_threads" si "ready_threads"
- [list_threads] va contine toate threadurile care se vor crea, in ordinea crearii
(pe principiul primul venit, primul introdus)
- [ready_threads] va contine doar threadurile care sunt in starea READY, ele vor fi
retinute in ordinea descrescatoare prioritatii lor (primul din lista va avea cea mai
mare prioritate, iar ultimul cea mai mica); am grija ca pe langa ordinea de prioritati
sa fie respectata si ordinea venirii (daca avem threaduri cu prioritatile 5 5 4 3 daca
vrem sa adaugam altul cu 5, el va fi adaugat astfel 5 5 "5" 4 3)
- pentru usurarea taskurilor am creat niste functii ajutatoare:
    - [initList] = initializeaza o lista
    - [destroy_list] = distruge o lista
    - [init_scheduler] = aici setam parametrii schedulerului si ii cream cele doua liste
    mentionate mai sus
    - pthread_join_end_threads] = trece prin toate threadurile create avand grija sa
    astepte terminarea lor si sa le distruga semafoarele
    - [add_list_threads] = adauga threadul in lista "list_threads", dupa cum am spus si
    mai sus, in lista asta mereu se va adauga la final
    - [add_ready_threads] = adauga threadul in lista "ready_threads", in functie de 
    prioritatea lui (regula de adaugare este explicata mai sus)
    - [pop_ready_threads] = scoate primul thread din lista "ready_threads", doar de atat 
    avem nevoie deoarece mereu se va scoate threadul cu prioritatea cea mai mare.
    - [running_next_thread] = realizeaza planificare corecta a threadurilor dupa algoritmul
    Round Robin (mai multe detalii sunt date in cod)
    - [pt_aflare_pid] = ne da contextul de executie a unui thread, e facut dupa modelui 
    start_hread din cerinta
    - [init_thread] = initializeaza threadul, il aloca, ii atibuie toate informatiile, ii 
    initializam semaforul si ii aflam pid-ul folosind pthread_create
- acum o sa explic pe scurt ce am scris si cum am folosit functiile facute de mine in functiile
care au fost date

- [so_init] 
    - daca schedulerul a fost initializat sau quanta e 0 sau numarul de dispozitive
    - depaseste numarul maxim => returnam -1
    - in caz contrar => marcam schedulerul ca fiind initializat si folosim functia "init_scheduler"
pe care am creat-o => returnam 0

- [so_fork]
    - daca handlerul dat e null si prioritatea depaseste prioritatea maxima => returnam -1
    - in caz contrar alocam thredul care va fi intors de functia "init_thread"
    - il adaugam in cele doua liste 
    - daca nu avem thread in RUNNING folosim "running_next_thread" pentru a treceunul dintre 
threaduri in starea asta
    - daca avem unul in RUNNING apelam [so_exec]
    - returnam pid-ul threadului creat

- [so_wait]
    - daca nr de dispozitive i/o nu este valid => returnam -1
    - ducem threadul din RUNNING in WAINTING si ii dam numarul nou de dispozitive i/o
    - apelam [so_exec]
    - returnam 0

- [so_signal]
    - daca numarul de dispozitive i/o nu convine => returnam -1
    - trecem prin toate nodurile si numaram cate threaduri sunt in WAITING si asteapta evenimente,
carora le atribuim evenimentele si le trecem in READY, apoi le adaugam in "ready_threads"
    - returnam numarul de threduri gasit


- [so_exec]
    - scadem quanta threadului din RUNNING
    - verificam cu "running_next_thread" daca ramane in RUNNING sau vine alt thread in locul lui
    - ii trecem semaforul pe wait

- [so_end]
    - daca schedulerul nu a fost initializat => returnam 0
    - apelam "pthread_join_end_lisr_threads" --scrie ce face mai sus
    - distrugem lista cu toate threadurile "list_threads"
    - eliberam lista "ready_threads" (nu este nevoie sa o distrugem pentru ca se presupune ca este
goala atuncti cand am ajuns aici, se presupune ca toate tredurile si-au incheiat executia in
momentul asta)
    - marcam schedulerul ca fiind neinitializat
