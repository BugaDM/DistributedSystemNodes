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
int nodo;

int main(int argc, char *argv[]){
  /***************************************
   *          TESTIGO                    *
   ***************************************/
  struct testigo test;
  struct cola colas;
  int enviado=0; 
  nodo=atoi(argv[1]);
  int myID=atoi(argv[1])*100+atoi(argv[2]);
  printf("[Pagos:%u]Pertenezco al nodo [%u]\n",myID,nodo);
  /* Una vez sabemos a que nodo pertenece este proceso,
     se procede a "localizar" las tres colas que va a utilizar,
     que son ID_NODO,ID_NODO+1,ID_NODO+2
  */
  colas = iDColaNodo(nodo);
 
  
  /* INICIALIZAMOS LAS 3 COLAS */

  int msqid_var=colas.msqid_vars;
  int msqid_mutex=colas.msqid_mutex;
  int msqid_peticiones=colas.msqid_peticiones;
  printf("[Pagos:%u NODE:%u]msqid_var-> %u\n",myID,nodo,msqid_var);
  printf("[Pagos %u NODE: %u]msqid_mutex-> %u\n",myID,nodo,msqid_mutex);
  printf("[Pagos %u NODE: %u]msqid_peticiones-> %u\n",myID,nodo,msqid_peticiones);
  
  /* Una vez inicializamos las 3 colas pasamos a recoger
     las variables que nos permiten comunicarnos con el resto 
     de nodos del sistema.*/
  struct vars variables;

 
  esperar(myID);//hago otras cosas
  printf("[Pagos %u NODE: %u]Actualizando variables\n",myID);

  variables=actualizarVariables(msqid_var,1,TIPO_PAGOS);
  //Tenemos que acordarnos que una vez salgamos de la
  //zona de mutex hay que hacer actualizarVariables(msqid_var,0)
  //para indicar que este thread ya no quiere entrar más.
  
  //Si no tenemos el testigo, y no hay más procesos de pagos
  //en este nodo que quieran entrar,
  if(variables.num_pagos<=1){
    printf("[Pagos %u NODE: %u]Enviando peticiones al resto de nodos.\n",myID,nodo);
    
    send_peticion(nodo,TIPO_PAGOS);
    printf("[Pagos %u NODE: %u]Liberando variables\n",myID,nodo);

  } else{
    if(variables.testigoLibre==1){
      send_peticion(nodo,TIPO_PAGOS);
    }
  }
  enviarVariables(msqid_var,&variables,1);

  /* Solicitamos la exclusion mutua */
  printf("[Pagos %u NODE: %u]Pidiendo permiso COLA[%u]\n",myID,nodo,msqid_mutex);

  test=receive_permiso_pagos(msqid_mutex);

  
  variables = actualizarVariables(msqid_var,0,TIPO_PAGOS); // El 2 indica no modificar num_pagos
  /* Actualizamos las variables, ya hemos sido atendidos */
  
  variables.testigoLibre=0;
  variables.testigo=1;
  variables.dentro=1;
  variables.k=0;
  test.lector=0;
  test.k=variables.k;
  variables.bloqueado=0;//Indica que existiendo lectores hay un escritor esperando.









  /* Avisamos del bloqueo a los demás 
     printf("[Pagos %u NODE: %u]Avisando del bloqueo\n",myID,nodo);

     send_bloqueo(nodo);
  */








  enviarVariables(msqid_var,&variables,1);
  pagos();//Hacemos lo que tengamos que hacer 
  /* Volvemos a actualizar las variables */

  variables = actualizarVariables(msqid_var,2,TIPO_PAGOS);
  /* Si en nuestro nodo no hay más pagos enviamos el permiso */
  
  if(variables.num_pagos>0){
    
    enviado=1;
    sendTestigo(nodo,&test);
    //send_permiso_pagos(msqid_mutex,&test);
  }
  else{
    printf("[Pagos %u NODE: %u]Comprobando pagos otros nodos.\n",myID,nodo);
    pintarVariables(&variables);
    int otro=checkOtroNodo(variables.peticionesPagos,variables.atendidasPagos,nodo);
    printf("Valor de otro %d\n",otro);
    if (otro>=0){
      if(otro!=nodo){
	    variables.testigo=0;
	  }
      variables.dentro=0;
      enviado=1;
       if(otro!=nodo){
	    variables.testigoLibre=0;
	  }else{
	variables.testigoLibre=1;
      }
       printf("[Pagos %u NODE: %u]Enviando testigo al nodo %u (Detectado pagos).\n",myID,nodo,otro);

      sendTestigo(otro,&test);
    }else{
      if(variables.num_anulaciones>0){
	enviado=1;
 

	sendTestigo(nodo,&test);
	//send_permiso_anulaciones(msqid_mutex,&test);
      }else{
	printf("[Pagos %u NODE: %u]Comprobando anulaciones otros nodos.\n",myID,nodo);

	int otro=checkOtroNodo(variables.peticionesAnulaciones,variables.atendidasAnulaciones,nodo);
	if(otro>=0){
	  if(otro!=nodo){
	    variables.testigo=0;
	  }
	  variables.dentro=0;
	  enviado=1;
	   if(otro!=nodo){
	    variables.testigoLibre=0;
	  }else{
	variables.testigoLibre=1;
      }
printf("[Pagos %u NODE: %u]Enviando testigo al nodo %u (Detectado anulaciones).\n",myID,nodo,otro);
	  sendTestigo(otro,&test);
	}else{
	  if(variables.num_lectores>0){
	    enviado=1;
	    sendTestigo(nodo,&test);
	    //send_permiso_lectores(msqid_mutex,&test);
	  }else{
	    printf("[Pagos %u NODE: %u]Comprobando lectores otros nodos.\n",myID,nodo);

	    int otro=checkOtroNodo(variables.peticionesLectores,variables.atendidasLectores,nodo);
	    if(otro>=0){
	      if(otro!=nodo){
		variables.testigo=0;
	      }
	      variables.dentro=0;
	      enviado=1;
	             if(otro!=nodo){
	    variables.testigoLibre=0;
	  }else{
	variables.testigoLibre=1;
      }printf("[Pagos %u NODE: %u]Enviando testigo al nodo %u (Detectado Lector).\n",myID,nodo,otro);
	      sendTestigo(otro,&test);
	    }
	  }
	}
      }
    }
  }
  variables.dentro=0;
  if (enviado==0){
    variables.testigoLibre=1;

    enviarVariables(msqid_var,&variables,1);
   
    enviarTestigoNodo (msqid_peticiones,&test);
  }else{
    enviarVariables(msqid_var,&variables,1);
  } 
  return 0;
}


