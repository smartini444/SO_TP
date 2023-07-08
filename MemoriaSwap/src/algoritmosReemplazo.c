#include "headers/algoritmosReemplazo.h"

int reubicarClock(int pid){
	//tiene que retornar el numero del frame victima a reemplazar.
	t_list* pagsEnRam = list_create();
	list_add_all(pagsEnRam, getPagsEnRam());

	t_filaFrame* frame;
	t_tabla2N* tabla2N;
	t_fila2N* pag2N;
	pthread_mutex_lock(&mutexListaFrames);
	for(int i=0; i<list_size(tablaDeFrames); i++){
		frame = malloc(sizeof(t_filaFrame));
		frame = list_get(tablaDeFrames, i);
		if(frame->idProceso == pid){
			//analizo si la pag que esta en ese frame, tiene bit de uso en 0 o en 1
			pthread_mutex_lock(&mutexLista2N);
				for(int j=0; j<list_size(listaTodasTablas2N); j++){
					tabla2N = malloc(sizeof(tabla2N));
					tabla2N = list_get(listaTodasTablas2N, j);
					for(int k=0; k<list_size(tabla2N->tablaPaginas2N); k++){
						pthread_mutex_lock(&mutexAlgoritmoReemplazo);
							if(posicionClock >= (cantidadFramesTotal-1)){
								posicionClock = 0;
							}
						pthread_mutex_unlock(&mutexAlgoritmoReemplazo);

						pag2N = malloc(sizeof(t_fila2N));
						pag2N = list_get(tabla2N->tablaPaginas2N, k);
						if(pag2N->nroPagina == frame->nroPagina){ //significa que la pagina esta cargada en ese frame, esta en ram.
							if(pag2N->bitDeUso == 0){ //ES LA PAG VICTIMAAA
								pthread_mutex_unlock(&mutexLista2N);
								pthread_mutex_unlock(&mutexListaFrames);
								incrementarPosicionClock();
								return pag2N->nroFrame;
							}
							else{
								pag2N->bitDeUso = 0;
								list_replace(tabla2N->tablaPaginas2N, k, pag2N);
								if(i == list_size(tablaDeFrames)-1){
									//sirve para resetear el for de la lista de frames y dar otra vuelta entera a la lista
									//por si en la primer vuelta no se llego a encontrar una victima
									i = 0;
								}
							}
						}
					}
				}
			pthread_mutex_unlock(&mutexLista2N);
		}
		incrementarPosicionClock();
	}
	pthread_mutex_unlock(&mutexListaFrames);
	return -1;
}

int reubicarClockModificado(int pid){
	//tiene que retornar el numero del frame victima a reemplazar.
	t_filaFrame* frame;
	t_tabla2N* tabla2N;
	t_fila2N* pag2N;
	int nroVuelta = 0;
	pthread_mutex_lock(&mutexListaFrames);
	for(int i=0; i<list_size(tablaDeFrames); i++){
		frame = malloc(sizeof(t_filaFrame));
		frame = list_get(tablaDeFrames, i);
		if(frame->idProceso == pid){
			//analizo si la pag que esta en ese frame, tiene bit de uso en 0 o en 1
			pthread_mutex_lock(&mutexLista2N);
				for(int j=0; j<list_size(listaTodasTablas2N); j++){
					tabla2N = malloc(sizeof(tabla2N));
					tabla2N = list_get(listaTodasTablas2N, j);
					for(int k=0; k<list_size(tabla2N->tablaPaginas2N); k++){
						pag2N = malloc(sizeof(t_fila2N));
						pag2N = list_get(tabla2N->tablaPaginas2N, k);
						if(pag2N->nroPagina == frame->nroPagina){ //significa que la pagina esta cargada en ese frame, esta en ram.
							if(nroVuelta == 1 || nroVuelta == 3){
								if(pag2N->bitDeUso == 0 && pag2N->bitModificado == 0){ //ES LA PAG VICTIMAAA
									pthread_mutex_unlock(&mutexLista2N);
									pthread_mutex_unlock(&mutexListaFrames);
									return pag2N->nroFrame;
								}
								else{
									pag2N->bitDeUso = 0;
									list_replace(tabla2N->tablaPaginas2N, k, pag2N);
									if(i == list_size(tablaDeFrames)-1){
										//sirve para resetear el for de la lista de frames y dar otra vuelta entera a la lista
										//por si en la primer vuelta no se llego a encontrar una victima
										i = 0;
										nroVuelta++;
									}
								}
							}
							if(nroVuelta == 2 || nroVuelta == 4){
								if(pag2N->bitDeUso == 0 && pag2N->bitModificado == 1){ //ES LA PAG VICTIMAAA
									pthread_mutex_unlock(&mutexLista2N);
									pthread_mutex_unlock(&mutexListaFrames);
									return pag2N->nroFrame;
								}
								else{
									pag2N->bitDeUso = 0;
									list_replace(tabla2N->tablaPaginas2N, k, pag2N);
									if(i == list_size(tablaDeFrames)-1){
										//sirve para resetear el for de la lista de frames y dar otra vuelta entera a la lista
										//por si en la primer vuelta no se llego a encontrar una victima
										i = 0;
										nroVuelta++;
									}
								}
							}
						}
					}
				}
			pthread_mutex_unlock(&mutexLista2N);
		}
	}
	pthread_mutex_unlock(&mutexListaFrames);
	return -1;
}


void incrementarPosicionClock(void){
	pthread_mutex_lock(&mutexAlgoritmoReemplazo);
		if(posicionClock >= (cantidadFramesTotal-1)){
			posicionClock = 0;
		}
		else{
			posicionClock++;
		}
	pthread_mutex_unlock(&mutexAlgoritmoReemplazo);
}



