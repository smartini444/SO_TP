#include "headers/utils.h"

t_list* getPagsEnRam(void){
	t_list* lista = list_create();
	int pid = 0;
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
						pag2N = malloc(sizeof(t_fila2N));
						pag2N = list_get(tabla2N->tablaPaginas2N, k);
						if(pag2N->nroPagina == frame->nroPagina){ //significa que la pagina esta cargada en ese frame, esta en ram.
							if(pag2N->bitDeUso == 0){ //ES LA PAG VICTIMAAA
								pthread_mutex_unlock(&mutexLista2N);
								pthread_mutex_unlock(&mutexListaFrames);
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

	return lista;
}


bool buscarProcesoEnTabla(int pid){
	pthread_mutex_lock(&mutexLista1N);
	t_tabla1N* tabla1Naux;
	for(int i = 0; i< list_size(listaTodasTablas1N); i++){
		tabla1Naux = malloc(sizeof(t_tabla1N));
		tabla1Naux = list_get(listaTodasTablas1N, i);
		if(tabla1Naux->idProceso == pid){
			pthread_mutex_unlock(&mutexLista1N);
			return true;
		}
	}
	pthread_mutex_unlock(&mutexLista1N);
	return false;
}

int calcularCantTablasSegundoNivelParaUnProceso(int cantPaginasAUsar) {
	int cantPaginasAUsarAux = cantPaginasAUsar;
	int cantTablasSegundoNivel = 0;
	while(cantPaginasAUsarAux >= configMemoriaSwap.entradasPorTabla){
		cantTablasSegundoNivel++;
		cantPaginasAUsarAux = cantPaginasAUsarAux - configMemoriaSwap.entradasPorTabla;
	}
	if(cantPaginasAUsarAux != 0) {
		cantTablasSegundoNivel++;
	}
	return cantTablasSegundoNivel;
}

int buscarNroTabla1N(int pid){
	pthread_mutex_lock(&mutexLista1N);
	t_tabla1N* tabla1Naux;
	for(int i = 0; i< list_size(listaTodasTablas1N); i++){
		tabla1Naux = malloc(sizeof(t_tabla1N));
		tabla1Naux = list_get(listaTodasTablas1N, i);
		if(tabla1Naux->idProceso == pid){
			int nroTabla1N = tabla1Naux->idTabla;
			pthread_mutex_unlock(&mutexLista1N);
			return nroTabla1N;
		}
	}
	pthread_mutex_unlock(&mutexLista1N);
	return -1; //no encontro el nro de la tabla de 1N
}

t_tabla1N* buscarTabla1N(int idTabla){
	pthread_mutex_lock(&mutexLista1N);
	t_tabla1N* tabla1Naux;
	for(int i = 0; i< list_size(listaTodasTablas1N); i++){
		tabla1Naux = malloc(sizeof(t_tabla1N));
		tabla1Naux = list_get(listaTodasTablas1N, i);
		if(tabla1Naux->idTabla == idTabla){
			pthread_mutex_unlock(&mutexLista1N);
			return tabla1Naux;
		}
	}
	pthread_mutex_unlock(&mutexLista1N);
	return NULL; //no encontro la tabla1N
}

int buscarIdTabla2NEnUnaLista(t_list* tablaPaginas1N, int pagina){
	t_fila1N* fila1;
	for(int i=0; i< list_size(tablaPaginas1N); i++){
		fila1 = malloc(sizeof(t_fila1N));
		fila1 = list_get(tablaPaginas1N, i);
		if(fila1->nroPagina == pagina){
			int nroTabla2N = fila1->nroTabla2N;
			return nroTabla2N;
		}
	}
	return -1; //no encontro el idTabla2 para esa lista de filas1N
}

t_tabla2N* buscarTabla2N(int idTabla){
	pthread_mutex_lock(&mutexLista2N);
	t_tabla2N* tabla2Naux;
	for(int i = 0; i< list_size(listaTodasTablas2N); i++){
		tabla2Naux = malloc(sizeof(t_tabla2N));
		tabla2Naux = list_get(listaTodasTablas2N, i);
		if(tabla2Naux->idTabla == idTabla){
			pthread_mutex_unlock(&mutexLista2N);
			return tabla2Naux;
		}
	}
	pthread_mutex_unlock(&mutexLista2N);
	return NULL; //no encontro la tabla2N
}

int buscarFrameEnFilaEnListaConPagina(int pagina, t_list* tablaPaginas2Nivel){ // @suppress("No return")
	t_fila2N* fila2;
	for(int i=0; i<list_size(tablaPaginas2Nivel); i++){
		fila2 = malloc(sizeof(t_fila2N));
		fila2 = list_get(tablaPaginas2Nivel, i);
		if(fila2->nroPagina == pagina){
			int frameBuscado = fila2->nroFrame;
			return frameBuscado;
		}
	}
	return -1;
}

int estaEnRam(int nroFrame){
	pthread_mutex_lock(&mutexListaFrames);
	t_filaFrame* frame = malloc(sizeof(t_filaFrame));
	for(int i=0; i<list_size(tablaDeFrames); i++){
		frame = list_get(tablaDeFrames, i);
		if(frame->nroFrame == nroFrame){
			if((frame->estado == NO_OCUPADO) && (frame->nroPagina = -1)){
				pthread_mutex_unlock(&mutexListaFrames);
				return 1; //ESTA EN RAM
			}
			pthread_mutex_unlock(&mutexListaFrames);
			return 0; //NO ESTA EN RAM
		}
	}
	pthread_mutex_unlock(&mutexListaFrames);
	return 0; //NO ESTA EN RAM
}

int hayFrameLibreParaElPid(int pid){
	pthread_mutex_lock(&mutexListaFrames);
	t_filaFrame* frame = malloc(sizeof(t_filaFrame));
	for(int i=0; i<list_size(tablaDeFrames); i++){
		frame = list_get(tablaDeFrames, i);
		if((frame->idProceso == pid) && (frame->estado == NO_OCUPADO) && (frame->nroPagina = -1)){
			pthread_mutex_unlock(&mutexListaFrames);
			return frame->nroFrame; //ES UN FRAME LIBRE
		}
	}
	pthread_mutex_unlock(&mutexListaFrames);
	return -1; //NO HAY FRAMES  LIBRES PARA ESE PID

}



