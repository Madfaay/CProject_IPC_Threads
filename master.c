#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "utils.h"
#include "myassert.h"
#include "config.h"

#include "client_master.h"
#include "master_worker.h"




#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/************************************************************************
 * Données persistantes d'un master
 ************************************************************************/
 
static int compteur = 0 ;
typedef struct
{

   int openRes ;
   int accuse ;
   int fds_To_Master[2] ;
   int fds_To_Worker[2] ;
   int fdWorker_To_Master[2];
    
    // infos pour le travail à faire (récupérées sur la ligne de commande)
    int order;     // ordre de l'utilisateur (cf. CM_ORDER_* dans client_master.h)
    float elt;     // pour CM_ORDER_EXIST, CM_ORDER_INSERT, CM_ORDER_LOCAL
    int nb;        // pour CM_ORDER_INSERT_MANY, CM_ORDER_LOCAL
    float min;     // pour CM_ORDER_INSERT_MANY, CM_ORDER_LOCAL
    float max;     // pour CM_ORDER_INSERT_MANY, CM_ORDER_LOCAL
    int nbThreads; // pour CM_ORDER_LOCAL
} Data;


/************************************************************************
 * Usage et analyse des arguments passés en ligne de commande
 ************************************************************************/
static void usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usage : %s\n", exeName);
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}


/************************************************************************
 * initialisation complète
 ************************************************************************/
void init(Data *data)
{
    myassert(data != NULL, "il faut l'environnement d'exécution");
  


}


/************************************************************************
 * fin du master
 ************************************************************************/
void orderStop(Data *data)
{
    TRACE0("[master] ordre stop\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");
    if(compteur==0)
    {
      int reponse = CM_ANSWER_STOP_OK ;
	  int write_res = write(data->openRes , &reponse  , sizeof(int) );
	  myassert(write_res != -1 , " ") ;
	  
    }
    
    else
    {
		int order = CM_ORDER_STOP;
		int write_res = write(data->fds_To_Worker[1] , &order , sizeof(int)) ;
		myassert(write_res != -1 ," ") ;
		wait(NULL) ;

	}
    
  
    //TODO
    // - traiter le cas ensemble vide (pas de premier worker)
    // - envoyer au premier worker ordre de fin (cf. master_worker.h)
    // - attendre sa fin
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    //END TODO
}


/************************************************************************
 * quel est la cardinalité de l'ensemble
 ************************************************************************/
void orderHowMany(Data *data)
{
    TRACE0("[master] ordre how many\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");
    int write_res ;
    int nb_elements = 0;
    int nb_all_elements = 0 ;
    if(compteur==0)
    {
		write_res = write(data->openRes , &nb_all_elements , sizeof(int)) ;
		myassert(write_res!=0 , " ") ;
		write_res = write(data->openRes , &nb_elements , sizeof(int)) ;
		myassert(write_res!=0 , " ") ;

	}
	   else
    {
    	int order = CM_ORDER_HOW_MANY;
		int write_res = write(data->fds_To_Worker[1] , &order , sizeof(int)) ;
		myassert(write_res != -1 ," ") ;
		int read_res = read(data->fds_To_Master[0] , &nb_elements , sizeof(int)) ;
			    myassert(read_res != -1 ," ") ;
			read_res = read(data->fds_To_Master[0] , &nb_all_elements , sizeof(int)) ;
			    myassert(read_res != -1 ," ") ;
	     	write_res = write(data->openRes , &nb_all_elements , sizeof(int)) ;
			myassert(write_res!=0 , " ") ;
			write_res = write(data->openRes , &nb_elements , sizeof(int)) ;
			myassert(write_res!=0 , " ") ;
			printf("je print le resultat a master de how many nbdifferents : %d , nbtotal : %d \n " , nb_elements , nb_all_elements) ;
			
    
    }

    //TODO
    // - traiter le cas ensemble vide (pas de premier worker)
    // - envoyer au premier worker ordre howmany (cf. master_worker.h)
    // - recevoir accusé de réception venant du premier worker (cf. master_worker.h)
    // - recevoir résultats (deux quantités) venant du premier worker
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    // - envoyer les résultats au client
    //END TODO
}


/************************************************************************
 * quel est la minimum de l'ensemble
 ************************************************************************/
void orderMinimum(Data *data)
{
    TRACE0("[master] ordre minimum\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");
 if(compteur==0)
    {
    
    int reponse = CM_ANSWER_MINIMUM_EMPTY  ;
	int write_res = write(data->openRes , &reponse  , sizeof(int) );
	myassert(write_res != -1 , " ") ;
    	
    
    }	
    
    else
    
    {
    	int order = CM_ORDER_MINIMUM;
		int write_res = write(data->fds_To_Worker[1] , &order , sizeof(int)) ;
		    myassert(write_res != -1 ," ") ;
		float rep ;
		int read_res = read(data->fdWorker_To_Master[0] , &rep , sizeof(float)) ;
			myassert(read_res != -1 ," ") ;
		    printf("je print le reponse de min a master %f \n", rep) ;
	        write_res = write(data->openRes , &rep  , sizeof(float) );
		    myassert(write_res != -1 , " ") ;

    	
    }
    //TODO
    // - si ensemble vide (pas de premier worker)
    //       . envoyer l'accusé de réception dédié au client (cf. client_master.h)
    // - sinon
    //       . envoyer au premier worker ordre minimum (cf. master_worker.h)
    //       . recevoir accusé de réception venant du worker concerné (cf. master_worker.h)
    //       . recevoir résultat (la valeur) venant du worker concerné
    //       . envoyer l'accusé de réception au client (cf. client_master.h)
    //       . envoyer le résultat au client
    //END TODO
}


/************************************************************************
 * quel est la maximum de l'ensemble
 ************************************************************************/
void orderMaximum(Data *data)
{
    TRACE0("[master] ordre maximum\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");
    if(compteur==0)
    {
    

    int reponse = CM_ANSWER_MAXIMUM_EMPTY  ;
	int write_res = write(data->openRes , &reponse  , sizeof(int) );
	myassert(write_res != -1 , " ") ;
    	
    
    }	
    
    else
    
    {
    	int order = CM_ORDER_MAXIMUM ;
		int write_res = write(data->fds_To_Worker[1] , &order , sizeof(int)) ;
		myassert(write_res != -1 ," ") ;
		float rep ;
		int read_res = read(data->fdWorker_To_Master[0] , &rep , sizeof(float)) ;
			    myassert(read_res != -1 ," ") ;
		printf("je print le reponse de max a master %f \n", rep) ;
	     write_res = write(data->openRes , &rep  , sizeof(float) );
		myassert(write_res != -1 , " ") ;

    	
    }
    //TODO
    // cf. explications pour le minimum
    //END TODO
}


/************************************************************************
 * test d'existence
 ************************************************************************/
void orderExist(Data *data)
{
    TRACE0("[master] ordre existence\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - recevoir l'élément à tester en provenance du client
    // - si ensemble vide (pas de premier worker)
    //       . envoyer l'accusé de réception dédié au client (cf. client_master.h)
    // - sinon
    //       . envoyer au premier worker ordre existence (cf. master_worker.h)
    //       . envoyer au premier worker l'élément à tester
    //       . recevoir accusé de réception venant du worker concerné (cf. master_worker.h)
    //       . si élément non présent
    //             . envoyer l'accusé de réception dédié au client (cf. client_master.h)
    //       . sinon
    //             . recevoir résultat (une quantité) venant du worker concerné
    //             . envoyer l'accusé de réception au client (cf. client_master.h)
    //             . envoyer le résultat au client
    //END TODO
}

/************************************************************************
 * somme
 ************************************************************************/
void orderSum(Data *data)
{
    TRACE0("[master] ordre somme\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    if(compteur==0)
    {
    	float reponse = 0. ;
		int write_res = write(data->openRes , &reponse  , sizeof(float) );
		myassert(write_res != -1 , " ") ;
    
    }
    else
    {
    	int order = CM_ORDER_SUM;
		int write_res = write(data->fds_To_Worker[1] , &order , sizeof(int)) ;
		myassert(write_res != -1 ," ") ;
		float rep ;
		printf("d'ici sa vient je pense avant le read \n") ;
		int read_res = read(data->fds_To_Master[0] , &rep , sizeof(float)) ;
			    myassert(read_res != -1 ," ") ;
		printf("je print le reponse de sum a master %f \n", rep) ;
	     write_res = write(data->openRes , &rep  , sizeof(float) );
		myassert(write_res != -1 , " ") ;
    
    }
    

  
    // - traiter le cas ensemble vide (pas de premier worker) : la somme est alors 0
    // - envoyer au premier worker ordre sum (cf. master_worker.h)
    // - recevoir accusé de réception venant du premier worker (cf. master_worker.h)
    // - recevoir résultat (la somme) venant du premier worker
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    // - envoyer le résultat au client
    //END TODO
}

/************************************************************************
 * insertion d'un élément
 ************************************************************************/

//TODO voir si une fonction annexe commune à orderInsert et orderInsertMany est justifiée

void orderInsert(Data *data)
{
    TRACE0("[master] ordre insertion\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");
    printf("j'ai recu l'ordre %d \n" , data->order ) ;

	int reponse = CM_ANSWER_INSERT_OK ;
	int write_res = write(data->openRes , &reponse  , sizeof(int) );
		myassert(write_res != -1 , " ") ;
	    compteur++ ;


  	if(compteur==1)
  	{

		
	    char stringfds1[50];
	    char stringfds2[50] ;
		char stringElement[50] ;
		char fdWorker[50] ;
		sprintf(stringfds1, "%d", data->fds_To_Worker[0]);
   		sprintf(stringfds2, "%d", data->fds_To_Master[1]);
    	sprintf(stringElement, "%f", data->elt);
    	sprintf(fdWorker, "%d", data->fdWorker_To_Master[1]);
		int res = fork() ;


			

				if(res ==0) 
			
				{
				
				
				

    			    printf("%s + %s + %s + %s on est a master proc fils string ......... \n" , stringElement , stringfds1 , stringfds2 , fdWorker) ;
					char * argv[] = {"./worker" , stringElement , stringfds1 , stringfds2 , fdWorker ,  NULL } ;
					char *path = "./worker" ;

					execv(path , argv) ;

				
				}
			
			//int res_creating ;
			//int res_work = read(data->fdWorker_To_Master[0] , &res_creating , sizeof(int)) ;
			//myassert(res_work != -1 , "") ;
			//if (res_creating==1000)
			//	{
			//	printf("premier worker est cree avec success \n") ;

			//	}
				
		//	else
		//	{
				//	printf("le premier worker n'est pas creer\n") ;

			//}

}


else
{

		int order = CM_ORDER_INSERT ;
		float elt = data->elt ;
		int write_res = write(data->fds_To_Worker[1] , &order , sizeof(int)) ;
		myassert(write_res != -1 ," ") ;
			write_res =write(data->fds_To_Worker[1] , &elt , sizeof(float)) ;
		myassert(write_res != -1 ," ") ;
			


}




 
    
	
    //TODO
    // - recevoir l'élément à insérer en provenance du client
    // - si ensemble vide (pas de premier worker)
    //       . créer le premier worker avec l'élément reçu du client
    // - sinon
    //       . envoyer au premier worker ordre insertion (cf. master_worker.h)
    //       . envoyer au premier worker l'élément à insérer
    // - recevoir accusé de réception venant du worker concerné (cf. master_worker.h)
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    //END TODO
}


/************************************************************************
 * insertion d'un tableau d'éléments
 ************************************************************************/
void orderInsertMany(Data *data)
{
    TRACE0("[master] ordre insertion tableau\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - recevoir le tableau d'éléments à insérer en provenance du client
    // - pour chaque élément du tableau
    //       . l'insérer selon l'algo vu dans orderInsert (penser à factoriser le code)
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    //END TODO
}


/************************************************************************
 * affichage ordonné
 ************************************************************************/
void orderPrint(Data *data)
{
    TRACE0("[master] ordre affichage\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");
    int write_res ;


    //TODO
    // - traiter le cas ensemble vide (pas de premier worker)
    if(compteur==0)
    {
    	int rep = CM_ANSWER_PRINT_OK ;
    	write_res = write(data->openRes , &rep , sizeof(int)) ;
    	myassert(write_res !=0 , " " ) ;
    	
    }
    else
    {
    	int order = MW_ORDER_PRINT;
        write_res = write(data->fds_To_Worker[1] , &order , sizeof(int)) ;
			myassert(write_res != -1 ," ") ;
		int accuse;
		int read_res ;
		read_res = read(data->openRes , &accuse , sizeof(int)) ;
    	myassert(read_res !=0 , " " ) ;
    	printf("accuse de worker %d \n" , accuse) ;
    	int rep = CM_ANSWER_PRINT_OK ;
    	write_res = write(data->openRes , &rep , sizeof(int)) ;
    	myassert(write_res !=0 , " " ) ;
    
    }
    
    // - envoyer au premier worker ordre print (cf. master_worker.h)
    // - recevoir accusé de réception venant du premier worker (cf. master_worker.h)
    //   note : ce sont les workers qui font les affichages
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    //END TODO
}


/************************************************************************
 * boucle principale de communication avec le client
 ************************************************************************/
void loop(Data *data)
{
    bool end = false;



    while (! end)
    {
        //TODO ouverture des tubes avec le client (cf. explications dans client.c)
        printf("on est dans la boucle'n") ;
        int open_CTM = open(FD_CTOM , O_RDONLY) ;
                myassert(open_CTM != -1 , " ") ;
        int open_MTC = open(FD_MTOC , O_WRONLY) ;
                myassert(open_MTC != -1 , " ") ;
     

         //TODO pour que ça ne boucle pas, mais recevoir l'ordre du client
        int order_recieve = read(open_CTM , &(data->order) , sizeof(int)) ;
        myassert(order_recieve != -1 , " " ) ;
        data->openRes = open_MTC ;
        switch(data->order)
        {
          case CM_ORDER_STOP:
            orderStop(data);
            end = true;
            break;
          case CM_ORDER_HOW_MANY:
            orderHowMany(data);
            break;
          case CM_ORDER_MINIMUM:
            orderMinimum(data);
            break;
          case CM_ORDER_MAXIMUM:
            orderMaximum(data);
            break;
          case CM_ORDER_EXIST:
            orderExist(data);
            break;
          case CM_ORDER_SUM:
            orderSum(data);
            break;
          case CM_ORDER_INSERT:
          	 order_recieve = read(open_CTM , &(data->elt) , sizeof(float)) ;
    		 myassert(order_recieve != -1 , " " ) ;
            orderInsert(data);
            break;
          case CM_ORDER_INSERT_MANY:
            orderInsertMany(data);
            break;
          case CM_ORDER_PRINT:
            orderPrint(data);
            break;
          default:
            myassert(false, "ordre inconnu");
            exit(EXIT_FAILURE);
            break;
        }

        //TODO fermer les tubes nommés
        //     il est important d'ouvrir et fermer les tubes nommés à chaque itération
        //     voyez-vous pourquoi ?
       int close_res =  close(open_MTC) ;
        myassert(close_res != -1 ," ") ;
        close_res =close(open_CTM) ;
                myassert(close_res != -1 ," ") ;
        //TODO attendre ordre du client avant de continuer (sémaphore pour une précédence)

        //TRACE0("[master] fin ordre\n");
    }
}


/************************************************************************
 * Fonction principale
 ************************************************************************/

//TODO N'hésitez pas à faire des fonctions annexes ; si les fonctions main
//TODO et loop pouvaient être "courtes", ce serait bien

int main(int argc, char * argv[])
{
    if (argc != 1)
        usage(argv[0], NULL);

    TRACE0("[master] début\n");

    Data data;

    //TODO
    // - création des sémaphores
    // - création des tubes nommés
    int Master_To_Client = mkfifo(FD_MTOC , 0644) ;
    assert(Master_To_Client !=-1) ;
    int Client_To_Master = mkfifo(FD_CTOM , 0644) ;
    assert(Client_To_Master !=-1) ;
    
    
 

    //END TODO
int fdWorker_To_Master[2];

int fds_To_Master[2] ;
int fds_To_Worker[2] ;  
pipe(fds_To_Master);
pipe(fdWorker_To_Master) ;
pipe(fds_To_Worker) ;



data.fds_To_Master[0] = fds_To_Master[0];
data.fds_To_Master[1] = fds_To_Master[1];
data.fds_To_Worker[0] = fds_To_Worker[0];
data.fds_To_Worker[1] = fds_To_Worker[1];

data.fdWorker_To_Master[0] = fdWorker_To_Master[0];
data.fdWorker_To_Master[1] = fdWorker_To_Master[1];
    loop(&data);


   unlink(FD_MTOC);
   unlink(FD_CTOM);

    //TODO destruction des tubes nommés, des sémaphores, ...

    TRACE0("[master] terminaison\n");
    return EXIT_SUCCESS;
}
