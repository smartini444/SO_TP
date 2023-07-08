#include "headers/MemoriaSwap.h"

int main(void) {
	iniciarConfigYLog();
	iniciarSemaforos();
	iniciarListas();
	iniciarEstructuras();


	pthread_mutex_lock(&mutexLoggerMemoria);
		log_debug(loggerMemoria,"[INIT - MEMORIA] FINALIZADO. COMIENZA MULTIHILO");
	pthread_mutex_unlock(&mutexLoggerMemoria);

	iniciarModuloMultihilo();
	
	return EXIT_SUCCESS;

}

void iniciarConfigYLog(void) {
	//estas lineas de log y config sirven para ejecutar desde la terminal
	loggerMemoria = log_create(PATH_LOG_MEMORIA, "MEMORIA", 1, LOG_LEVEL_DEBUG);
	loggerSwap = log_create(PATH_LOG_SWAP, "SWAP", 1, LOG_LEVEL_DEBUG);
	t_config* archivoConfig = config_create(PATH_CONFIG_MEMORIASWAP);

	//estas lineas de log y config sirven para debuggear en eclipse
	//loggerKernel = log_create("../Kernel/logKernel.log", "kernel", 1, LOG_LEVEL_DEBUG);
	//t_config* archivoConfig = config_create("../Kernel/configKernel.config");

	if (archivoConfig == NULL){
		printf("////////////no se puede leer el archivo config porque estas ejecutando con las lineas comentadas");
		exit(EXIT_FAILURE);
	}
	extraerDatosConfig(archivoConfig);
	config_destroy(archivoConfig);

	pthread_mutex_lock(&mutexLoggerMemoria);
		log_debug(loggerMemoria,"[FIN - CONFIG Y LOG] CREADOS EXITOSAMENTE");
	pthread_mutex_unlock(&mutexLoggerMemoria);
}

void extraerDatosConfig(t_config* archivoConfig) {
	configMemoriaSwap.algoritmoReemplazo = string_new();
	configMemoriaSwap.pathSwap = string_new();

	string_append(&configMemoriaSwap.algoritmoReemplazo, config_get_string_value(archivoConfig, "ALGORITMO_REEMPLAZO"));
	string_append(&configMemoriaSwap.pathSwap,config_get_string_value(archivoConfig, "PATH_SWAP"));

	configMemoriaSwap.puertoEscucha  = config_get_int_value(archivoConfig, "PUERTO_ESCUCHA");
	configMemoriaSwap.tamanioMemoria = config_get_int_value(archivoConfig, "TAM_MEMORIA");
	configMemoriaSwap.tamanioPagina  = config_get_int_value(archivoConfig, "TAM_PAGINA");
	configMemoriaSwap.entradasPorTabla = config_get_int_value(archivoConfig, "ENTRADAS_POR_TABLA");
	configMemoriaSwap.retardoMemoria   = config_get_int_value(archivoConfig, "RETARDO_MEMORIA");
	configMemoriaSwap.marcosPorProceso = config_get_int_value(archivoConfig, "MARCOS_POR_PROCESO");
	configMemoriaSwap.retardoSwap = config_get_int_value(archivoConfig, "RETARDO_SWAP");
}


void iniciarSemaforos(void){
	pthread_mutex_init(&mutexLoggerMemoria, NULL);
	pthread_mutex_init(&mutexLoggerSwap, NULL);
	pthread_mutex_init(&mutexSwap, NULL);
	pthread_mutex_init(&mutexIdTabla1N, NULL);
	pthread_mutex_init(&mutexIdTabla2N, NULL);
	pthread_mutex_init(&mutexLista1N, NULL);
	pthread_mutex_init(&mutexLista2N, NULL);
	pthread_mutex_init(&mutexListaFrames, NULL);
	pthread_mutex_init(&mutexVoidMemoriaPrincipal, NULL);
	pthread_mutex_init(&mutexAlgoritmoReemplazo, NULL);
}

void iniciarListas(void){
	tablaDeFrames      = list_create();
	listaTodasTablas1N = list_create();
	listaTodasTablas2N = list_create();
}

void iniciarModuloMultihilo(void){
	conexion = iniciarServidor(configMemoriaSwap.puertoEscucha);
	struct sockaddr_in dir_cliente;
	socklen_t tam_direccion = sizeof(struct sockaddr_in);

	while(1){
		socketAceptado = 0;
		socketAceptado = accept(conexion, (void*)&dir_cliente, &tam_direccion);
		if(socketAceptado == -1)
			continue;
		else {
			pthread_t hiloConexionGenerica;
			pthread_create(&hiloConexionGenerica, NULL, (void*) conexionGenerica, (void*)socketAceptado);

			pthread_mutex_lock(&mutexLoggerMemoria);
				log_debug(loggerMemoria,"[MULTIHILO] NUEVA CONEXION");
			pthread_mutex_unlock(&mutexLoggerMemoria);

			pthread_detach(hiloConexionGenerica);
		}
	}
}


