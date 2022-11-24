/*
 * Loader Implementation
 *
 * 2022, Operating Systems
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "exec_parser.h"

#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
static so_exec_t *exec;
static struct sigaction default_handler;

static int fd;


static void segv_handler(int signum, siginfo_t *info, void *context)
{
	/* TODO - actual loader implementation */
	if(signum != SIGSEGV){																					//daca nu se respecta conditia rulam handlerul default(HA)
		default_handler.sa_sigaction(signum,info,context);
		return;
	}else{
		so_seg_t *segment_gasit;
		int seg_gasit=-1;
		int pag_gasita=-1;
		for(int i=0; i<exec->segments_no; i++){																//cautam segmentul in care se afla adresa
			segment_gasit = (exec->segments) + i;															//retinem segmentul curent
			if( segment_gasit->vaddr <= (int)info->si_addr && (int)info->si_addr <= segment_gasit->vaddr + segment_gasit->mem_size){		//am gasit un segment valid
				if(info->si_code != SEGV_MAPERR){															//daca pagina este mapata => rulam HA
					default_handler.sa_sigaction(signum,info,context);
					return;
				}
				uintptr_t pagina;
				seg_gasit=1;
				int dim_pag = getpagesize();																//retinem dimensiunea unei pagini
				int nr_pagini = (segment_gasit->vaddr + segment_gasit->mem_size) / dim_pag;					//calculam cate pagini are segmentul
				for(int pag=0; pag<nr_pagini; pag++){
					pagina = segment_gasit->vaddr + pag*dim_pag;											//retinem adresa de inceput a paginii curente
					if(pagina <= (int)info->si_addr && (int)info->si_addr < pagina + dim_pag){				//daca adresa se afla in pagina, o mapam
						void *return_mmap = mmap((void *)pagina, dim_pag, PERM_R | PERM_W, MAP_FIXED | MAP_SHARED | MAP_ANONYMOUS, -1, 0);
						if(return_mmap == MAP_FAILED){														//daca mmap se termina cu eroare => rulam HA
							default_handler.sa_sigaction(signum,info,context);
							return;
						}
						if(pagina <= segment_gasit->vaddr + segment_gasit->file_size ){						//daca adresa de inceput a paginii sa afla in file_size region facem lseek
																												//pentru a plasa corect cursorul in file descriptor (pregatim citirea)
							if(lseek(fd, segment_gasit->offset + pag*dim_pag, SEEK_SET) < 0){				//verificam daca lseek se termina sau nu cu eroare
								default_handler.sa_sigaction(signum,info,context);							//gasim eroare => rulam HA
								return;
							}
							int cat_citim;																	//retinem cat avem de citit
							if(segment_gasit->file_size - dim_pag*pag < dim_pag)							//daca este ultima pagina, verificam daca e completa sau nu
								cat_citim=segment_gasit->file_size - dim_pag*pag;							//daca file_size zone se termina inainte de terminarea paginii
							else
								cat_citim = dim_pag;														//daca pagina este completa
							read(fd, (void *)return_mmap, cat_citim);										//citim din file descriptor si punem in memorie
						}
						mprotect(return_mmap, dim_pag, segment_gasit->perm);								//atribuim paginii mapate permisiunile pe care le are segmentul
						return;
					}
				}
			}
		}
		if(seg_gasit==0 || pag_gasita == -1){																//daca nu a fost gasit vreun segment sau vreo pagina => rulam HA
			default_handler.sa_sigaction(signum,info,context);
			return;
		}
	}
}

int so_init_loader(void)
{
	int rc;
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = segv_handler;
	sa.sa_flags = SA_SIGINFO;
	rc = sigaction(SIGSEGV, &sa, &default_handler);
	if (rc < 0) {
		perror("sigaction");
		return -1;
	}
	return 0;
}

int so_execute(char *path, char *argv[])
{
	fd=open(path, O_RDONLY);																			//pregatim file descriptorul pentru functia de read

	exec = so_parse_exec(path);
	if (!exec)
		return -1;

	so_start_exec(exec, argv);

	close(fd);

	return -1;
}
