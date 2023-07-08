#include "headers/utils.h"

void initCPU() {
	loggerCPU = log_create(PATH_LOG_CPU, "kernel", 1, LOG_LEVEL_DEBUG);
	t_config* archivoConfig = config_create(PATH_CONFIG_CPU);
	configCPU = extraerDatosConfig(archivoConfig);
	config_destroy(archivoConfig);

	sem_init(&sem_exec, 0, 0);
}

t_configCPU extraerDatosConfig(t_config* archivoConfig) {
	t_configCPU config;
	config.reemplazoTLB = string_new();
	config.ipMemoria = string_new();
	config.entradasTLB = config_get_int_value(archivoConfig, "ENTRADAS_TLB");
	string_append(&config.reemplazoTLB, config_get_string_value(archivoConfig, "REEMPLAZO_TLB"));
	config.retardoNOOP = config_get_int_value(archivoConfig, "RETARDO_NOOP");
	string_append(&config.ipMemoria, config_get_string_value(archivoConfig, "IP_MEMORIA"));
	config.puertoMemoria = config_get_int_value(archivoConfig, "PUERTO_MEMORIA");
	config.puertoEscuchaDispatch = config_get_int_value(archivoConfig, "PUERTO_ESCUCHA_DISPATCH");
	config.puertoEscuchaInterrupt = config_get_int_value(archivoConfig, "PUERTO_ESCUCHA_INTERRUPT");

	return config;
}

void recibir_config_memoria(){
	log_debug(loggerCPU,"Buscando configuracion inicial de memoria");
	socketMemoria = iniciarCliente(configCPU.ipMemoria, configCPU.puertoMemoria);
	int mensaje = 1;

	enviarMensaje(socketMemoria, CPU, &mensaje, sizeof(mensaje), CONFIG_DIR_LOG_A_FISICA);
	log_debug(loggerCPU,"Esperando mensaje de memoria");

	t_paquete paquete;
	recibirMensaje(socketMemoria, &paquete);
	log_debug(loggerCPU,"Recibida informacion de memoria");

	MSJ_MEMORIA_CPU_INIT* infoDeMemoria;
	infoDeMemoria = paquete.mensaje;

	configCPU.cantidadEntradasPorTabla = infoDeMemoria->cantEntradasPorTabla;
	configCPU.tamanioPagina = infoDeMemoria->tamanioPagina;
}

