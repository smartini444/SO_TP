#ifndef TLB_H_
#define TLB_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/temporal.h>
#include <pthread.h>
#include "utils.h"

typedef struct{
	int nroPagina;
	int nroFrame;
	char* last_use;
} entrada_tlb;

typedef struct {
	t_list* entradas;
	int size; //cantidad de entradas
	char* algoritmo;
	// int retardoAcierto;
	// int retardoFallo;
} tlb;

typedef struct {
	int CANTIDAD_ENTRADAS_TLB;
	char* ALGORITMO_REEMPLAZO_TLB;
	int RETARDO_ACIERTO_TLB;
	int RETARDO_FALLO_TLB;
}t_config_memoria;

//extern tlb* TLB;

tlb* TLB;
int TLBEnable;
pthread_mutex_t mutexTLB;
t_config_memoria* configMemoria;

void iniciar_TLB(t_config_memoria* config);
void actualizar_TLB(int nroPagina,int nroFrame);
int buscar_en_TLB(int nroPagina);
void limpiar_entrada_TLB(int nroPagina, int pid);
void limpiar_entradas_TLB();
void reemplazo_fifo(int nroPagina, int nroFrame);
void reemplazo_lru(int nroPagina, int nroFrame);
void cerrar_TLB();
void destruir_entrada(void* entry);
void logs_reemplazo_entrada(int viejoPag, int viejoFrame, int nuevoPag, int nuevoFrame);


#endif
