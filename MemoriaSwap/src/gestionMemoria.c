#include "headers/gestionMemoria.h"

int asignarFrame(int pid){ //SE ASIGNA SIEMPREEEE
	int nroFrame;
	pthread_mutex_lock(&mutexListaFrames);
	t_filaFrame* frame;
	for(int i = 0; i < list_size(tablaDeFrames); i++){
		frame = malloc(sizeof(t_filaFrame));
		frame = list_get(tablaDeFrames, i);
		if(frame->estado == NO_OCUPADO){ //en el caso de que encuentre un frame desocupado
			frame->estado = OCUPADO;
			frame->idProceso = pid;
			frame->nroPagina = -1; //SIGNIFICA QUE NO TIENE UNA PAGINA CARGADA PERO ESTA ASIGNADO A UN PROCESO
			nroFrame = frame->nroFrame;
			list_replace(tablaDeFrames, i, frame);
			break;
		}
	}
	pthread_mutex_unlock(&mutexListaFrames);

	return nroFrame;
}

int algoritmoDeReemplazo(int pid){
	if(string_equals_ignore_case(configMemoriaSwap.algoritmoReemplazo, "CLOCK-M")){
		return reubicarClockModificado(pid);
	}
	else if(string_equals_ignore_case(configMemoriaSwap.algoritmoReemplazo, "CLOCK")){
		return reubicarClock(pid);
	}
	else {
		pthread_mutex_lock(&mutexLoggerMemoria);
			log_error(loggerMemoria,"[NAME ALGORITMO] EL NOMBRE DEL ALGORITMO EN CONFIG NO ES \"CLOCK\" NI \"CLOCK-M\". NO SE RECONOCE");
		pthread_mutex_unlock(&mutexLoggerMemoria);
		return -1;
	}
}

void borrarDeRam(int nroFrame){
	pthread_mutex_lock(&mutexVoidMemoriaPrincipal);
		void* frameVacio = malloc(tamanioDeCadaFrame);
		memcpy(memoriaPrincipal+(nroFrame*tamanioDeCadaFrame), frameVacio, tamanioDeCadaFrame);
	pthread_mutex_unlock(&mutexVoidMemoriaPrincipal);
}

void reemplazarEnRam(void* page, int frameAReemplazar, int pid, int frameSwap, t_fila2N* pag2N, t_tabla2N* tabla2, int indice){
	pthread_mutex_lock(&mutexVoidMemoriaPrincipal);
		//reescribo la page a reemplazar, en swap SOLO SI ESTA MODIFICADA
		if(pag2N->bitModificado == 1){ //es una pag modificada, hay que cargarla en swap
			void* aSwap = malloc(tamanioDeCadaFrame);
			memcpy(aSwap, memoriaPrincipal+(frameAReemplazar*tamanioDeCadaFrame), tamanioDeCadaFrame);

			pthread_mutex_lock(&mutexSwap);
				write_frame(pid, frameSwap, aSwap);
			pthread_mutex_unlock(&mutexSwap);

			free(aSwap);
		}
		//reemplazo la page en ram
		memcpy(memoriaPrincipal+(frameAReemplazar*tamanioDeCadaFrame), page, tamanioDeCadaFrame);
	pthread_mutex_unlock(&mutexVoidMemoriaPrincipal);

	//actualizo la tabla de paginas
	pag2N->bitPresencia = 0;
	pag2N->bitDeUso = 0;
	pag2N->bitModificado = 0;
	pag2N->nroFrame = -1;
	list_replace(tabla2->tablaPaginas2N, indice, pag2N);
	pthread_mutex_unlock(&mutexLista2N);

	//actualizo la tabla de frames
	t_filaFrame* marco;
	pthread_mutex_lock(&mutexListaFrames);
	for(int i=0; i<list_size(tablaDeFrames); i++){
		marco = malloc(sizeof(t_filaFrame));
		marco = list_get(tablaDeFrames, i);
		if(marco->nroFrame == frameAReemplazar){
			marco->estado = NO_OCUPADO;
			marco->idProceso = -1;
			marco->nroPagina = -1;
			list_replace(tablaDeFrames, i, marco);
		}
	}
	pthread_mutex_unlock(&mutexListaFrames);
}











