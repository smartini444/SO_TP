#include "headers/Kernel.h"

int main(void) {
	initKernel();

	iniciar_planif_largo_plazo();
	iniciar_planif_corto_plazo();
	iniciar_planif_io();
	iniciar_mensajes_cpu();
	consolaMultihilo();

	signal(SIGINT,  &cerrarKernel);
}

void consolaMultihilo() {
	log_debug(loggerKernel,"consolaMultihilo");
	int socketConsola = iniciarServidor(configKernel.puertoEscucha);
	struct sockaddr_in dir_cliente;
	socklen_t tam_direccion = sizeof(struct sockaddr_in);

	while(1) {
		log_debug(loggerKernel,"entro al while consolaMultihilo");
		pthread_t hilo_atender_consola;
		int* socketAceptadoConsola =  malloc(sizeof(int)); //TODO: free a esto
		*socketAceptadoConsola = accept(socketConsola, (void*)&dir_cliente, &tam_direccion);
		log_debug(loggerKernel,"Esperando mensaje de consola");

		t_paquete paqueteConsola;
		recibirMensaje(*socketAceptadoConsola, &paqueteConsola);
		log_info(loggerKernel,"RECIBI EL MENSAJE: %s\n", (char*)paqueteConsola.mensaje);

		args_pcb arguments;
		arguments.mensaje = (char*)paqueteConsola.mensaje;
		arguments.socket_consola = *socketAceptadoConsola;

		log_info(loggerKernel,"socketAceptadoConsola %i", *socketAceptadoConsola);

		pthread_create(&hilo_atender_consola, NULL , (void*)crearPCB, (void*)&arguments);
		pthread_detach(hilo_atender_consola);

	}
	log_error(loggerKernel,"Muere hilo multiconsolas");
}

int enviarMensajeMemoriaSwapReady(MSJ_KERNEL_MEMORIA_READY* mensaje) {
	//conexion de cliente con MemoriaSwap, envia y responde

	int tamanioMensajeMemoriaSwap = sizeof(MSJ_KERNEL_MEMORIA_READY);

	enviarMensaje(socketMemoriaSwap, KERNEL, mensaje, tamanioMensajeMemoriaSwap, PASAR_A_READY);
	log_info(loggerKernel, "/////////////////////Envie el mensaje a la memoria swap\n");

	t_paquete paqueteMemoriaSwap;
	recibirMensaje(socketMemoriaSwap, &paqueteMemoriaSwap);

	log_info(loggerKernel, "////////////////////Recibi este mensaje: %s", (char*)paqueteMemoriaSwap.mensaje);
	return (int)paqueteMemoriaSwap.mensaje;
}

void crearPCB(void* arguments) {
	args_pcb *args = (args_pcb*) arguments;

	list_add(LISTA_SOCKETS, (void*)(args->socket_consola));
	log_error(loggerKernel, "socket guardado %i", args->socket_consola);

	log_debug(loggerKernel, "creando PCB");
	char** mensajesplit = string_split(args->mensaje, "\n");
	//int len =strlen(args->mensaje) +1;

	PCB* nuevoProceso = malloc(sizeof(PCB));
	nuevoProceso->instrucciones = strdup(args->mensaje);
	nuevoProceso->ins_length = strlen(args->mensaje) +1;


	pthread_mutex_lock(&mutex_creacion_ID);
	nuevoProceso->id = ID_PROXIMO;
	ID_PROXIMO++;
	pthread_mutex_unlock(&mutex_creacion_ID);


	nuevoProceso->tamanio =  atoi(mensajesplit[0]);
	nuevoProceso->programCounter = 0;
	nuevoProceso->estimacion_anterior = configKernel.estimacionInicial;
	nuevoProceso->real_anterior = 0;
	nuevoProceso->tablaPag = 0;

	t_list* ins = leerPseudocodigo(args->mensaje);
	int cant = list_size(ins);
	instruccion_x_pcb* nuevo = malloc(sizeof(instruccion_x_pcb) + sizeof(instruccionConsola)*cant);
	nuevo->id = nuevoProceso->id;
	nuevo->instrucciones = ins;

	pthread_mutex_lock(&mutex_lista_instrxpcb);
	list_add(instrucciones_x_pcb, nuevo);
	pthread_mutex_unlock(&mutex_lista_instrxpcb);

	pasar_a_new(nuevoProceso);

	sem_post(&sem_planif_largo_plazo);

}

void iniciar_planif_largo_plazo() {
	pthread_create(&planificador_largo_plazo, NULL, (void*)planifLargoPlazo, NULL);
	log_info(loggerKernel, "Inicio planificador largo plazo");
	pthread_detach(planificador_largo_plazo);
}

void iniciar_planif_corto_plazo() {
	pthread_create(&planificador_corto_plazo, NULL, (void*)planifCortoPlazo, NULL);
	log_info(loggerKernel, "Inicio planificador corto plazo %s", configKernel.algoritmo);
	pthread_detach(planificador_corto_plazo);
}

void iniciar_planif_io() {
	pthread_create(&planif_io, NULL, (void*)planifBloqueados, NULL);
	log_info(loggerKernel, "Inicio planificador bloqueados");
	pthread_detach(planif_io);
}

void iniciar_mensajes_cpu() {
	pthread_create(&mensajes_cpu, NULL, (void*)recibirMensajeCPU, NULL);
	log_info(loggerKernel, "Inicio mensajes cpu \n");
	pthread_detach(mensajes_cpu);
}

void planifLargoPlazo() {  //planificador largo plazo: de NEW/SUSPENDED_READY a READY
	log_warning(loggerKernel, "Hilo planif Largo Plazo");
	while(1){
		sem_wait(&sem_planif_largo_plazo);
		sem_wait(&contador_multiprogramacion);

		if(list_is_empty(LISTA_READY_SUSPENDED) == 0){
			log_info(loggerKernel, "Recuperando suspendidos");
			planifMedianoPlazoSuspReady();
		} else {
			log_info(loggerKernel, "Buscando news");
			// pasa de new a ready
			pthread_mutex_lock(&mutex_lista_new);
			PCB* pcb = (PCB*) list_remove(LISTA_NEW, 0);
			pthread_mutex_unlock(&mutex_lista_new);

			/* TODO
			MSJ_KERNEL_MEMORIA_READY* mensajeAMemoria = malloc(sizeof(MSJ_KERNEL_MEMORIA_READY));
			mensajeAMemoria->pid = pcb->id;
			mensajeAMemoria->tamanioProceso= pcb->tamanio;
			pcb->tablaPag = enviarMensajeMemoriaSwapReady(mensajeAMemoria);
			log_debug(loggerKernel, "pcb->tablaPag %i", pcb->tablaPag);
			 */
			pthread_mutex_lock(&mutex_lista_exec);
			int size = list_size(LISTA_EXEC);
			pthread_mutex_unlock(&mutex_lista_exec);
			pasar_a_ready(pcb);
			if (strcmp(configKernel.algoritmo, "SJF") == 0 && size > 0){
				enviarInterruptCPU();
				log_error(loggerKernel, "envie interrupt por nuevo proceso");
			};
			sem_post(&sem_ready);
		}

	}
}


void planifMedianoPlazoSuspReady() {
	// SUSPENDED_READY -> READY
	pthread_mutex_lock(&mutex_lista_ready_suspended);
	PCB* pcb = (PCB*) list_remove(LISTA_READY_SUSPENDED, 0);
	pthread_mutex_unlock(&mutex_lista_ready_suspended);

	/* TODO
	MSJ_KERNEL_MEMORIA_READY* mensajeAMemoria = malloc(sizeof(MSJ_KERNEL_MEMORIA_READY));
	mensajeAMemoria->pid = pcb->id;
	mensajeAMemoria->tamanioProceso= pcb->tamanio;
	pcb->tablaPag = enviarMensajeMemoriaSwapReady(mensajeAMemoria);
	log_debug(loggerKernel, "pcb->tablaPag %i", pcb->tablaPag);
	*/
	pasar_a_ready(pcb);
	if (strcmp(configKernel.algoritmo, "SJF") == 0){
		enviarInterruptCPU();
		log_error(loggerKernel, "envie interrupt por de-suspension");
	};
	sem_post(&sem_ready);
}

void planifCortoPlazo() { // READY -> EXEC
	log_warning(loggerKernel, "Hilo planif corto plazo");
	while(1) {
		sem_wait(&sem_ready);
		sem_wait(&sem_procesador);
		PCB* pcb;

		if(strcmp(configKernel.algoritmo, "SJF") == 0) {
			pcb = algoritmo_SJF();
		} else if (strcmp(configKernel.algoritmo, "FIFO") == 0){
			pcb = algoritmo_FIFO();
		} else {
			log_info(loggerKernel, "El algoritmo de planificacion ingresado no existe\n");
		}

		log_debug(loggerKernel, "PCB elegido: %i", pcb->id);

		pasar_a_exec(pcb);

		gettimeofday(&timeValBefore, NULL);

		serializarPCB(socketCpuDispatch, pcb, DISPATCH_PCB);
		log_debug(loggerKernel,"/////////////////////Envie el mensaje a la cpu dispatch\n");
	}
}


PCB* algoritmo_SJF() {
	void* _eleccion_SJF(void* elemento1, void* elemento2) {

		double estimacion_actual1;
		double estimacion_actual2;

		estimacion_actual1 = (configKernel.alfa) * (((PCB*)elemento1)->real_anterior) + (1-(configKernel.alfa))*(((PCB*)elemento1)->estimacion_anterior);
				estimacion_actual2 = (configKernel.alfa) * (((PCB*)elemento2)->real_anterior) + (1-(configKernel.alfa))*(((PCB*)elemento2)->estimacion_anterior);
		if(estimacion_actual1 <= estimacion_actual2) {
			return ((PCB*)elemento1);
		} else {
			return ((PCB*)elemento2);
		}
	}

	pthread_mutex_lock(&mutex_lista_ready);

	PCB* pcb = (PCB*) list_get_minimum(LISTA_READY, _eleccion_SJF);

	for(int i = 0; i < list_size(LISTA_READY); i++) {
		PCB* pcbActualizacion = list_get(LISTA_READY, i);
		pcbActualizacion->estimacion_anterior = configKernel.alfa*(pcbActualizacion->real_anterior) + ((1 - configKernel.alfa)*(pcbActualizacion->estimacion_anterior));
	}

	pthread_mutex_unlock(&mutex_lista_ready);

	bool _criterio_remocion_lista(void* elemento) {
		return (((PCB*)elemento)->id == pcb->id);
	}

	log_debug(loggerKernel, "size: %i", list_size(LISTA_READY));
	pthread_mutex_lock(&mutex_lista_ready);
	list_remove_by_condition(LISTA_READY, _criterio_remocion_lista);
	pthread_mutex_unlock(&mutex_lista_ready); 
	log_debug(loggerKernel, "size: %i", list_size(LISTA_READY));

	return pcb;
}


PCB* algoritmo_FIFO() {
	log_debug(loggerKernel, "size: %i", list_size(LISTA_READY));
	pthread_mutex_lock(&mutex_lista_ready);
	PCB* pcb = list_remove(LISTA_READY, 0);
	pthread_mutex_unlock(&mutex_lista_ready);
	log_debug(loggerKernel, "size: %i", list_size(LISTA_READY));
	return pcb;
}

void recibirMensajeCPU() {
	log_warning(loggerKernel, "Hilo mensaje cpu");

	while(1) {
		t_pqte* paquete = recibirPaquete(socketCpuDispatch);
		if(paquete == NULL) {
			continue;
		}
		PCB* pcb = deserializoPCB(paquete->buffer);

		bool _criterio_remocion_lista(void* elemento) {
			return (((PCB*)elemento)->id == pcb->id);
		}
		// guardar tiempo que estuvo en EXEC
		gettimeofday(&timeValAfter, NULL);
		pcb->real_anterior= (timeValAfter.tv_sec - timeValBefore.tv_sec)*1000 + (timeValAfter.tv_usec - timeValBefore.tv_usec)/1000;
		switch(paquete->codigo_operacion) {
			case BLOCK_PCB: {
				pthread_t hilo_control_suspendido;
				pthread_create(&hilo_control_suspendido,NULL,(void*)hiloSuspendedor,pcb);
				pthread_detach(hilo_control_suspendido);
				break;
			}
			case EXIT_PCB: {
				log_debug(loggerKernel, "Me llego el finish del pcb %i", pcb->id);
				pthread_mutex_lock(&mutex_lista_exec);
				list_remove_by_condition(LISTA_EXEC, _criterio_remocion_lista);
				pthread_mutex_unlock(&mutex_lista_exec);
				sem_post(&sem_procesador);
				pasar_a_exit(pcb);
				int socketPCB = (int) list_get(LISTA_SOCKETS, pcb->id);
				log_debug(loggerKernel,"Envio la respuesta a la consola socket %i ", socketPCB);
				log_debug(loggerKernel, "pcb->id %i", pcb->id);
				log_debug(loggerKernel, "Tipo mensaje: %i", EXIT_PCB);
				serializarPCB(socketPCB, pcb, EXIT_PCB);
				sem_post(&contador_multiprogramacion);
				break;
			}
			case INTERRUPT_INTERRUPCION: {
				pthread_mutex_lock(&mutex_lista_exec);
				list_remove_by_condition(LISTA_EXEC, _criterio_remocion_lista);
				pthread_mutex_unlock(&mutex_lista_exec);
				pasar_a_ready(pcb);
				sem_post(&sem_ready);
				sem_post(&sem_procesador);
				break;
			}
		}


	   free(paquete->buffer->stream);
	   free(paquete->buffer);
	   free(paquete);
	}
}

void hiloSuspendedor(PCB* pcb){
	bool _criterio_igual_id(void* elemento) {
		return (((PCB*)elemento)->id == pcb->id);
	}

	//EXEC -> BLOCKED
	pthread_mutex_lock(&mutex_lista_exec);
	list_remove_by_condition(LISTA_EXEC, _criterio_igual_id);
	pthread_mutex_unlock(&mutex_lista_exec);

	sem_post(&sem_procesador);
	pasar_a_block(pcb);

	pthread_mutex_lock(&mutex_lista_cola_io);
	list_add(COLA_BLOQUEO_IO, pcb);
	pthread_mutex_unlock(&mutex_lista_cola_io);
	sem_post(&sem_bloqueo);

	sleep(configKernel.tiempoMaximoBloqueado/1000);
			
	if(list_any_satisfy(LISTA_BLOCKED,_criterio_igual_id)){
		// BLOCKED -> SUSPENDED_BLOCK
		pthread_mutex_lock(&mutex_lista_blocked);
		list_remove_by_condition(LISTA_BLOCKED,_criterio_igual_id);
		pthread_mutex_unlock(&mutex_lista_blocked);

		//CUANDO PASAS EL PROCESO A SUSPENDIDO, HAY QUE AVISARLE A LA MEMORIA PARA Q PASE LAS PAGINAS A DISCO
		//enviarMensaje(socketMemoriaSwap, KERNEL, &pcb->id, sizeof(int), SUSPENDER);

		pasar_a_susp_block(pcb);
		sem_post(&contador_multiprogramacion);
	}
}

void planifBloqueados() {
	log_warning(loggerKernel, "Hilo planifBloqueados");
	while (1) {
		sem_wait(&sem_bloqueo);

		pthread_mutex_lock(&mutex_lista_cola_io);
		PCB* pcb = list_remove(COLA_BLOQUEO_IO, 0);
		pthread_mutex_unlock(&mutex_lista_cola_io);

		bool _criterio_ins(void* elemento) {
			return (((instruccion_x_pcb*)elemento)->id == pcb->id);
		}

		pthread_mutex_lock(&mutex_lista_instrxpcb);
		instruccion_x_pcb* insxpcb = list_find(instrucciones_x_pcb,_criterio_ins);
		pthread_mutex_unlock(&mutex_lista_instrxpcb);

		int index = pcb->programCounter - 1; //en cpu le aumenta el pc al prox

		instruccionConsola* ins = list_get(insxpcb->instrucciones, index);
		uint32_t duracion = ins->parametro1 /1000;
		log_warning(loggerKernel, "pcb->programCounter %i", pcb->programCounter);
		log_warning(loggerKernel, "ins->parametro1 %i", ins->parametro1);
		log_warning(loggerKernel, "duracion %i", duracion);
		sleep(duracion);
		//aumento Program counter
		pcb->programCounter++;
		bool _criterio_igual_id(void* elemento) {
			return (((PCB*)elemento)->id == pcb->id);
		}

		if(list_any_satisfy(LISTA_BLOCKED,_criterio_igual_id)) {
			pthread_mutex_lock(&mutex_lista_blocked);
			list_remove_by_condition(LISTA_BLOCKED,_criterio_igual_id);
			pthread_mutex_unlock(&mutex_lista_blocked);

			pasar_a_ready(pcb);
			if (strcmp(configKernel.algoritmo, "SJF") == 0){
				enviarInterruptCPU();
				log_error(loggerKernel, "envie interrupt por fin de IO");
			};
			sem_post(&sem_ready);
		} else if (list_any_satisfy(LISTA_BLOCKED_SUSPENDED,_criterio_igual_id)) {
			pthread_mutex_lock(&mutex_lista_blocked_suspended);
			list_remove_by_condition(LISTA_BLOCKED_SUSPENDED,_criterio_igual_id);
			pthread_mutex_unlock(&mutex_lista_blocked_suspended);

			pasar_a_susp_ready(pcb);
			sem_post(&sem_planif_largo_plazo);
		}
	}

}
