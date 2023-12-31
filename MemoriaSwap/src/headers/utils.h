#ifndef UTILS_H_
#define UTILS_H_

#include "MemoriaSwap.h"


bool buscarProcesoEnTabla(int pid);
int calcularCantTablasSegundoNivelParaUnProceso(int cantPaginasAUsar);
int buscarNroTabla1N(int pid);
int buscarIdTabla2NEnUnaLista(t_list* tablaPaginas1N, int pagina);
int buscarFrameEnFilaEnListaConPagina(int pagina, t_list* tablaPaginas2Nivel);
int estaEnRam(int nroFrame);
int hayFrameLibreParaElPid(int pid);  //retorna el nro de frame si existe, o sino retorna -1 si no hay ninguno libre
t_list* getPagsEnRam(void);
#endif
