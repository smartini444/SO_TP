#include "headers/peticiones.h"

void conexionGenerica(void* socketAceptadoVoid){
	int socketAceptado = (int)socketAceptadoVoid;
	t_paquete paquete;

	int pid;
	int pagina;
	int tamanioProceso;
	int idTabla1Nivel;
	int idTabla2Nivel;
	int valorAEscribir;

	t_direccionFisica* dirFisicaOrigen = malloc(sizeof(t_direccionFisica));
	t_direccionFisica* dirFisicaDestino = malloc(sizeof(t_direccionFisica));
	t_direccionFisica* dirFisica = malloc(sizeof(t_direccionFisica));

	MSJ_KERNEL_MEMORIA_READY* infoKernelMemoriaReady = malloc(sizeof(MSJ_KERNEL_MEMORIA_READY));
	MSJ_MEMORIA_CPU_ACCESO_1ERPASO* infoMemoriaCpu1erPaso = malloc(sizeof(MSJ_MEMORIA_CPU_ACCESO_1ERPASO));
	MSJ_MEMORIA_CPU_ACCESO_2DOPASO* infoMemoriaCpu2doPaso = malloc(sizeof(MSJ_MEMORIA_CPU_ACCESO_2DOPASO));

	MSJ_MEMORIA_CPU_WRITE* infoMemoriaCpuWrite = malloc(sizeof(MSJ_MEMORIA_CPU_WRITE));
	infoMemoriaCpuWrite->dirFisica = malloc(sizeof(t_direccionFisica));

	MSJ_MEMORIA_CPU_COPY* infoMemoriaCpuCopy = malloc(sizeof(MSJ_MEMORIA_CPU_COPY));
	infoMemoriaCpuCopy->dirFisicaDestino = malloc(sizeof(t_direccionFisica));
	infoMemoriaCpuCopy->dirFisicaOrigen = malloc(sizeof(t_direccionFisica));

	MSJ_MEMORIA_CPU_READ* infoMemoriaCpuRead = malloc(sizeof(MSJ_MEMORIA_CPU_READ));
	infoMemoriaCpuRead->dirFisica = malloc(sizeof(t_direccionFisica));

	recibirMensaje(socketAceptado, &paquete);

	switch(paquete.header.tipoMensaje) {
		case PASAR_A_READY:
			infoKernelMemoriaReady = paquete.mensaje;
			pid = infoKernelMemoriaReady->pid;
		    tamanioProceso = infoKernelMemoriaReady->tamanioProceso;
			pasarAReady(pid, tamanioProceso, socketAceptado);
			break;
		case SUSPENDER:
			pid = (int)paquete.mensaje;
			suspender(pid);
			break;
		case PASAR_A_EXIT:
			pid = (int)paquete.mensaje;
			pasarAExit(pid);
			break;
		case CONFIG_DIR_LOG_A_FISICA:
			configurarDireccionesCPU(socketAceptado);
			break;
		case TRADUCCION_DIR_PRIMER_PASO:
			infoMemoriaCpu1erPaso = paquete.mensaje;
			idTabla1Nivel = infoMemoriaCpu1erPaso->idTablaPrimerNivel;
			pagina = infoMemoriaCpu1erPaso->pagina;
			primerPasoTraduccionDireccion(idTabla1Nivel, pagina, socketAceptado);
			break;
		case TRADUCCION_DIR_SEGUNDO_PASO:
			infoMemoriaCpu2doPaso = paquete.mensaje;
			idTabla2Nivel = infoMemoriaCpu2doPaso->idTablaSegundoNivel;
			pagina = infoMemoriaCpu2doPaso->pagina;
			segundoPasoTraduccionDireccion(idTabla2Nivel, pagina, socketAceptado);
			break;
		case ACCESO_MEMORIA_READ:
			infoMemoriaCpuRead = paquete.mensaje;
			dirFisica = infoMemoriaCpuRead->dirFisica;
			pid = infoMemoriaCpuRead->pid;
			accesoMemoriaRead(dirFisica, pid, socketAceptado);
			break;
		case ACCESO_MEMORIA_WRITE:
			infoMemoriaCpuWrite = paquete.mensaje;
			dirFisica = infoMemoriaCpuWrite->dirFisica;
			valorAEscribir = infoMemoriaCpuWrite->valorAEscribir;
			pid = infoMemoriaCpuWrite->pid;
			accesoMemoriaWrite(dirFisica, valorAEscribir, pid, socketAceptado);
			break;
		case ACCESO_MEMORIA_COPY:
			infoMemoriaCpuCopy = paquete.mensaje;
			dirFisicaDestino = infoMemoriaCpuCopy->dirFisicaDestino;
			dirFisicaOrigen = infoMemoriaCpuCopy->dirFisicaOrigen;
			pid = infoMemoriaCpuCopy->pid;
			accesoMemoriaCopy(dirFisicaDestino, dirFisicaOrigen, pid, socketAceptado);
			break;
		default: 
			pthread_mutex_lock(&mutexLoggerMemoria);
				log_error(loggerMemoria, "[TIPO DE MENSAJE] NO SE RECONOCE");
			pthread_mutex_unlock(&mutexLoggerMemoria);
	}

	close(socketAceptado);

	free(dirFisicaOrigen);
	free(dirFisicaDestino);
	free(dirFisica);
	free(infoKernelMemoriaReady);
	free(infoMemoriaCpu1erPaso);
	free(infoMemoriaCpu2doPaso);
	free(infoMemoriaCpuWrite->dirFisica);
	free(infoMemoriaCpuWrite);
	free(infoMemoriaCpuCopy->dirFisicaDestino);
	free(infoMemoriaCpuCopy->dirFisicaOrigen);
	free(infoMemoriaCpuCopy);
	free(infoMemoriaCpuRead->dirFisica);
	free(infoMemoriaCpuRead);

}

void pasarAReady(int pid, int tamanioProceso, int socketAceptado){
	pthread_mutex_lock(&mutexLoggerMemoria);
		log_debug(loggerMemoria,"[INIT - PASAR_A_READY] PID: %d", pid);
	pthread_mutex_unlock(&mutexLoggerMemoria);

    int cantPags2N = crearTablasParaUnProceso(pid, tamanioProceso);

    //creo las pags vacias para guardar en swap. no las guardo en ram por carga a demanda
    void* paginasASwap = malloc(tamanioProceso);
    void* pag2N;
    for(int i=0; i<cantPags2N; i++){
    	pag2N = malloc(configMemoriaSwap.tamanioPagina);
    	memcpy(paginasASwap+(i*configMemoriaSwap.tamanioPagina),
    			pag2N,
				configMemoriaSwap.tamanioPagina);
    }

	pthread_mutex_lock(&mutexSwap);
		write_pid_completo(pid, tamanioProceso, paginasASwap);
    pthread_mutex_unlock(&mutexSwap);

    sleep(configMemoriaSwap.retardoSwap);

    int nroTablad1N = buscarNroTabla1N(pid);
    enviarMensaje(socketAceptado, MEMORIA_SWAP, &nroTablad1N, sizeof(int), PASAR_A_READY);

	pthread_mutex_lock(&mutexLoggerMemoria);
		log_debug(loggerMemoria,"[FIN - PASAR_A_READY] PID: %d tNRO TABLA ENVIADO AL KERNEL", pid);
	pthread_mutex_unlock(&mutexLoggerMemoria);

	//TODO: confirmar que esto no rompe
	free(paginasASwap);

}

void suspender(int pid){
	pthread_mutex_lock(&mutexLoggerMemoria);
		log_debug(loggerMemoria,"[INIT - SUSPENDER] PID: %d", pid);
	pthread_mutex_unlock(&mutexLoggerMemoria);

	t_tabla2N* tablaSegundoNivel;
	t_fila2N* pag2N;
	int cantPagASwapear;

	pthread_mutex_lock(&mutexLista2N);
		cantPagASwapear = 0;
		for(int i=0; i<list_size(listaTodasTablas2N); i++){
			tablaSegundoNivel = malloc(sizeof(t_tabla2N));
			tablaSegundoNivel = list_get(listaTodasTablas2N, i);
			if(tablaSegundoNivel->idProceso == pid){    //es una tabla2N de ese pid
				for(int j=0; j<list_size(tablaSegundoNivel->tablaPaginas2N); j++){ //para cada pag2N... etc
					pag2N = malloc(sizeof(t_fila2N));
					pag2N = list_get(tablaSegundoNivel->tablaPaginas2N, j);
					if(pag2N->tipo == PAG_A_USAR){
						pag2N->bitPresencia = 0;
						pag2N->bitDeUso = 0;
						pag2N->bitModificado = 0;
						list_replace(tablaSegundoNivel->tablaPaginas2N, j, pag2N);
						cantPagASwapear++;
					}
				}
			}
		}
		int tamanio = cantPagASwapear*configMemoriaSwap.tamanioPagina;
		void* pagsASwap = malloc(tamanio);
		int cantMemcopiadas = 0;
		for(int k=0; k<list_size(listaTodasTablas2N); k++){
			tablaSegundoNivel = malloc(sizeof(t_tabla2N));
			tablaSegundoNivel = list_get(listaTodasTablas2N, k);
			if(tablaSegundoNivel->idProceso == pid){    //es una tabla2N de ese pid
				for(int w=0; w<list_size(tablaSegundoNivel->tablaPaginas2N); w++){ //para cada pag2N... etc
					pag2N = malloc(sizeof(t_fila2N));
					pag2N = list_get(tablaSegundoNivel->tablaPaginas2N, w);
					if(pag2N->tipo == PAG_A_USAR){
						memcpy(pagsASwap+(cantMemcopiadas*configMemoriaSwap.tamanioPagina),
								memoriaPrincipal+(pag2N->nroFrame*tamanioDeCadaFrame),
								configMemoriaSwap.tamanioPagina);
						borrarDeRam(pag2N->nroFrame);
						cantMemcopiadas++;
					}
				}
			}
		}
		for(int u=0; u<list_size(listaTodasTablas2N); u++){
			tablaSegundoNivel = malloc(sizeof(t_tabla2N));
			tablaSegundoNivel = list_get(listaTodasTablas2N, u);
			if(tablaSegundoNivel->idProceso == pid){    //es una tabla2N de ese pid
				for(int r=0; r<list_size(tablaSegundoNivel->tablaPaginas2N); r++){ //para cada pag2N... etc
					pag2N = malloc(sizeof(t_fila2N));
					pag2N = list_get(tablaSegundoNivel->tablaPaginas2N, r);
					if(pag2N->tipo == PAG_A_USAR){
						liberarFrame_Tabla(pag2N->nroFrame);
						pag2N->nroFrame = -1;
						list_replace(tablaSegundoNivel->tablaPaginas2N, r, pag2N);
					}
				}
			}
		}
	pthread_mutex_unlock(&mutexLista2N);

	if(cantMemcopiadas == cantPagASwapear){
		pthread_mutex_lock(&mutexLoggerMemoria);
			log_debug(loggerMemoria,"[SUSPENDER] SE MEMCOPIO TODO BIEN. PID: %d" , pid);
		pthread_mutex_unlock(&mutexLoggerMemoria);
	}
	else {
		pthread_mutex_lock(&mutexLoggerMemoria);
			log_warning(loggerMemoria,"[SUSPENDER] OJO CON EL MEMCOPEO, NO COINCIDEN LA CANT DE PAGES. PID: %d" , pid);
		pthread_mutex_unlock(&mutexLoggerMemoria);
	}
	pthread_mutex_lock(&mutexSwap);
		replace_pid_suspendido(pid, tamanio, pagsASwap);
    pthread_mutex_unlock(&mutexSwap);

    sleep(configMemoriaSwap.retardoSwap);

	pthread_mutex_lock(&mutexLoggerMemoria);
		log_debug(loggerMemoria,"[FIN - SUSPENDER] PID: %d" , pid);
	pthread_mutex_unlock(&mutexLoggerMemoria);
}

void pasarAExit(int pid){
	pthread_mutex_lock(&mutexLoggerMemoria);
		log_debug(loggerMemoria,"[INIT - PASAR_A_EXIT] PID: %d", pid);
	pthread_mutex_unlock(&mutexLoggerMemoria);

	t_tabla2N* tablaSegundoNivel;
	t_fila2N* pag2N;

	pthread_mutex_lock(&mutexLista2N);
		for(int i=0; i<list_size(listaTodasTablas2N); i++){
			tablaSegundoNivel = malloc(sizeof(t_tabla2N));
			tablaSegundoNivel = list_get(listaTodasTablas2N, i);
			if(tablaSegundoNivel->idProceso == pid){    //es una tabla2N de ese pid
				for(int j=0; j<list_size(tablaSegundoNivel->tablaPaginas2N); j++){ //para cada pag2N... etc
					pag2N = malloc(sizeof(t_fila2N));
					pag2N = list_get(tablaSegundoNivel->tablaPaginas2N, j);
					if(pag2N->tipo == PAG_A_USAR){
						pag2N->bitPresencia = 0;
						pag2N->bitDeUso = 0;
						pag2N->bitModificado = 0;
						liberarFrame_Tabla(pag2N->nroFrame);
						pag2N->nroFrame = -1;
						list_replace(tablaSegundoNivel->tablaPaginas2N, j, pag2N);
						borrarDeRam(pag2N->nroFrame);
					}
				}
			}
		}
	pthread_mutex_unlock(&mutexLista2N);

	pthread_mutex_lock(&mutexSwap);
    	delete_in_swap(pid);
    pthread_mutex_unlock(&mutexSwap);

    sleep(configMemoriaSwap.retardoSwap);

	pthread_mutex_lock(&mutexLoggerMemoria);
		log_debug(loggerMemoria,"[FIN - PASAR_A_EXIT] PID: %d" , pid);
	pthread_mutex_unlock(&mutexLoggerMemoria);
}

void configurarDireccionesCPU(int socketAceptado){
	//enviar ENTRADAS_POR_TABLA y TAM_PAGINA
	pthread_mutex_lock(&mutexLoggerMemoria);
		log_debug(loggerMemoria,"[INIT - CONFIG_DIR_LOG_A_FISICA]");
	pthread_mutex_unlock(&mutexLoggerMemoria);

	MSJ_MEMORIA_CPU_INIT* infoAcpu = malloc(sizeof(MSJ_MEMORIA_CPU_INIT));
	infoAcpu->cantEntradasPorTabla = configMemoriaSwap.entradasPorTabla;
	infoAcpu->tamanioPagina = configMemoriaSwap.tamanioPagina;

	sleep(configMemoriaSwap.retardoMemoria);
	enviarMensaje(socketAceptado, MEMORIA_SWAP, infoAcpu, sizeof(MSJ_MEMORIA_CPU_INIT), CONFIG_DIR_LOG_A_FISICA);
	free(infoAcpu);

	pthread_mutex_lock(&mutexLoggerMemoria);
		log_debug(loggerMemoria,"[FIN - CONFIG_DIR_LOG_A_FISICA] INFO DE CANT ENTRADAS POR TABLA Y TAMANIO PAGINA ENVIADO A CPU");
	pthread_mutex_unlock(&mutexLoggerMemoria);
}

void primerPasoTraduccionDireccion(int idTabla1Nivel, int pagina, int socketAceptado){
	//CPU PREGUNTA CUAL ES LA TABLA DE SEGUNDO NIVEL DE ESA PAGINA EN ESA TABLA1N
	pthread_mutex_lock(&mutexLoggerMemoria);
		log_debug(loggerMemoria,"[INIT - TRADUCCION_DIR_PRIMER_PASO] ID_TABLA1N: %d NRO_PAGINA: %d", idTabla1Nivel, pagina);
	pthread_mutex_unlock(&mutexLoggerMemoria);

	t_tabla1N* tabla1 = malloc(sizeof(t_tabla1N));
	tabla1 = buscarTabla1N(idTabla1Nivel);
	int idTabla2NSolicitada = buscarIdTabla2NEnUnaLista(tabla1->tablaPaginas1N, pagina);

	sleep(configMemoriaSwap.retardoMemoria);
	enviarMensaje(socketAceptado, MEMORIA_SWAP, &idTabla2NSolicitada, sizeof(int), TRADUCCION_DIR_PRIMER_PASO);

	pthread_mutex_lock(&mutexLoggerMemoria);
		log_debug(loggerMemoria,"[FIN - TRADUCCION_DIR_PRIMER_PASO] ID TABLA 2DO NIVEL SOBRE PAGINA: %d DE TABLA 1ER NIVEL: %d ENVIADA A LA CPU",
				pagina, idTabla1Nivel);
	pthread_mutex_unlock(&mutexLoggerMemoria);
}

void segundoPasoTraduccionDireccion(int idTabla2Nivel, int pagina, int socketAceptado){
	//CPU PREGUNTA CUAL ES EL FRAME DONDE ESTA LA PAGINA DE ESA TABLA DE 2N
	pthread_mutex_lock(&mutexLoggerMemoria);
		log_debug(loggerMemoria,"[INIT - TRADUCCION_DIR_SEGUNDO_PASO] ID_TABLA2N: %d NRO_PAGINA: %d",
				idTabla2Nivel, pagina);
	pthread_mutex_unlock(&mutexLoggerMemoria);

	int frameBuscado;
	t_fila2N* pag2N;
	t_tabla2N* tabla2N;
	int indice;

	//busco la pagina que piden
	pthread_mutex_lock(&mutexLista2N);
		for(int i=0; i<list_size(listaTodasTablas2N); i++){
			tabla2N = malloc(sizeof(tabla2N));
			tabla2N = list_get(listaTodasTablas2N, i);
			if(tabla2N->idTabla == idTabla2Nivel){
				for(int j=0; j<list_size(tabla2N->tablaPaginas2N); j++){
					pag2N = malloc(sizeof(t_fila2N));
					pag2N = list_get(tabla2N->tablaPaginas2N, j);
					if(pag2N->nroPagina == pagina){
						indice = j;
						break;
					}
				}
			}
		}
	pthread_mutex_unlock(&mutexLista2N);

	//analizo si esta en ram
	if(pag2N->bitPresencia == 1 && pag2N->nroFrame!=-1){ //la pag esta cargada en ram
		pthread_mutex_lock(&mutexLista2N);
			//TODO: ver si es necesario este mutex o si hay q poner otro o que
			pag2N->bitDeUso = 1;
			list_replace(tabla2N->tablaPaginas2N, indice, pag2N);
		pthread_mutex_unlock(&mutexLista2N);
		frameBuscado = pag2N->nroFrame;
	}
	else{ //la pag no esta en ram. hay que acceder a swap.
		//1) verificar si hay frames disponibles para ese proceso
		void* aReemplazar = malloc(configMemoriaSwap.tamanioPagina);
		int frameAReemplazar = hayFrameLibreParaElPid(tabla2N->idProceso); //-1: no hay. !=-1:nroFrame a reemplazar

		if(frameAReemplazar != -1){
			//2) si hay 1 frame disponible: pedir a swap la pagina, actualizar la pag a reemplazar en swap y cargar la nueva
			pthread_mutex_lock(&mutexSwap);
				memcpy(aReemplazar, read_in_swap(tabla2N->idProceso, pag2N->posicionSwap, 0), tamanioDeCadaFrame);
			pthread_mutex_unlock(&mutexSwap);
			sleep(configMemoriaSwap.retardoSwap);
			reemplazarEnRam(aReemplazar, frameAReemplazar, tabla2N->idProceso, pag2N->posicionSwap, pag2N, tabla2N, indice);
			frameBuscado = frameAReemplazar;
		}
		else{
			//3) si no hay frames: arrancar algoritmo de reemplazo para buscar la victima
			//4) reemplazar victima por pagina nueva.
			frameAReemplazar = algoritmoDeReemplazo(tabla2N->idProceso);
			pthread_mutex_lock(&mutexSwap);
				memcpy(aReemplazar, read_in_swap(tabla2N->idProceso, pag2N->posicionSwap, 0), tamanioDeCadaFrame);
			pthread_mutex_unlock(&mutexSwap);
			sleep(configMemoriaSwap.retardoSwap);
			reemplazarEnRam(aReemplazar, frameAReemplazar, tabla2N->idProceso, pag2N->posicionSwap, pag2N, tabla2N, indice);
			frameBuscado = frameAReemplazar;
		}
	}

	sleep(configMemoriaSwap.retardoMemoria);
	enviarMensaje(socketAceptado, MEMORIA_SWAP, &frameBuscado, sizeof(int), TRADUCCION_DIR_SEGUNDO_PASO);

	pthread_mutex_lock(&mutexLoggerMemoria);
		log_debug(loggerMemoria,"[FIN - TRADUCCION_DIR_SEGUNDO_PASO] FRAME DE LA PAGINA: %d DE TABLA 2DO NIVEL: %d ENVIADO A CPU",
				pagina, idTabla2Nivel);
	pthread_mutex_unlock(&mutexLoggerMemoria);
}

void accesoMemoriaRead(t_direccionFisica* dirFisica, int pid, int socketAceptado){
	pthread_mutex_lock(&mutexLoggerMemoria);
		log_debug(loggerMemoria,"[INIT - ACCESO_MEMORIA_READ] DIR_FISICA: %d%d",
				dirFisica->nroMarco, dirFisica->desplazamiento);
	pthread_mutex_unlock(&mutexLoggerMemoria);

	int nroFrame = dirFisica->nroMarco;
	int offset = dirFisica->desplazamiento;
	void* aLeer = malloc(tamanioDeCadaFrame);
	int valorLeido;

	//valido que el offset sea valido
	if(offset>tamanioDeCadaFrame){
		sleep(configMemoriaSwap.retardoMemoria);
		enviarMensaje(socketAceptado, MEMORIA_SWAP, "ERROR_DESPLAZAMIENTO", strlen("ERROR_DESPLAZAMIENTO")+1, ACCESO_MEMORIA_READ);

		pthread_mutex_lock(&mutexLoggerMemoria);
			log_error(loggerMemoria,"[ACCESO_MEMORIA_READ] OFFSET MAYOR AL TAMANIO DEL FRAME.  DIR_FISICA: %d%d",
					dirFisica->nroMarco, dirFisica->desplazamiento);
		pthread_mutex_unlock(&mutexLoggerMemoria);
		return;
	}

	//valido que el nro frame sea valido
	if(nroFrame>cantidadFramesTotal){
		sleep(configMemoriaSwap.retardoMemoria);
		enviarMensaje(socketAceptado, MEMORIA_SWAP, "ERROR_NRO_FRAME", strlen("ERROR_NRO_FRAME")+1, ACCESO_MEMORIA_READ);

		pthread_mutex_lock(&mutexLoggerMemoria);
			log_error(loggerMemoria,"[ACCESO_MEMORIA_READ] NRO DE FRAME INEXISTENTE.  DIR_FISICA: %d%d",
					dirFisica->nroMarco, dirFisica->desplazamiento);
		pthread_mutex_unlock(&mutexLoggerMemoria);
		return;
	}

	pthread_mutex_lock(&mutexVoidMemoriaPrincipal);
		memcpy(aLeer, memoriaPrincipal+(nroFrame*tamanioDeCadaFrame)+offset, tamanioDeCadaFrame-offset);
	pthread_mutex_unlock(&mutexVoidMemoriaPrincipal);
	valorLeido = atoi((char*)aLeer);

	sleep(configMemoriaSwap.retardoMemoria);
	enviarMensaje(socketAceptado, MEMORIA_SWAP, &valorLeido, sizeof(int), ACCESO_MEMORIA_READ);

	pthread_mutex_lock(&mutexLoggerMemoria);
		log_debug(loggerMemoria,"[FIN - ACCESO_MEMORIA_READ] DIR_FISICA: %d%d",
				dirFisica->nroMarco, dirFisica->desplazamiento);
	pthread_mutex_unlock(&mutexLoggerMemoria);
}

void accesoMemoriaWrite(t_direccionFisica* dirFisica, int valorAEscribir, int pid, int socketAceptado){
	pthread_mutex_lock(&mutexLoggerMemoria);
		log_debug(loggerMemoria,"[INIT - ACCESO_MEMORIA_WRITE] DIR_FISICA: %d%d, VALOR: %d",
				dirFisica->nroMarco, dirFisica->desplazamiento, valorAEscribir);
	pthread_mutex_unlock(&mutexLoggerMemoria);

	int nroFrame = dirFisica->nroMarco;
	int offset = dirFisica->desplazamiento;

	//valido que el offset sea valido
	if(offset>tamanioDeCadaFrame){
		sleep(configMemoriaSwap.retardoMemoria);
		enviarMensaje(socketAceptado, MEMORIA_SWAP, "ERROR_DESPLAZAMIENTO", strlen("ERROR_DESPLAZAMIENTO")+1, ACCESO_MEMORIA_WRITE);

		pthread_mutex_lock(&mutexLoggerMemoria);
			log_error(loggerMemoria,"[ACCESO_MEMORIA_WRITE] OFFSET MAYOR AL TAMANIO DEL FRAME.  DIR_FISICA: %d%d, VALOR: %d",
					dirFisica->nroMarco, dirFisica->desplazamiento, valorAEscribir);
		pthread_mutex_unlock(&mutexLoggerMemoria);
		return;
	}

	//valido que el nro frame sea valido
	if(nroFrame>cantidadFramesTotal){
		sleep(configMemoriaSwap.retardoMemoria);
		enviarMensaje(socketAceptado, MEMORIA_SWAP, "ERROR_NRO_FRAME", strlen("ERROR_NRO_FRAME")+1, ACCESO_MEMORIA_WRITE);

		pthread_mutex_lock(&mutexLoggerMemoria);
			log_error(loggerMemoria,"[ACCESO_MEMORIA_WRITE] NRO DE FRAME INEXISTENTE.  DIR_FISICA: %d%d, VALOR: %d",
					dirFisica->nroMarco, dirFisica->desplazamiento, valorAEscribir);
		pthread_mutex_unlock(&mutexLoggerMemoria);
		return;
	}

	int tamanio = tamanioDeCadaFrame-offset;
	void* aEscribir = malloc(tamanio);
	memcpy(aEscribir, string_itoa(valorAEscribir), tamanio);
	t_tabla2N* tabla2N;
	t_fila2N* pag2N;

	pthread_mutex_lock(&mutexVoidMemoriaPrincipal);
		memcpy(memoriaPrincipal+(nroFrame*tamanioDeCadaFrame)+offset, aEscribir, tamanioDeCadaFrame-offset);
	pthread_mutex_unlock(&mutexVoidMemoriaPrincipal);

	//busco la pagina que piden y actualizo el bit de modificado porque se hizo write
	pthread_mutex_lock(&mutexLista2N);
		for(int i=0; i<list_size(listaTodasTablas2N); i++){
			tabla2N = malloc(sizeof(tabla2N));
			tabla2N = list_get(listaTodasTablas2N, i);
			for(int j=0; j<list_size(tabla2N->tablaPaginas2N); j++){
				pag2N = malloc(sizeof(t_fila2N));
				pag2N = list_get(tabla2N->tablaPaginas2N, j);
				if(pag2N->nroFrame == nroFrame){
					pag2N->bitModificado = 1;
					list_replace(tabla2N->tablaPaginas2N, j, pag2N);
					break;
				}
			}
		}
	pthread_mutex_unlock(&mutexLista2N);

	sleep(configMemoriaSwap.retardoMemoria);
	enviarMensaje(socketAceptado, MEMORIA_SWAP, "OK", strlen("OK")+1, ACCESO_MEMORIA_WRITE);

	pthread_mutex_lock(&mutexLoggerMemoria);
		log_debug(loggerMemoria,"[FIN - ACCESO_MEMORIA_WRITE] DIR_FISICA: %d%d, VALOR: %d",
				dirFisica->nroMarco, dirFisica->desplazamiento, valorAEscribir);
	pthread_mutex_unlock(&mutexLoggerMemoria);
}

void accesoMemoriaCopy(t_direccionFisica* dirFisicaDestino, t_direccionFisica* dirFisicaOrigen, int pid, int socketAceptado){
	pthread_mutex_lock(&mutexLoggerMemoria);
		log_debug(loggerMemoria,"[INIT - ACCESO_MEMORIA_COPY] DIR_FISICA_DEST: %d%d, DIR_FISICA_ORIGEN: %d%d",
				dirFisicaDestino->nroMarco, dirFisicaDestino->desplazamiento,
				dirFisicaOrigen->nroMarco, dirFisicaOrigen->desplazamiento);
	pthread_mutex_unlock(&mutexLoggerMemoria);

	int origen_nroFrame = dirFisicaOrigen->nroMarco;
	int origen_offset = dirFisicaOrigen->desplazamiento;
	int dest_nroFrame = dirFisicaDestino->nroMarco;
	int dest_offset = dirFisicaDestino->desplazamiento;

	//valido que los offset sean validos
	if(origen_offset>tamanioDeCadaFrame){
		sleep(configMemoriaSwap.retardoMemoria);
		enviarMensaje(socketAceptado, MEMORIA_SWAP, "ERROR_DESPLAZAMIENTO_ORIGEN",
				strlen("ERROR_DESPLAZAMIENTO_ORIGEN")+1, ACCESO_MEMORIA_COPY);

		pthread_mutex_lock(&mutexLoggerMemoria);
			log_error(loggerMemoria,"[ACCESO_MEMORIA_COPY] OFFSET_ORIGEN MAYOR AL TAMANIO DEL FRAME.  DIR_FISICA: %d%d",
					dirFisicaOrigen->nroMarco, dirFisicaOrigen->desplazamiento);
		pthread_mutex_unlock(&mutexLoggerMemoria);
		return;
	}
	if(dest_offset>tamanioDeCadaFrame){
		sleep(configMemoriaSwap.retardoMemoria);
		enviarMensaje(socketAceptado, MEMORIA_SWAP, "ERROR_DESPLAZAMIENTO_DESTINO",
				strlen("ERROR_DESPLAZAMIENTO_DESTINO")+1, ACCESO_MEMORIA_COPY);

		pthread_mutex_lock(&mutexLoggerMemoria);
			log_error(loggerMemoria,"[ACCESO_MEMORIA_COPY] OFFSET_DESTINO MAYOR AL TAMANIO DEL FRAME.  DIR_FISICA: %d%d",
					dirFisicaDestino->nroMarco, dirFisicaDestino->desplazamiento);
		pthread_mutex_unlock(&mutexLoggerMemoria);
		return;
	}

	//valido que los nros de frame sean validos
	if(origen_nroFrame>cantidadFramesTotal){
		sleep(configMemoriaSwap.retardoMemoria);
		enviarMensaje(socketAceptado, MEMORIA_SWAP, "ERROR_NRO_FRAME_ORIGEN", strlen("ERROR_NRO_FRAME_ORIGEN")+1, ACCESO_MEMORIA_COPY);

		pthread_mutex_lock(&mutexLoggerMemoria);
			log_error(loggerMemoria,"[ACCESO_MEMORIA_COPY] NRO DE FRAME ORIGEN INEXISTENTE.  DIR_FISICA: %d%d",
					dirFisicaOrigen->nroMarco, dirFisicaOrigen->desplazamiento);
		pthread_mutex_unlock(&mutexLoggerMemoria);
		return;
	}
	if(dest_nroFrame>cantidadFramesTotal){
		sleep(configMemoriaSwap.retardoMemoria);
		enviarMensaje(socketAceptado, MEMORIA_SWAP, "ERROR_NRO_FRAME_DESTINO", strlen("ERROR_NRO_FRAME_DESTINO")+1, ACCESO_MEMORIA_COPY);

		pthread_mutex_lock(&mutexLoggerMemoria);
			log_error(loggerMemoria,"[ACCESO_MEMORIA_COPY] NRO DE FRAME DESTINO INEXISTENTE.  DIR_FISICA: %d%d",
					dirFisicaDestino->nroMarco, dirFisicaDestino->desplazamiento);
		pthread_mutex_unlock(&mutexLoggerMemoria);
		return;
	}

	int tamanioAEscribir = tamanioDeCadaFrame-origen_offset;
	t_tabla2N* tabla2N;
	t_fila2N* pag2N;

	pthread_mutex_lock(&mutexVoidMemoriaPrincipal);
		memcpy(memoriaPrincipal+(dest_nroFrame+tamanioDeCadaFrame)+dest_offset,
				memoriaPrincipal+(origen_nroFrame+tamanioDeCadaFrame)+origen_offset,
				tamanioAEscribir);
	pthread_mutex_unlock(&mutexVoidMemoriaPrincipal);

	//busco la pagina que piden y actualizo el bit de modificado porque se hizo write
	pthread_mutex_lock(&mutexLista2N);
		for(int i=0; i<list_size(listaTodasTablas2N); i++){
			tabla2N = malloc(sizeof(tabla2N));
			tabla2N = list_get(listaTodasTablas2N, i);
			for(int j=0; j<list_size(tabla2N->tablaPaginas2N); j++){
				pag2N = malloc(sizeof(t_fila2N));
				pag2N = list_get(tabla2N->tablaPaginas2N, j);
				if(pag2N->nroFrame == dest_nroFrame){
					pag2N->bitModificado = 1;
					list_replace(tabla2N->tablaPaginas2N, j, pag2N);
					break;
				}
			}
		}
	pthread_mutex_unlock(&mutexLista2N);

	sleep(configMemoriaSwap.retardoMemoria);
	enviarMensaje(socketAceptado, MEMORIA_SWAP, "OK", strlen("OK")+1, ACCESO_MEMORIA_COPY);

	pthread_mutex_lock(&mutexLoggerMemoria);
		log_debug(loggerMemoria,"[FIN - ACCESO_MEMORIA_COPY] DIR_FISICA_DEST: %d%d, DIR_FISICA_ORIGEN: %d%d",
				dirFisicaDestino->nroMarco, dirFisicaDestino->desplazamiento,
				dirFisicaOrigen->nroMarco, dirFisicaOrigen->desplazamiento);
	pthread_mutex_unlock(&mutexLoggerMemoria);
}






