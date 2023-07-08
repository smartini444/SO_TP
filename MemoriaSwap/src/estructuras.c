#include "headers/estructuras.h"

int crearTablasParaUnProceso(int pid, int tamanioProceso){
	int cantPaginasAUsar = tamanioProceso / configMemoriaSwap.tamanioPagina;
	int cantTablasSegundoNivel = calcularCantTablasSegundoNivelParaUnProceso(cantPaginasAUsar);

	t_tabla1N* tabla1N = malloc(sizeof(t_tabla1N));
	tabla1N->tablaPaginas1N = list_create();
	tabla1N->idProceso = pid;

	t_fila1N* fila1;
	t_tabla2N* tabla2N;
	t_fila2N* fila2;

	for(int k = 0; k < configMemoriaSwap.entradasPorTabla; k++){ //tabla 1N
		fila1 = malloc(sizeof(t_fila1N));
		fila1->nroPagina = k;

		if(k < cantTablasSegundoNivel){ //es una fila que tiene que tener asociada una tabla2N
			tabla2N = malloc(sizeof(t_tabla2N));
			tabla2N->idProceso = pid;
			tabla2N->tablaPaginas2N = list_create();
			fila1->nroTabla2N = k;

			for(int j = 0; j < configMemoriaSwap.entradasPorTabla; j++){ //tabla 2N
				fila2 = malloc(sizeof(t_fila2N));
				fila2->nroPagina = j;

				if((configMemoriaSwap.entradasPorTabla * k) + j < cantPaginasAUsar){ //PAG_A_USAR: es una entrada de 2N que se va a usar
					fila2->nroFrame = asignarFrame(pid);
					fila2->bitPresencia = 0;
					fila2->bitDeUso = 0;
					fila2->bitModificado = 0;
					fila2->posicionSwap = j;
					/*
					 * 	si entradasPorTabla = 3, entonces la posicionSwap va a ser 0, 1 y 2.
					 * 	No es lo mismo nroFrame que posicionSwap porque el nroFrame es desde 0 hasta cantFramesTotal
					 * 	y el posicionSwap va desde 0 hasya entradasPorTabla porque es 1 archivo por cada proceso.
					 */
					fila2->tipo = PAG_A_USAR;
				}
				else { //PAG_INUTIL: es una entrada de 2N que no se necesita porque el tamanio del proceso es mas chico
					fila2->nroFrame = -1;
					fila2->bitPresencia = -1;
					fila2->bitDeUso = -1;
					fila2->bitModificado = -1;
					fila2->posicionSwap = -1;
					fila2->posicionSwap = -1;
					fila2->tipo = PAG_INUTIL;
				}

				list_add(tabla2N->tablaPaginas2N, fila2);
			}

			pthread_mutex_lock(&mutexIdTabla2N);
				idTabla2N++;
				tabla2N->idTabla = idTabla2N;
			pthread_mutex_unlock(&mutexIdTabla2N);

			//agrega la tabla2N a la lista global de tablas2N en el indice igual a su id
			pthread_mutex_lock(&mutexLista2N);
				list_add_in_index(listaTodasTablas2N, tabla2N->idTabla, tabla2N);
			pthread_mutex_unlock(&mutexLista2N);

			fila1->nroTabla2N = tabla2N->idTabla;
		}

		else { //es una fila que no tiene que tener asociada una tabla2N
			fila1->nroTabla2N = -1;
		}

		list_add(tabla1N->tablaPaginas1N, fila1);
	}

	pthread_mutex_lock(&mutexIdTabla1N);
		idTabla1N++;
		tabla1N->idTabla = idTabla1N;
	pthread_mutex_unlock(&mutexIdTabla1N);

	pthread_mutex_lock(&mutexLista1N);
		list_add_in_index(listaTodasTablas1N, tabla1N->idTabla, tabla1N);
	pthread_mutex_unlock(&mutexLista1N);

	return cantTablasSegundoNivel;
}

void iniciarEstructuras(void){
	memoriaPrincipal = malloc(configMemoriaSwap.tamanioMemoria);

	if (memoriaPrincipal == NULL) {
		pthread_mutex_lock(&mutexLoggerMemoria);
			log_error(loggerMemoria,"[MEMORIA PRINCIPAL] NO SE PUDO ALOCAR");
		pthread_mutex_unlock(&mutexLoggerMemoria);
		exit(EXIT_FAILURE);
	}

	idTabla1N = 0;
	idTabla2N = 0;
	posicionClock = 0;

	tamanioDeCadaFrame = configMemoriaSwap.tamanioPagina;
	cantidadFramesTotal = configMemoriaSwap.tamanioMemoria / tamanioDeCadaFrame;

	//es lo mismo que el grado de multiprogramacion del kernel
	cantidadProcesosMaximosEnRam = cantidadFramesTotal / configMemoriaSwap.marcosPorProceso;

	iniciarFrames();
}

void iniciarFrames(void){
	//creo los frames struct
	t_filaFrame* frame;
	for(int i = 0; i < cantidadFramesTotal; i++){
		frame = malloc(sizeof(t_filaFrame));
		frame->estado = NO_OCUPADO;
		frame->idProceso = -1; //no tiene proceso asignado, por eso es negativo.
		frame->nroFrame = i;
		frame->nroPagina = -1; //no tiene nro de pagina asignada por eso es -1
		list_add(tablaDeFrames, frame);
	}

	//creo los frames void
	void* frameVoid;
	for(int j=0; j<cantidadFramesTotal;j++){
		frameVoid = malloc(tamanioDeCadaFrame);
		memcpy(memoriaPrincipal+(j*tamanioDeCadaFrame), frameVoid, tamanioDeCadaFrame);
	}
}

void liberarFrame_Tabla(int nroFrame){
	t_filaFrame* frame;
	for(int i=0; i<list_size(tablaDeFrames); i++){
		frame = malloc(sizeof(t_filaFrame));
		frame = list_get(tablaDeFrames, i);
		if(frame->nroFrame == nroFrame){
			frame->estado = NO_OCUPADO;
			frame->idProceso = -1;
			frame->nroPagina = -1;
			list_replace(tablaDeFrames, i, frame);
		}
	}
}


