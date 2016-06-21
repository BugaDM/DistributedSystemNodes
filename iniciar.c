#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/msg.h>
#include "aux.h"


/*IPCS -Q MOSTRAR
  IPCRM -A ELIMINAR

*/


int main(int argc, char *argv[]){
  system("clear");
  printf("Eliminando basura...\n");
  system("ipcrm -a");
  struct cola colas;
  
  /* Inicializa las variables de las colas de variables
     de todos los nodos del sistema, por defecto todo a 0
  */
  int s=0;
  int xxx;
  for(s=0;s<NODOS;s++){
   colas= iDColaNodo(s);
   
   printf("[Nodo:%u] cola vars= %u\n",s,colas.msqid_vars);
   printf("[Nodo:%u] cola mutex= %u\n",s,colas.msqid_mutex);
   printf("[Nodo:%u] cola peticiones= %u\n",s,colas.msqid_peticiones);
   if (s==0) xxx = colas.msqid_peticiones;
   inicializarVariables(colas.msqid_vars);

  }

  printf("Presiona ENTER para enviar el testigo\n");
  getchar();

  inyectarTestigo(xxx);
  while(1);

  return 0;
}

