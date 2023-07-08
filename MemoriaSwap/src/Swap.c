#include "headers/Swap.h"

void* read_in_swap(int pid, int numero_frame,int desplazamiento){
	//char* path = string_new();
	//int tamanio_pagina = 64;
	//int tamanio_swap = tamanio_pagina * 4;
	pthread_mutex_lock(&mutexLoggerSwap);
		log_debug(loggerSwap, "[INIT - READ_SWAP] PID: %d FRAME: %d OFFSET: %d", pid, numero_frame, desplazamiento);
	pthread_mutex_unlock(&mutexLoggerSwap);

    char* path = configMemoriaSwap.pathSwap;
    int tamanio_pagina = configMemoriaSwap.tamanioPagina;
    int tamanio_swap = tamanio_pagina * configMemoriaSwap.marcosPorProceso;
    int posicion = tamanio_pagina*numero_frame;
    int posicion_final = posicion + desplazamiento;
    char* new_pid = string_itoa(pid);
    string_append(&path, "/");
    string_append(&path, new_pid ); //se puede abstraer
    string_append(&path, ".swap");
    int fd = open(path,O_RDWR, S_IRUSR| S_IWUSR);

    char* maped = mmap(NULL,tamanio_swap, PROT_WRITE, MAP_SHARED,fd,0);
    char* bytes = malloc(tamanio_swap);
    for(int i = 0 ; i < tamanio_swap ; i++ ){
        bytes[i] = maped[posicion];
        posicion++;
        if(posicion == posicion_final){
        	break;
        }
    }
    int error = munmap(maped,tamanio_swap);
    close(fd);
    if(error != 0){
        free(bytes);
        free(path);
    	pthread_mutex_lock(&mutexLoggerSwap);
    		log_error(loggerSwap, "[READ_SWAP] NPID: %d FRAME: %d OFFSET: %d", pid, numero_frame, desplazamiento);
    	pthread_mutex_unlock(&mutexLoggerSwap);
        return "ERROR";
    }
    else{
        free(path);
    	pthread_mutex_lock(&mutexLoggerSwap);
    		log_debug(loggerSwap, "[FIN - READ_SWAP] PID: %d FRAME: %d OFFSET: %d", pid, numero_frame, desplazamiento);
    	pthread_mutex_unlock(&mutexLoggerSwap);
        return bytes;
    }
}

t_resultadoWriteSwap write_frame(int pid, int numero_frame, void* bytes){
	pthread_mutex_lock(&mutexLoggerSwap);
		log_debug(loggerSwap, "[INIT - WRITE_FRAME_SWAP] PID: %d FRAME: %d", pid, numero_frame);
	pthread_mutex_unlock(&mutexLoggerSwap);
	char* bytes_char;
	bytes_char = (char *)bytes;
	//char* path = string_new();
	//int tamanio_pagina = 64;
	//int tamanio_swap = tamanio_pagina * 4;
	char* path = configMemoriaSwap.pathSwap;
	int tamanio_pagina = configMemoriaSwap.tamanioPagina;
	int tamanio_swap = tamanio_pagina * configMemoriaSwap.marcosPorProceso;
	int posicion = tamanio_pagina*numero_frame;
	int posicion_final = posicion + tamanio_pagina;
	char* new_pid = string_itoa(pid);
	string_append(&path, "/");
	string_append(&path, new_pid ); //se puede abstraer
	string_append(&path, ".swap");
	int fd = open(path,O_RDWR, S_IRUSR| S_IWUSR);

	char* maped = mmap(NULL,tamanio_swap, PROT_WRITE, MAP_SHARED,fd,0);
	for(int i = 0 ; i < tamanio_swap ; i++ ){
		maped[posicion] = bytes_char[i];
		posicion++;
		if(posicion == posicion_final){
			break;
		}
	}
	int error = munmap(maped,tamanio_swap);
	if(error != 0){
		free(bytes);
		free(path);
		pthread_mutex_lock(&mutexLoggerSwap);
			log_error(loggerSwap, "[WRITE_FRAME_SWAP] MAPPING FAILED PID: %d FRAME: %d", pid, numero_frame);
		pthread_mutex_unlock(&mutexLoggerSwap);
		return WRITE_ERROR;
	}
	else{
		free(path);
		pthread_mutex_lock(&mutexLoggerSwap);
			log_debug(loggerSwap, "[FIN - WRITE_FRAME_SWAP] PID: %d FRAME: %d", pid, numero_frame);
		pthread_mutex_unlock(&mutexLoggerSwap);
		return WRITE_OK;
	}
}

t_resultadoWriteSwap write_pid_completo(int pid, int tamanio, void* bytes){
	pthread_mutex_lock(&mutexLoggerSwap);
		log_debug(loggerSwap, "[INIT - WRITE_NEW_PID_SWAP] PID: %d", pid);
	pthread_mutex_unlock(&mutexLoggerSwap);
	char* bytes_char;
	bytes_char = (char *)bytes;
	//char* path = string_new();
	//int tamanio_pagina = 64;
	//int tamanio_swap = tamanio_pagina * 4;
	char* path = configMemoriaSwap.pathSwap;
	int tamanio_pagina = configMemoriaSwap.tamanioPagina;
	int tamanio_swap = tamanio_pagina * configMemoriaSwap.marcosPorProceso;
	int posicion_final = tamanio;
	char* new_pid = string_itoa(pid);
	string_append(&path, "/");
	string_append(&path, new_pid ); //se puede abstraer
	string_append(&path, ".swap");
	FILE *f = fopen(path,"w");
	ftruncate(fileno(f),tamanio_swap);
	fclose(f);
	int fd = open(path,O_RDWR, S_IRUSR| S_IWUSR);
	char* maped = mmap(NULL,tamanio_swap, PROT_WRITE, MAP_SHARED,fd,0);
	for(int posicion = 0 ; posicion < tamanio_swap ; posicion++ ){
		maped[posicion] = bytes_char[posicion];
		if(posicion == posicion_final){
			break;
		}
	}
	int error = munmap(maped,tamanio_swap);
	if(error != 0){
		free(bytes);
		free(path);
		pthread_mutex_lock(&mutexLoggerSwap);
			log_error(loggerSwap, "[WRITE_NEW_PID_SWAP] MAPPING FAILED PID: %d", pid);
		pthread_mutex_unlock(&mutexLoggerSwap);
		return WRITE_ERROR;
	}
	else{
		free(path);
		pthread_mutex_lock(&mutexLoggerSwap);
			log_debug(loggerSwap, "[FIN - WRITE_NEW_PID_SWAP] PID: %d", pid);
		pthread_mutex_unlock(&mutexLoggerSwap);
		return WRITE_OK;
	}
}

t_resultadoWriteSwap copy(int pid, int numero_frame, void* bytes, int desplazamiento){
	pthread_mutex_lock(&mutexLoggerSwap);
		log_debug(loggerSwap, "[INIT - COPY_SWAP] PID: %d FRAME: %d", pid, numero_frame);
	pthread_mutex_unlock(&mutexLoggerSwap);
	char* bytes_char;
	bytes_char = (char *)bytes;
	//char* path = string_new();
	//int tamanio_pagina = 64;
	//int tamanio_swap = tamanio_pagina * 4;
	char* path = configMemoriaSwap.pathSwap;
	int tamanio_pagina = configMemoriaSwap.tamanioPagina;
	int tamanio_swap = tamanio_pagina * configMemoriaSwap.marcosPorProceso;
	int posicion = tamanio_pagina*numero_frame;
	int posicion_final = posicion + desplazamiento;
	char* new_pid = string_itoa(pid);
	string_append(&path, "/home/utnso/swap/");
	string_append(&path, new_pid ); //se puede abstraer
	string_append(&path, ".swap");
	int fd = open(path,O_RDWR, S_IRUSR| S_IWUSR);

	char* maped = mmap(NULL,tamanio_swap, PROT_WRITE, MAP_SHARED,fd,0);
	for(int i = 0 ; i < tamanio_swap ; i++ ){
		maped[posicion] = bytes_char[i];
		posicion++;
		if(posicion == posicion_final){
			break;
		}
	}
	int error = munmap(maped,tamanio_swap);
	if(error != 0){
		free(bytes);
		free(path);
		pthread_mutex_lock(&mutexLoggerSwap);
			log_error(loggerSwap, "[COPY_SWAP] MAPPING FAILED PID: %d FRAME: %d", pid, numero_frame);
		pthread_mutex_unlock(&mutexLoggerSwap);
		return WRITE_ERROR;
	}
	else{
		free(path);
		pthread_mutex_lock(&mutexLoggerSwap);
			log_debug(loggerSwap, "[FIN - COPY_SWAP] PID: %d FRAME: %d", pid, numero_frame);
		pthread_mutex_unlock(&mutexLoggerSwap);
		return WRITE_OK;
	}
}

void delete_in_swap(int pid){
	pthread_mutex_lock(&mutexLoggerSwap);
		log_debug(loggerSwap, "[INIT - DELETE_PID_SWAP] PID: %d", pid);
	pthread_mutex_unlock(&mutexLoggerSwap);
	char* path = string_new();
    string_append(&path, string_itoa(pid) ); //se puede abstraer
    string_append(&path, ".swap");
	if (remove(path) == 0){
		pthread_mutex_lock(&mutexLoggerSwap);
			log_debug(loggerSwap, "[FIN - DELETE_PID_SWAP] PID: %d", pid);
		pthread_mutex_unlock(&mutexLoggerSwap);
	}
    else{
    	pthread_mutex_lock(&mutexLoggerSwap);
    		log_error(loggerSwap, "[DELETE_PID_SWAP] PID: %d", pid);
    	pthread_mutex_unlock(&mutexLoggerSwap);
    }
}

void replace_pid_suspendido(int pid, int tamanio, void* bytes){
	//TODO write_pid_suspendido
}


