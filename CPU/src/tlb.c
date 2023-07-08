#include "tlb.h"



/* Campos que vienen del archivo de configuracion
 * CANTIDAD_ENTRADAS_TLB : Num
 * ALGORITMO_REEMPLAZO_TLB: String
 * RETARDO_ACIERTO_TLB: Num
 * RETARDO_FALLO_TLB: Num
 */
void iniciar_TLB(t_config_memoria* config){
	//Fin de para pruebas
	int cantidad_entradas_tlb = config->CANTIDAD_ENTRADAS_TLB;

	if(cantidad_entradas_tlb==0){
		TLBEnable = 0;
		return;
	}

	TLBEnable = 1;

	TLB = malloc(sizeof(tlb));

	TLB->size = cantidad_entradas_tlb;
	TLB->entradas = list_create();
	TLB->algoritmo = config->ALGORITMO_REEMPLAZO_TLB;
	iniciar_metricas(); //   ???

}

int tlbTieneEntradasLibres(){
	return TLB->size > TLB->entradas->elements_count;
}

//En este caso, la TLB tiene una o mas entradas libres
void llenar_TLB(int nroPagina,int nroFrame){
	entrada_tlb* entry = malloc(sizeof(entrada_tlb));
	entry->nroPagina = nroPagina;
	entry->nroFrame = nroFrame;
	pthread_mutex_lock(&mutexTLB);
	list_add_in_index(TLB->entradas, 0, entry);
	pthread_mutex_unlock(&mutexTLB);

}



void actualizar_TLB(int nroPagina,int nroFrame){

	if(tlbTieneEntradasLibres()){
		llenar_TLB(nroPagina, nroFrame);
		return;
	}


	//REEMPLAZO DE PAGINA
	if(strcmp(TLB->algoritmo , "LRU")== 0){
		reemplazo_lru(nroPagina, nroFrame);
	} else {
		reemplazo_fifo(nroPagina, nroFrame);
	}

}

void reemplazo_fifo(int nroPagina, int nroFrame){
	entrada_tlb* nuevo = malloc(sizeof(entrada_tlb));
	nuevo->nroPagina = nroPagina;
	nuevo->nroFrame = nroFrame;

	
	entrada_tlb* viejo = list_remove(TLB->entradas, TLB->size-1);
	list_add_in_index(TLB->entradas, 0, nuevo);


	logs_reemplazo_entrada(viejo->nroPagina, viejo->nroFrame, nroPagina, nroFrame);
	free(viejo);
}

/*void reemplazo_lru(int nroPagina, int nroFrame){
	int i = 0;
	entrada_tlb* actual = list_get(TLB->entradas, i);
	entrada_tlb* leastRecentlyUsed = actual; //victima
	pthread_mutex_lock(&mutexTLB);
	for(i=1; i< TLB->size; i++){
		actual = list_get(TLB->entradas, i);
		if(es_fecha_anterior_marco(actual->last_use, leastRecentlyUsed->last_use)){
			leastRecentlyUsed = actual;
		}
	}
	logs_reemplazo_entrada(leastRecentlyUsed->nroPagina, leastRecentlyUsed->nroFrame, nroPagina, nroFrame);
	leastRecentlyUsed->nroFrame = nroFrame;
	leastRecentlyUsed->nroPagina = nroPagina;
	leastRecentlyUsed->last_use = temporal_get_string_time("%d/%m/%y %H:%M:%S");
	pthread_mutex_unlock(&mutexTLB);*/
}

int buscar_en_TLB(int nroPagina){

	entrada_tlb* actual;
	for(int i=0; i< TLB->entradas->elements_count; i++){
		actual = list_get(TLB->entradas, i);
		if(actual->nroPagina == nroPagina){
			//printf("MOMENTO: %s. \n\n\n\n", temporal_get_string_time("%d/%m/%y %H:%M:%S:%MS")); ???
			//usleep(TLB->retardoAcierto * 1000); //RETARDO DE ACIERTO ??
			log_TLB_Hit(actual->nroPagina, actual->nroFrame);
			//hit_metrics(pid); ???
			return actual->nroFrame;
		}
	}
	printf("MOMENTO: %s. \n\n\n\n", temporal_get_string_time("%d/%m/%y %H:%M:%S:%MS"));
	usleep(TLB->retardoFallo * 1000); //RETARDO DE FALLO
	log_TLB_Miss(nroPagina);
	//miss_metrics(pid); ??
	return -1;
}

void destruir_entrada(void* entry){
	entrada_tlb* tlbEntry = (entrada_tlb*) entry;
	free(tlbEntry);
}

void limpiar_entradas_TLB(){
	list_clean_and_destroy_elements(TLB->entradas, destruir_entrada);
}

void cerrar_TLB(){
	int i;
	entrada_tlb* actual;
	for(i=0; i< TLB->entradas->elements_count; i++){
		actual = list_get(TLB->entradas, i);
		list_remove_and_destroy_element(TLB->entradas, i, destruir_entrada);
	}
	free(TLB->algoritmo);
	free(TLB);
}

void buscarEntrada(int Pag){
	int i = 0;
	int entradas =  list_size(lista_entradas);
	while(i<entradas){
		int tlbAux = list_get(lista_entradas,i);
		if(tlbAux == Pag){return 1;}
		else{i++}
		}
	return 0;
}


void reemplazo_lru(int nroPagina, int nroFrame){
	int i = list_size(entradas);
	i--;
	entrada_tlb* nuevaEntrada;
	nuevaEntrada->nroPagina = nroPagina;
	nuevaEntrada->nroFrame = nroFrame;
	entrada_tlb* remplazada;
	remplazada = list_remove(TLB->entradas, i, destruir_entrada);
	list_add_in_index(TLB->entradas, 0, nuevaEntrada);

	logs_reemplazo_entrada(remplazada->nroPagina, remplazada->nroFrame, nroPagina, nroFrame);

}




int buscar_en_TLB(int nroPagina){ //devuelve numero de frame, si esta en la tlb, devuelve -1 si no esta en la tlb

	entrada_tlb* actual;
	for(int i=0; i< tlb->entradas->elements_count; i++){
		actual = list_get(TLB->entradas, i);
		if(actual->nroPagina == nroPagina){
			//printf("MOMENTO: %s. \n\n\n\n", temporal_get_string_time("%d/%m/%y %H:%M:%S:%MS")); ???
			//usleep(TLB->retardoAcierto * 1000); //RETARDO DE ACIERTO ??
			log_TLB_Hit(actual->nroPagina, actual->nroFrame);
			if(tlb->algoritmo == lru){
				actual = list_remove(tlb->entradas,i)
				list_add_in_index(tlb->entradas,0,actual);}
			//hit_metrics(pid); ???
			return actual->nroFrame;
		}
	}
	printf("MOMENTO: %s. \n\n\n\n", temporal_get_string_time("%d/%m/%y %H:%M:%S:%MS"));
	//usleep(TLB->retardoFallo * 1000); //RETARDO DE FALLO ??
	log_TLB_Miss(nroPagina);
	//miss_metrics(pid); ??
	return -1;
}


