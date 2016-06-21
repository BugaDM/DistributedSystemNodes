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
  int envVar=0;
  int leyendo=0;
  int nodo=atoi(argv[1]);
  int myID=atoi(argv[1])*100+atoi(argv[2]);
  printf("[Lectores:%u]Pertenezco al nodo [%u]\n",myID,nodo);
  /* Una vez sabemos a que nodo pertenece este proceso,
     se procede a "localizar" las tres colas que va a utilizar,
     que son ID_NODO,ID_NODO+1,ID_NODO+2
  */
  colas = iDColaNodo(nodo);
 
  
  /* INICIALIZAMOS LAS 3 COLAS */

  int msqid_var=colas.msqid_vars;
  int msqid_mutex=colas.msqid_mutex;
  int msqid_peticiones=colas.msqid_peticiones;
  printf("[Lectores:%u NODE:%u]msqid_var-> %u\n",myID,nodo,msqid_var);
  printf("[Lectores %u NODE: %u]msqid_mutex-> %u\n",myID,nodo,msqid_mutex);
  printf("[Lectores %u NODE: %u]msqid_peticiones-> %u\n",myID,nodo,msqid_peticiones);
  
  /* Una vez inicializamos las 3 colas pasamos a recoger
     las variables que nos permiten comunicarnos con el resto 
     de nodos del sistema.*/
  struct vars variables;

 
  esperar(myID);//hago otras cosas

  printf("[Lector:%u NODE:%u]Actualizando variables\n",myID,nodo);
  printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ Primera toma de variables\n");
  variables=actualizarVariables(msqid_var,1,TIPO_LECTOR);
  envVar=0;

  //getchar();

  //Si no tenemos el testigo, y no hay más procesos de pagos
  //en este nodo que quieran entrar,



  //Send peticion & wait
  if((variables.num_lectores-variables.num_lectoresDentro)>=1&&variables.num_lectoresDentro>0){  
    printf("Entro en la primera parte del if\n");
    //getchar();
    printf("[Lector:%u NODE:%u]Liberando variables\n",myID,nodo);
    if(envVar==0){
      envVar=1;
      enviarVariables(msqid_var,&variables,1);
    }
    /* Solicitamos la exclusion mutua */
    printf("++++++++++[Lector:%u NODE:%u]Pidiendo permiso con alguien dentro\n",myID,nodo);
    send_peticion(nodo,3);
    test=receive_permiso_lectores(msqid_mutex);
  }else{
    if(variables.testigoLibre==1){
      printf("Entro en la segunda parte del if\n");
      //getchar();
      send_peticion(nodo,3);
      if(envVar==0){
	envVar=1;
	enviarVariables(msqid_var,&variables,1);
      }
    }
    else{
      if(variables.testigo==1 && variables.bloqueado==1){
	printf("Entro en la tercera parte del if\n");
	//getchar();



	if(variables.k==K_MAX){
	  printf("[Lector:%u NODE:%u]Liberando variables\n",myID,nodo);
	  if(envVar==0){
	    envVar=1;
	    enviarVariables(msqid_var,&variables,1);
	  }
	  /* Solicitamos la exclusion mutua */
	  printf("[Lector:%u NODE:%u]Pidiendo permiso\n",myID,nodo);

	  test=receive_permiso_lectores(msqid_mutex);

	}else{
	  variables.k++;
	  printf("[Lector:%u NODE:%u]Liberando variables\n",myID,nodo);
	  if(envVar==0){
	    envVar=1;
	    enviarVariables(msqid_var,&variables,1);
	  }

	}
	  
      }else{if(variables.leyendo==1){
	  envVar=1;
	  
	  enviarVariables(msqid_var,&variables,1);
	  send_peticion(nodo,3);
	
	}else{
	  if(variables.num_lectores==1){
	    envVar=1;
	  
	    enviarVariables(msqid_var,&variables,1);
	    send_peticion(nodo,3);
	
	

	  }
	}
	}
      }
      if(envVar==0){
	envVar=1;
	enviarVariables(msqid_var,&variables,1);
      }
      test=receive_permiso_lectores(msqid_mutex);
    }
      
    


 



    /*---------------------------------------------------------------
      -------------------------------------------------------------------
      -------------------------------------------------------------------
      ------------------------------------------------------------------- */  
    printf("[Lector:%u NODE:%u]Actualizando variables\n",myID,nodo);

    variables = actualizarVariables(msqid_var,2,TIPO_LECTOR); // El 2 indica no modificar num_lectores
    envVar=0;
    /* Actualizamos las variables, ya hemos sido atendidos */


    variables.atendidasLectores[nodo]=variables.peticionesLectores[nodo];
    test.atendidasLectores[nodo]=test.lectores[nodo];
    variables.leyendo=1;
    variables.testigoLibre=0;
    variables.dentro=1;
    variables.num_lectoresDentro++;
    variables.testigo=1;
    variables.lectores_finalizados[nodo]=0;
    /* Avisamos del lectura a los demás */
    printf("[Lector:%u NODE:%u]Avisando de lectura\n",myID,nodo);

    if(leyendo==0){
      send_lectura(nodo);
    }

    pintarVariables(&variables);


    if(variables.bloqueado==1 && variables.k<K_MAX && enviado ==0){
      if(variables.num_lectoresDentro<variables.num_lectores){
	variables.k++;
	test.k=variables.k;
	enviado=1;
	test.lector=1;
	if(variables.num_lectores==0)
	  {send_finLectura(nodo);}
	sendTestigo(nodo,&test);
      }else{
	variables.k++;
	int otroo=checkOtroNodo(variables.peticionesLectores,variables.atendidasLectores,nodo);
	if(otroo>=0 && enviado ==0){
	  test.k=variables.k;
	  variables.testigo=0;
	  enviado=1;
	  test.lector=1;
	  if(variables.num_lectores==0)
	    {send_finLectura(nodo);}
	  sendTestigo(nodo,&test);
	}
      }
    }else{
      if(variables.num_lectoresDentro<variables.num_lectores && enviado ==0){
	test.k=variables.k;
	enviado=1;
	test.lector=1;
	if(variables.num_lectores==0)
	  {send_finLectura(nodo);}
	sendTestigo(nodo,&test);
      }else{
	int otroo=checkOtroNodo(variables.peticionesLectores,variables.atendidasLectores,nodo);
	if(otroo>=0 && enviado ==0){
	  test.k=variables.k;
	  variables.testigo=0;
	  enviado=1;
	  test.lector=1;
	  if(variables.num_lectores==0)
	    {send_finLectura(nodo);}
	  sendTestigo(otroo,&test);
	}else{
	
	  int otroo=checkOtroNodo(variables.peticionesLectores,variables.atendidasLectores,nodo);
	  if(otroo>=0 && enviado ==0){
	    test.k=variables.k;
	    variables.testigo=0;
	    enviado=1;
	    test.lector=1;
	    if(variables.num_lectores==0)
	      {send_finLectura(nodo);}
	    sendTestigo(nodo,&test);
	  }	
	}
      }
    }





    printf("[Lector:%u NODE:%u]Liberando variables debuuggggeeerrr\n",myID,nodo);

 
    if(envVar==0){
      envVar=1;
      enviarVariables(msqid_var,&variables,1);
    }
    lector();//Hacemos lo que tengamos que hacer 
    /* Volvemos a actualizar las variables */
    printf("[Lector:%u NODE:%u]Actualizando variables\n",myID,nodo);

    variables = actualizarVariables(msqid_var,0,TIPO_LECTOR);
    envVar=0;
    /* Si en nuestro nodo no hay más pagos enviamos el permiso */
    printf("[Lector:%u NODE:%u]Realizando comprobaciones\n",myID,nodo);
    
    variables.num_lectoresDentro--;


    if(variables.num_pagos>0 && variables.num_lectoresDentro==0&& enviado ==0){
      printf("Entro en el if 1\n");
      enviado=1;
      if(variables.num_lectores==0)
	{send_finLectura(nodo);}
      variables=checkLeyendo(&variables);
		if(variables.leyendo==0){
		  sendTestigo(nodo,&test);}
    }else{
      int otro=checkOtroNodo(variables.peticionesPagos,variables.atendidasPagos,nodo);
   
      if (otro>=0 && variables.num_lectoresDentro==0){
	printf("Entro en el if 2\n");
	if(otro!=nodo&& enviado ==0){
	  variables.testigo=0;
	}
	enviado=1;
	if(otro!=nodo){
	  variables.testigoLibre=0;
	}else{
	  //variables.testigoLibre=1;
	}
	if(variables.num_lectoresDentro==0){
	  variables.dentro=0;

	}
	printf("[Lectores %u NODE: %u]Enviando testigo al nodo %u (Detectado pagos).\n",myID,nodo,otro);

	if(variables.num_lectores==0)
	  {send_finLectura(nodo);}
	variables=checkLeyen2(&variables,nodo);
		if(variables.leyendo==0){
		  sendTestigo(otro,&test);
		}
      }else{
	if(variables.num_anulaciones>0&& variables.num_lectoresDentro==0&& enviado ==0){
	  enviado=1;
	  if(variables.num_lectores==0)
	    {send_finLectura(nodo);}
	  variables=checkLeyen2(&variables,nodo);
		if(variables.leyendo==0){
		  sendTestigo(otro,&test);}
	}else{
	  int otro=checkOtroNodo(variables.peticionesAnulaciones,variables.atendidasAnulaciones,nodo);
	  if(otro>=0 && variables.num_lectoresDentro==0){
	    printf("Entro en el if 4\n");
	    if(otro!=nodo&& enviado ==0){
	      variables.testigoLibre=0;
	    }else{
	      //variables.testigoLibre=1;
	    }
	    if(variables.num_lectoresDentro==0){
	      variables.dentro=0;
	      variables.leyendo=0;
	    }
	    enviado=1;
	    printf("[Lectores %u NODE: %u]Enviando testigo al nodo %u (Detectado anulaciones).\n",myID,nodo,otro);
	    if(variables.num_lectores==0)
	      {send_finLectura(nodo);}
	    variables=checkLeyen2(&variables,nodo);
		if(variables.leyendo==0){
		  sendTestigo(otro,&test);}
	  }else{
	    if((variables.num_lectores-variables.num_lectoresDentro)>0&& enviado ==0){
	    
	      enviado=1;
	      if(variables.num_lectores==0)
		{send_finLectura(nodo);}
	      sendTestigo(nodo,&test);
	    }else{
	      printf("[Lectores %u NODE: %u]Comprobando lectores otros nodos.\n",myID,nodo);
	      int otro=checkOtroNodo(variables.peticionesLectores,variables.atendidasLectores,nodo);
test.lector=1;
	      if(otro>=0 && enviado==0){
		printf("Entro en el if 1\n");
		if(otro!=nodo){
		  variables.testigo=0;
		}
		if(variables.num_lectoresDentro==0){
		  variables.dentro=0;
		  variables.leyendo=0;
		}
		enviado=1;
		if(otro!=nodo&& enviado ==0){
		  variables.testigoLibre=0;
		}else{
		  //variables.testigoLibre=1;
		}printf("[Lectores %u NODE: %u]Enviando testigo al nodo %u (Detectado Lector).\n",myID,nodo,otro);
		if(variables.num_lectores==0)
		  {send_finLectura(nodo);}
		  sendTestigo(otro,&test);
	      }
	    }
	  }
	}
      }
    }
    if(variables.num_lectoresDentro==0){
      variables.dentro=0;
      //variables.leyendo=0;
    }
    if(enviado == 0 && variables.num_lectores == 0){
      send_finLectura(nodo);
    }
    if (enviado==0 && variables.num_lectoresDentro==0){
      variables.testigoLibre=1;

      if(envVar==0){
	envVar=1;
	enviarVariables(msqid_var,&variables,1);
      }
   
variables=checkLeyendo(&variables);
		if(variables.leyendo==0){
		 
		  enviarTestigoNodo (msqid_peticiones,&test);}
    }else{
      if(envVar==0){
	envVar=1;
	enviarVariables(msqid_var,&variables,1);
      }
    } 
    return 0;
  }


