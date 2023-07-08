#ifndef TLB_H_
#define TLB_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/temporal.h>
#include "configMemoria.h"
#include "logs.h"
#include "signals.h"

typedef struct{
	int nroPagina;
	int nroFrame;
} entrada_tlb;

typedef struct tlb{
	t_list* entradas;
	int size; //cantidad de entradas
	char* algoritmo;
	// int retardoAcierto;
	// int retardoFallo;
} tlb;

//extern tlb* TLB;

tlb* TLB;
int TLBEnable;
pthread_mutex_t mutexTLB;


void iniciar_TLB(t_config_memoria* config);
void actualizar_TLB(int nroPagina, int pid, int nroFrame);
int buscar_en_TLB(int nroPagina, int pid);
void limpiar_entrada_TLB(int nroPagina, int pid);
void limpiar_entradas_TLB();
void reemplazo_fifo(int nroPagina, int pid, int nroFrame);
void reemplazo_lru(int nroPagina, int pid, int nroFrame);
void cerrar_TLB();
void destruir_entrada(void* entry);


#endif
