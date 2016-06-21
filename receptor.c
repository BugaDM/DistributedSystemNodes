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
/******************************************
 *         PROTOTIPOS                     *
 ******************************************/

struct vars control_peticion(struct envio*,struct vars*);
struct vars control_bloqueo(struct bloqueo*,struct vars*);
struct vars control_testigo(struct testigo*,struct vars*);
struct testigo var2test(struct vars*);
struct vars guardarTestigo(struct vars*);
int escritoresPendientes(struct vars*);
int peticionLectura(struct vars*);

int recibe_mensaje(struct envio*);
int comprobador(struct vars*);
int comprobarTurno(struct testigo*,struct vars*);

int nodo;
int msqid_mutex;
int msqid_peticiones;
int msqid_var;
int enviado;
int main(int argc, char *argv[]){
  nodo=atoi(argv[1]);
  struct cola colas;
  enviado=0;

  /* Una vez sabemos a que nodo pertenece este proceso,
     se procede a "localizar" las tres colas que va a utilizar,
     que son ID_NODO,ID_NODO+1,ID_NODO+2
  */
  colas = iDColaNodo(nodo);
 
  
  /* INICIALIZAMOS LAS 3 COLAS */

  msqid_var=colas.msqid_vars;
  msqid_mutex=colas.msqid_mutex;
  msqid_peticiones=colas.msqid_peticiones;

  printf("[Listener NODO:%u] msqid_var:%u\n",nodo,msqid_var);
  printf("[Listener NODO:%u] msqid_mutex:%u\n",nodo,msqid_mutex);
  printf("[Listener NODO:%u] msqid_peticiones:%u\n",nodo,msqid_peticiones);
  while(1){
    struct envio copy;
    printf("[Listener NODO:%u] esperando petición en %u.\n",nodo,msqid_peticiones);sleep(1);
    if(msgrcv(msqid_peticiones,&copy,sizeof(struct envio)-sizeof(long),0,0) <0 ){
      perror("Error recibiendo permiso\n");
      exit(1);
    }
    printf("Ha llegado una peticion\n");
    // getchar();
    
    printf("++++++++++++++++++++++++++++++++++++++++++++++++++#\n");
    printf("###########################################################\n\n");

    pintarTestigo((struct testigo*)&copy);
    printf("###########################################################\n");

    printf("++++++++++++++++++++++++++++++++++++++########\n\n");
    int tipo= copy.mtype;

    struct vars variables=actualizarVariables(msqid_var,2,0);

    int z;
    if(tipo == 5 && variables.testigoLibre==0){
      for (z=0;z<NODOS;z++){
	/* Pone las peticiones de las variables en el testigo */
	copy.campo3[z]=variables.peticionesPagos[z];
	copy.campo4[z]=variables.peticionesAnulaciones[z];
	copy.campo5[z]=variables.peticionesLectores[z];
	/* Pone las atendidas del testigo en las variables */
	variables.atendidasPagos[z]=copy.campo6[z];
	variables.atendidasAnulaciones[z]=copy.campo7[z];
	variables.atendidasLectores[z]=copy.campo8[z];

      }
    }
    printf("9999999999999999999999999999999999999999999999999#\n");
    pintarVariables(&variables);
    printf("99999999999999999999999999999999999999999#\n");
    switch(tipo){
    case 1:
    case 2:
    case 3:
      printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
      printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
      printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
      variables=control_peticion((struct envio*)&copy,&variables);
      break;
    case 4:
      printf("##################################################\n");
      printf("##################################################\n");
      printf("##################################################\n");
      printf("##################################################\n");
      variables=control_bloqueo((struct bloqueo*)&copy,&variables);
      break;
    case 5:
      sleep(3);
      printf("=================================================\n");
      printf("=================================================\n");
      printf("=================================================\n");
      variables=control_testigo((struct testigo*)&copy,&variables);
      break;
  
    }

    pintarVariables(&variables);

    variables=checkLeyendo(&variables);
    enviarVariables(msqid_var,&variables,2);

  }
}
struct vars control_peticion(struct envio* pet,struct vars* v){
  int tipo=pet->mtype;printf("Pues resulta que aqui el tipo es %u\n",tipo);
  int dir_nodo=pet->campo1;
  int id_peticion=pet->campo2;printf("Aqui el id es %u\n",id_peticion);
  

  switch(tipo){
  case 1:
    v->peticionesPagos[dir_nodo]=id_peticion;
    if (v->leyendo==1){
      v->bloqueado=1;}
    break;
  case 2:
    v->peticionesAnulaciones[dir_nodo]=id_peticion;
    if (v->leyendo==1){
      v->bloqueado=1;}
    break;
  case 3:
    v->peticionesLectores[dir_nodo]=id_peticion;
    if(escritoresPendientes(v)){
      v->bloqueado=1;}
    break;
  }
  int checker=comprobador(v);
  struct testigo test;
  printf("El actual valor de checker es %d\n\n",checker);
  pintarVariables(v);
  printf("\n\n\n");
  int testigo=v->testigo;
  if(checker>=0 && testigo==1 && v->dentro==0){
    if(checker!=nodo){
      v->testigo=0;
    }
    test = var2test(v);
    printf("Testigo generado\n");
    pintarTestigo(&test);
    // if(tipo==3){
      //   test.lector=1;
    //  }
    if(checker!=nodo){
      v->testigoLibre=0;
    }
    sendTestigo(checker,&test);
  }else{
    int result = peticionLectura(v);
    if(v->bloqueado==1 && result>=0 && v->k<K_MAX && testigo ==1){
      v->k=v->k+1;
      if(result!=nodo){
	v->testigo=0;
      }
      test.lector=1;
      test = var2test(v);
      if(checker!=nodo){
	v->testigoLibre=0;
      }
      sendTestigo(result,&test);
      
    }else{
      if(v->leyendo==1 && result>=0 && testigo==1){
	if(result!=nodo){

	  v->testigo=0;
	}

	//v->testigoLibre=0; printf("################## 173 ##############\n");
    
	/* Igualamos la atendida a la petición para indicar que nos han atendido */
	/* Actualizamos vars con esta información */
	pet->campo8[nodo]=pet->campo5[nodo];
	v->atendidasLectores[nodo]=pet->campo8[nodo];
	printf("&&&&&&&&&&&&&&&&&&&&&&&&&&&");
	//	sendTestigo(nodo,&test);
	send_permiso_lectores(msqid_mutex,&test);

      }
    }
  }

  
  return *v;

}
struct vars control_testigo(struct testigo* test,struct vars* v){
  int i;
  v->testigo=1;
  

  for(i=0;i<NODOS;i++){
    test->pagos[i]=v->peticionesPagos[i];
    test->anulaciones[i]=v->peticionesAnulaciones[i];
    test->lectores[i]=v->peticionesLectores[i];
  }
  if (v->num_pagos>0 && test->lector == 0){
    enviado=1;
    test->atendidasPagos[nodo]=test->pagos[nodo];
    v->atendidasPagos[nodo]=test->pagos[nodo];
    v->testigoLibre=0;
    send_permiso_pagos(msqid_mutex,test);

  }else{if (v->num_anulaciones>0 && test->lector == 0){
      enviado=1;
      test->atendidasAnulaciones[nodo]=test->anulaciones[nodo];
      v->atendidasAnulaciones[nodo]=test->anulaciones[nodo];
      v->testigoLibre=0;
      send_permiso_anulaciones(msqid_mutex,test);

    }else{if (v->num_lectores>0){
	enviado=1;
	v->atendidasLectores[nodo]=test->lectores[nodo];
	test->atendidasLectores[nodo]=test->lectores[nodo];
	v->testigoLibre=0;
	send_permiso_lectores(msqid_mutex,test);

      }

    }

  }

  if(enviado == 0 && test->lector == 0){
    int turno=comprobarTurno(test,v);
    printf("El valor de turno es %d",turno);
    if(turno>=0){
      if(turno!=nodo){    
	v->testigo=0;
      }
      enviado=1;
      if(turno!=nodo){
	v->testigoLibre=0;
      }
      sendTestigo(turno,test);
    }else {

      if(v->num_pagos>0 && test->lector == 0){
	enviado=1;
	//v->testigoLibre=0; printf("################## 235 ##############\n");
	/* Igualamos la atendida a la petición para indicar que nos han atendido */
	/* Actualizamos vars con esta información */
	test->atendidasPagos[nodo]=test->pagos[nodo];
	v->atendidasPagos[nodo]=test->pagos[nodo];
	send_permiso_pagos(msqid_mutex,test);

      }else{ if(v->num_anulaciones>0 && test->lector == 0){
	  enviado=1;
	  //v->testigoLibre=0; printf("################## 244 ##############\n");
	  /* Igualamos la atendida a la petición para indicar que nos han atendido */
	  /* Actualizamos vars con esta información */
	  test->atendidasAnulaciones[nodo]=test->anulaciones[nodo];
	  v->atendidasAnulaciones[nodo]=test->anulaciones[nodo];
	  send_permiso_anulaciones(msqid_mutex,test);
	}else{
	  if(v->num_lectores>0){
	    enviado=1;
	    //v->testigoLibre=0;printf("################## 253 ##############\n");
	    /* Igualamos la atendida a la petición para indicar que nos han atendido */
	    /* Actualizamos vars con esta información */
	    test->atendidasLectores[nodo]=test->lectores[nodo];
	    v->atendidasLectores[nodo]=test->lectores[nodo];
	    send_permiso_lectores(msqid_mutex,test);
	  }
	}
      }
    }
  }
  printf("El valor de enviado es: %u\n",enviado);
  if (enviado==0){
    *v=guardarTestigo(v);
  }
  return *v;
}

struct vars control_bloqueo(struct bloqueo* bloqueo,struct vars* v){
  int acabado=bloqueo->acabado;
  int id_nodo=bloqueo->id_nodo;
  int i=0;
  int finalizado;

  if(acabado==1){
    v->lectores_finalizados[id_nodo]=1;
    v->leyendo=0;
    printf("\t\t\tLO PONGO A CERO\n");
    for(i=0;i<NODOS;i++){
      if(v->lectores_finalizados[i]!=1){
	v->leyendo=1;
	printf("\t\t\tLO PONGO A UNO 2.0\n");
      }
    }
  }else{
    v->lectores_finalizados[id_nodo]=0;
printf("\t\t\tLO PONGO A UNO\n");
    v->leyendo=1;

  }

  return *v;
}

int comprobador(struct vars* v){
  int i=0;
  for(i=0;i<NODOS;i++){
    if(v->peticionesPagos[i]!=v->atendidasPagos[i]){
      printf("HE DEVUELTO %u\n",i);
      return i;
    }
    if(v->peticionesAnulaciones[i]!=v->atendidasAnulaciones[i]){
      printf("HE DEVUELTO %u\n",i);
      return i;
    }
    if(v->peticionesLectores[i]!=v->atendidasLectores[i]){
      printf("HE DEVUELTO %u\n",i);
      return i;
    }
      
  }
  printf("HE DEVUELTO %u, es decir -1\n",i);
  return -1;
}
int peticionLectura(struct vars* v){
  int i=0;
  for(i=0;i<NODOS;i++){
    if(v->peticionesLectores[i]!=v->atendidasLectores[i]){return i;}

  }
  return -1;
}

int comprobarTurno(struct testigo* test,struct vars* v){


  if(test->pagos[nodo]!=test->atendidasPagos[nodo]){return -1;}
  int i;
  for(i=0;i<NODOS;i++){
    if (i!=nodo){
      if(test->pagos[i]!=test->atendidasPagos[i]){
	return i;}
    }
  }

  if(test->anulaciones[nodo]!=test->atendidasAnulaciones[nodo]){return -1;}
  for(i=0;i<NODOS;i++){
    if (i!=nodo){
      if(test->anulaciones[i]!=test->atendidasAnulaciones[i]){
	return i;}
    }
  }


  if(test->lectores[nodo]!=test->atendidasLectores[nodo]){return -1;}
  for(i=0;i<NODOS;i++){
    if (i!=nodo){
      if(test->lectores[i]!=test->atendidasLectores[i]){
	return i;}
    }
  }
  return -1;
}

int escritoresPendientes(struct vars* v ){
  int i=0;
  for(i=0;i<NODOS;i++){
    if(v->peticionesAnulaciones[i]!=v->atendidasAnulaciones[i]){return 1;}
    if(v->peticionesPagos[i]!=v->atendidasPagos[i]){return 1;}
  }

  return 0;
}
struct testigo var2test(struct vars* v){
  struct testigo test;
  test.mtype=5;
  test.k=v->k;
  test.lector=0;
  int i = 0;
  for(i=0;i<NODOS;i++){
    test.pagos[i]=v->peticionesPagos[i];
    test.anulaciones[i]=v->peticionesAnulaciones[i];
    test.lectores[i]=v->peticionesLectores[i];
    test.atendidasPagos[i]=v->atendidasPagos[i];
    test.atendidasAnulaciones[i]=v->atendidasAnulaciones[i];
    test.atendidasLectores[i]=v->atendidasLectores[i];
  }
  return test;
}



struct vars guardarTestigo(struct vars* v){
  
  struct envio copy;
  v->testigo=1;
  printf("\t\t%u lo pongo a 1\n",v->testigo);
  v->testigoLibre=1;
  enviarVariables(msqid_var,v,2);
  
  printf("[Listener NODO:%u ]Testigo en el nodo,esperando petición.\n",nodo);


  if(msgrcv(msqid_peticiones,&copy,sizeof(struct envio)-sizeof(long),0,0) <0 ){
    perror("Error recibiendo permiso\n");
    exit(1);
  }

  printf("Peticion direccion nodo %u, id peticion %u\n",copy.campo1,copy.campo2);
  int tipo=copy.mtype;printf("El tipo es %u\n",tipo);
  int dir_nodo= copy.campo1;
  int id_peticion = copy.campo2;
  /*
  switch(tipo){
  case 1:
    v->peticionesPagos[dir_nodo]=id_peticion;
    if (v->leyendo==1){
      v->bloqueado=1;}
    break;
  case 2:
    v->peticionesAnulaciones[dir_nodo]=id_peticion;
    if (v->leyendo==1){
      v->bloqueado=1;}
    break;
  case 3:
    printf("El tipo era 3\n");
    v->peticionesLectores[dir_nodo]=id_peticion;
    if(escritoresPendientes(v)){
      v->bloqueado=1;}
    break;
    }*/

  *v=actualizarVariables(msqid_var,2,0);

  printf("\t123123132131321321231231231321321321\n");
  pintarVariables(v);
  *v=control_peticion(&copy,v);
  return *v;
    
    

}

