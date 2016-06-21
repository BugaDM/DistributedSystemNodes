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
  /***************************************
   *          TESTIGO                    *
   ***************************************/
  struct testigo test;
  struct cola colas;
  int enviado=0;
  int nodo=atoi(argv[1]);
  int myID=atoi(argv[1])*100+atoi(argv[2]);
  printf("[Anulaciones:%u]Pertenezco al nodo [%u]\n",myID,nodo);
  /* Una vez sabemos a que nodo pertenece este proceso,
     se procede a "localizar" las tres colas que va a utilizar,
     que son ID_NODO,ID_NODO+1,ID_NODO+2
  */
  colas = iDColaNodo(nodo);
 
  
  /* INICIALIZAMOS LAS 3 COLAS */

  int msqid_var=colas.msqid_vars;
  int msqid_mutex=colas.msqid_mutex;
  int msqid_peticiones=colas.msqid_peticiones;
  printf("[Anulaciones:%u NODE:%u]msqid_var-> %u\n",myID,nodo,msqid_var);
  printf("[Anulaciones %u NODE: %u]msqid_mutex-> %u\n",myID,nodo,msqid_mutex);
  printf("[Anulaciones %u NODE: %u]msqid_peticiones-> %u\n",myID,nodo,msqid_peticiones);
  
  /* Una vez inicializamos las 3 colas pasamos a recoger
     las variables que nos permiten comunicarnos con el resto 
     de nodos del sistema.*/
  struct vars variables;

 
  esperar(myID);//hago otras cosas

  variables=actualizarVariables(msqid_var,1,TIPO_ANULACIONES);
  //Tenemos que acordarnos que una vez salgamos de la
  //zona de mutex hay que hacer actualizarVariables(msqid_var,0)
  //para indicar que este thread ya no quiere entrar más.
  
  //Si no tenemos el testigo, y no hay más procesos de pagos
  //en este nodo que quieran entrar,
  if(variables.num_anulaciones<=1){
    printf("[Anulaciones:%u NODE:%u]Enviando peticiones al resto de nodos.\n",myID);

    send_peticion(nodo,TIPO_ANULACIONES);

  }else{
    if(variables.testigoLibre==1){
      send_peticion(nodo,TIPO_ANULACIONES);}
  }
  enviarVariables(msqid_var,&variables,1);

  /* Solicitamos la exclusion mutua */
  printf("[Anulaciones:%u NODE:%u]Pidiendo permiso COLA[%u]\n",myID,nodo,msqid_mutex);

  test=receive_permiso_anulaciones(msqid_mutex);
  /* Tenemos la exclusión mutua.
   */  

  variables = actualizarVariables(msqid_var,0,TIPO_ANULACIONES); // El 2 indica no modificar num_pagos
  /* Actualizamos las variables, ya hemos sido atendidos */
  variables.testigo=1;
  variables.testigoLibre=0;
  variables.dentro=1;
  variables.k=0;
  test.k=variables.k;
test.lector=0;
  variables.bloqueado=0;//Indica que existiendo lectores hay un escritor esperando.



  enviarVariables(msqid_var,&variables,1);
  anulaciones();//Hacemos lo que tengamos que hacer 
  /* Volvemos a actualizar las variables */

  variables = actualizarVariables(msqid_var,2,TIPO_ANULACIONES);
  /* Si en nuestro nodo no hay más pagos enviamos el permiso */
    
  if(variables.num_pagos>0){
    enviado=1;
    sendTestigo(nodo,&test);
    //send_permiso_pagos(msqid_mutex,&test);
  }
  else{
    printf("[Anulaciones %u NODE: %u]Comprobando pagos otros nodos.\n",myID,nodo);
    int otro=checkOtroNodo(variables.peticionesPagos,variables.atendidasPagos,nodo);
   
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
       printf("[Anulaciones %u NODE: %u]Enviando testigo al nodo %u (Detectado pagos).\n",myID,nodo,otro);

      sendTestigo(otro,&test);
    }else{
      if(variables.num_anulaciones>0){
	enviado=1;
	sendTestigo(nodo,&test);
	//send_permiso_anulaciones(msqid_mutex,&test);
      }else{
	printf("[Anulaciones %u NODE: %u]Comprobando anulaciones otros nodos.\n",myID,nodo);

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

       printf("[Anulaciones %u NODE: %u]Enviando testigo al nodo %u (Detectado anulaciones).\n",myID,nodo,otro);

	  sendTestigo(otro,&test);
	}else{
	  if(variables.num_lectores>0){
	    enviado=1;
	    sendTestigo(nodo,&test);
	    //send_permiso_lectores(msqid_mutex,&test);
	  }else{
	    printf("[Anulaciones %u NODE: %u]Comprobando lectores otros nodos.\n",myID,nodo);

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
      }
       printf("[Anulaciones %u NODE: %u]Enviando testigo al nodo %u (Detectado lector).\n",myID,nodo,otro);

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
    printf("******************** ELECTROLATINO %u *******************",variables.testigo);
    enviarTestigoNodo (msqid_peticiones,&test);

  }else{
    enviarVariables(msqid_var,&variables,1);
  }
  return 0;
}


