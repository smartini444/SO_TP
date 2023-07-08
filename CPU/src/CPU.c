#include "headers/CPU.h"

int main(void) {
	initCPU();

	interrupciones = false;
	log_debug(loggerCPU,"main");
	iniciar_dispatch();
	iniciar_interrupt();
	recibir_config_memoria();

	signal(SIGINT,  &cerrarCPU);
}

void iniciar_dispatch() {
	pthread_create(&hiloDispatch, NULL, (void*)iniciarDispatch, NULL);
	log_info(loggerCPU, "iniciar_dispatch");
	pthread_detach(hiloDispatch);
}

void iniciar_interrupt() {
	pthread_create(&hiloInterrupt, NULL, (void*)iniciarInterrupt, NULL);
	log_info(loggerCPU, "iniciar_interrupt");
	pthread_join(hiloInterrupt, NULL);
}

void iniciarDispatch(){
	log_debug(loggerCPU,"iniciarDispatch");
	int conexionDispatch = iniciarServidor(configCPU.puertoEscuchaDispatch);
	struct sockaddr_in dir_cliente;
	socklen_t tam_direccion = sizeof(struct sockaddr_in);
	socketAceptadoDispatch = 0;
	socketAceptadoDispatch = accept(conexionDispatch, (void*)&dir_cliente, &tam_direccion);
	log_debug(loggerCPU,"[CPU] Puerto Dispatch Escuchando");
	while(1){
		t_pqte* paquete= recibirPaquete(socketAceptadoDispatch);
		if (paquete == NULL) {
			continue;
		}
		PCB* pcb = deserializoPCB(paquete->buffer);
		free(paquete->buffer->stream);
		free(paquete->buffer);
		free(paquete);
		t_list* instrucciones = leerPseudocodigo(pcb->instrucciones);
		log_debug(loggerCPU, "Leyo instrucciones cant %i", list_size(instrucciones));

		cicloInstruccion(instrucciones, pcb);

		log_info(loggerCPU, "destroy list");
		list_destroy(instrucciones);
	}
}

void iniciarInterrupt(){
	log_debug(loggerCPU,"iniciarInterrupt");
	int conexionInterrupt = iniciarServidor(configCPU.puertoEscuchaInterrupt);
	struct sockaddr_in dir_cliente;
	socklen_t tam_direccion = sizeof(struct sockaddr_in);
	socketAceptadoInterrupt = 0;
	socketAceptadoInterrupt = accept(conexionInterrupt, (void*)&dir_cliente, &tam_direccion);
	while(1){
		t_paquete paquete;
		recibirMensaje(socketAceptadoInterrupt, &paquete);

		interrupciones = true;
	}
}

void cicloInstruccion(t_list* instrucciones, PCB* pcb) {
	// fetch
	uint32_t index = pcb->programCounter;
	instruccionConsola* insActual = list_get(instrucciones, index);
	pcb->programCounter += 1;
	log_info(loggerCPU,"insActual pc: %i", index);
	log_info(loggerCPU,"insActual->identificador: %i", insActual->identificador);

	//decode y fetch operands
	if(insActual->identificador == COPY) {
		log_info(loggerCPU, "buscar operandos en memoria");
	}

	//execute
	log_warning(loggerCPU, "Ejecutando pcb->id %i", pcb->id);
	log_warning(loggerCPU, "real %i", pcb->real_anterior);
	log_warning(loggerCPU, "estimacion %i", pcb->estimacion_anterior);

	bool retornePCB = false;
	switch(insActual->identificador){
		case IO:
			serializarPCB(socketAceptadoDispatch, pcb, BLOCK_PCB);
			log_debug(loggerCPU,"Envie BLOCK al kernel por IO");
			retornePCB = true;
			break;
		case NO_OP:
			log_debug(loggerCPU,"NO_OP");
			usleep(1000 * configCPU.retardoNOOP);
			break;
		case READ:
			log_debug(loggerCPU, "READ %i", insActual->parametro1);
			MSJ_MEMORIA_CPU_READ* dirFisica /*= FUNC QUE ME VA A PASAR LAGO*/;
			enviarMensaje(socketMemoria, CPU, dirFisica, sizeof(MSJ_MEMORIA_CPU_READ), ACCESO_MEMORIA_READ);
			log_debug(loggerCPU, "Envie direccion fisica a memoria swap\n");

			t_paquete paqueteMemoriaSwap;
			recibirMensaje(socketMemoria, &paqueteMemoriaSwap);

			log_info(loggerCPU, "Mensaje leido: %s", (char*)paqueteMemoriaSwap.mensaje);

			break;
		case WRITE:
			log_debug(loggerCPU, "WRITE %i %i", insActual->parametro1, insActual->parametro2);
			break;
		case COPY:
			log_debug(loggerCPU, "COPY %i %i", insActual->parametro1, insActual->parametro2);
			//funciona como un write: el parametro2 es el valor, lo tendrias que haber buscado en el fetchOperands
			break;
		case EXIT:
			serializarPCB(socketAceptadoDispatch, pcb, EXIT_PCB);
			log_debug(loggerCPU,"Envie EXIT al kernel");
			retornePCB = true;
			break;
	}
	//check interrupt
	if(!interrupciones && !retornePCB) {
		cicloInstruccion(instrucciones, pcb);
		log_debug(loggerCPU,"recursivo %i", index);
	} else if (interrupciones) {
		// devuelvo pcb a kernel
		log_error(loggerCPU,"devuelvo pcb");
		serializarPCB(socketAceptadoDispatch, pcb, INTERRUPT_INTERRUPCION);
		interrupciones = false;
	}
}

void cerrarCPU(){
	log_warning(loggerCPU, "cerrarCPU");
	log_destroy(loggerCPU);
}
