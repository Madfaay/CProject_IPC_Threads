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

#include "master_worker.h"


/************************************************************************
 * Données persistantes d'un worker
 ************************************************************************/
typedef struct
{
    // données internes (valeur de l'élément, cardinalité)
    // communication avec le père (2 tubes) et avec le master (1 tube en écriture)
    // communication avec le fils gauche s'il existe (2 tubes)
    // communication avec le fils droit s'il existe (2 tubes)
    //TODO
    int order;
    int cardinality;
    float elt ;
    int fdIn ;
    int fdOut ;
    int fdToMaster ;
    int fdsG[2] ;
    int fdsD[2] ;
    int fdFilsG_To_Parent[2] ;
    int fdFilsD_To_Parent[2] ;
    bool fg;
    bool fd ;
   

} Data;


/************************************************************************
 * Usage et analyse des arguments passés en ligne de commande
 ************************************************************************/
static void usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usage : %s <elt> <fdIn> <fdOut> <fdToMaster>\n", exeName);
    fprintf(stderr, "   <elt> : élément géré par le worker\n");
    fprintf(stderr, "   <fdIn> : canal d'entrée (en provenance du père)\n");
    fprintf(stderr, "   <fdOut> : canal de sortie (vers le père)\n");
    fprintf(stderr, "   <fdToMaster> : canal de sortie directement vers le master\n");
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}


static void parseArgs(int argc, char * argv[], Data *data)
{
    myassert(data != NULL, "il faut l'environnement d'exécution");

    if (argc != 5)
        usage(argv[0], "Nombre d'arguments incorrect");

    //TODO initialisation data
    

    //TODO (à enlever) comment récupérer les arguments de la ligne de commande

    float elt = strtof(argv[1], NULL);
    int fdIn = strtol(argv[2], NULL, 10);
    int fdOut = strtol(argv[3], NULL, 10);
    close(fdOut) ;
    int fdToMaster = strtol(argv[4], NULL, 10);
    printf("on est a worker on regarde les params recus %f , %d , %d , %d \n " , elt , fdIn , fdOut , fdToMaster) ;
   // printf("%g %d %d %d dans le compteur\n", elt, fdIn, fdOut, fdToMaster);
    int reponse = 1000 ;
    int write_res = write(fdToMaster , &reponse , sizeof(int)) ;
    printf("on est la 76 ou pas \n") ;
    myassert(write_res !=-1 , "") ;
    data->elt = elt ;
    data->fdIn = fdIn ;
    data->fdOut = fdOut ;
    data->fdToMaster = fdToMaster ;
    data->cardinality+=1 ;
   
    
    
 
    
    
    //END TODO
}


/************************************************************************
 * Stop 
 ************************************************************************/
void stopAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre stop\n", getpid(), getppid(), 3.14 /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - traiter les cas où les fils n'existent pas
    // - envoyer au worker gauche ordre de fin (cf. master_worker.h)
    // - envoyer au worker droit ordre de fin (cf. master_worker.h)
    // - attendre la fin des deux fils
    //END TODO
}


/************************************************************************
 * Combien d'éléments
 ************************************************************************/
static void howManyAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre how many\n", getpid(), getppid(), 3.14 /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - traiter les cas où les fils n'existent pas
    // - pour chaque fils
    //       . envoyer ordre howmany (cf. master_worker.h)
    //       . recevoir accusé de réception (cf. master_worker.h)
    //       . recevoir deux résultats (nb elts, nb elts distincts) venant du fils
    // - envoyer l'accusé de réception au père (cf. master_worker.h)
    // - envoyer les résultats (les cumuls des deux quantités + la valeur locale) au père
    //END TODO
}


/************************************************************************
 * Minimum
 ************************************************************************/
static void minimumAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre minimum\n", getpid(), getppid(), 3.14 /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - si le fils gauche n'existe pas (on est sur le minimum)
    //       . envoyer l'accusé de réception au master (cf. master_worker.h)
    //       . envoyer l'élément du worker courant au master
    // - sinon
    //       . envoyer au worker gauche ordre minimum (cf. master_worker.h)
    //       . note : c'est un des descendants qui enverra le résultat au master
    //END TODO
}


/************************************************************************
 * Maximum
 ************************************************************************/
static void maximumAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre maximum\n", getpid(), getppid(), 3.14 /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // cf. explications pour le minimum
    //END TODO
}


/************************************************************************
 * Existence
 ************************************************************************/
static void existAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre exist\n", getpid(), getppid(), 3.14 /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - recevoir l'élément à tester en provenance du père
    // - si élément courant == élément à tester
    //       . envoyer au master l'accusé de réception de réussite (cf. master_worker.h)
    //       . envoyer cardinalité de l'élément courant au master
    // - sinon si (elt à tester < elt courant) et (pas de fils gauche)
    //       . envoyer au master l'accusé de réception d'échec (cf. master_worker.h)
    // - sinon si (elt à tester > elt courant) et (pas de fils droit)
    //       . envoyer au master l'accusé de réception d'échec (cf. master_worker.h)
    // - sinon si (elt à tester < elt courant)
    //       . envoyer au worker gauche ordre exist (cf. master_worker.h)
    //       . envoyer au worker gauche élément à tester
    //       . note : c'est un des descendants qui enverra le résultat au master
    // - sinon (donc elt à tester > elt courant)
    //       . envoyer au worker droit ordre exist (cf. master_worker.h)
    //       . envoyer au worker droit élément à tester
    //       . note : c'est un des descendants qui enverra le résultat au master
    //END TODO
}


/************************************************************************
 * Somme
 ************************************************************************/
static void sumAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre sum\n", getpid(), getppid(), 3.14 /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - traiter les cas où les fils n'existent pas
    // - pour chaque fils
    //       . envoyer ordre sum (cf. master_worker.h)
    //       . recevoir accusé de réception (cf. master_worker.h)
    //       . recevoir résultat (somme du fils) venant du fils
    // - envoyer l'accusé de réception au père (cf. master_worker.h)
    // - envoyer le résultat (le cumul des deux quantités + la valeur locale) au père
    //END TODO
}


/************************************************************************
 * Insertion d'un nouvel élément
 ************************************************************************/
static void insertAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre insert\n", getpid(), getppid(), 3.14 /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - recevoir l'élément à insérer en provenance du père
 float elt ;
 int read_res = read(data->fdIn , &elt , sizeof(float)) ;
 myassert(read_res != -1 , " ") ;
 if(elt ==data->elt)
 {
 data->cardinality+=1 ;
 }
 
 else 
 {
 		char stringfds1[50];
	    char stringfds2[50] ;
		char stringElement[50] ;
		char fdWorker[50] ;
		if(elt >=data->elt )
	{
			
			if(data->fd == false)
			{
			
			
		int res = fork() ;

				if(res ==0) 
			
				{
	
					sprintf(stringfds1, "%d", data->fdsD[0]);
   					sprintf(stringfds2, "%d", data->fdsD[1]);
    				sprintf(stringElement, "%f", elt);
    				sprintf(fdWorker, "%d", data->fdFilsD_To_Parent[1]);
				
			        printf("%f + %d + %d + %d  on est a worker pour creer un fils.....\n" , elt , data->fdsD[0] , data->fdsD[1] , data->fdFilsD_To_Parent[1]) ;
    			    printf("%s + %s + %s + %s on est a worker pour creer un fils. string ......... \n" , stringElement , stringfds1 , stringfds2 , fdWorker) ;
					char * argv[] = {"./worker" , stringElement , stringfds1 , stringfds2 , fdWorker ,  NULL } ;
					char *path = "./worker" ;

					execv(path , argv) ;
	

					}
					
			

					}
							}
			else if(elt <= data->elt)
			{
			
						if(data->fg== false)
			{
			
			
		int res = fork() ;

				if(res ==0) 
			
				{
	
					sprintf(stringfds1, "%d", data->fdsG[0]);
   					sprintf(stringfds2, "%d", data->fdsG[1]);
    				sprintf(stringElement, "%f", elt);
    				sprintf(fdWorker, "%d", data->fdFilsG_To_Parent[1]);
				
			        printf("%f + %d + %d + %d  on est a worker pour creer un fils.....\n" , elt , data->fdsG[0] , data->fdsG[1] , data->fdFilsG_To_Parent[1]) ;
    			    printf("%s + %s + %s + %s on est a worker pour creer un fils. string ......... \n" , stringElement , stringfds1 , stringfds2 , fdWorker) ;
					char * argv[] = {"./worker" , stringElement , stringfds1 , stringfds2 , fdWorker ,  NULL } ;
					char *path = "./worker" ;

					execv(path , argv) ;
	

					}
					
			

					}
			
			}

}
		
    // - si élément courant == élément à tester
    //       . incrémenter la cardinalité courante
    //       . envoyer au master l'accusé de réception (cf. master_worker.h)
    // - sinon si (elt à tester < elt courant) et (pas de fils gauche)
    //       . créer un worker à gauche avec l'élément reçu du client
    //       . note : c'est ce worker qui enverra l'accusé de réception au master
    // - sinon si (elt à tester > elt courant) et (pas de fils droit)
    //       . créer un worker à droite avec l'élément reçu du client
    //       . note : c'est ce worker qui enverra l'accusé de réception au master
    // - sinon si (elt à insérer < elt courant)
    //       . envoyer au worker gauche ordre insert (cf. master_worker.h)
    //       . envoyer au worker gauche élément à insérer
    //       . note : c'est un des descendants qui enverra l'accusé de réception au master
    // - sinon (donc elt à insérer > elt courant)
    //       . envoyer au worker droit ordre insert (cf. master_worker.h)
    //       . envoyer au worker droit élément à insérer
    //       . note : c'est un des descendants qui enverra l'accusé de réception au master
    //END TODO
}


/************************************************************************
 * Affichage
 ************************************************************************/
static void printAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre print\n", getpid(), getppid(), 3.14 /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - si le fils gauche existe
    //       . envoyer ordre print (cf. master_worker.h)
    //       . recevoir accusé de réception (cf. master_worker.h)
    // - afficher l'élément courant avec sa cardinalité
    // - si le fils droit existe
    //       . envoyer ordre print (cf. master_worker.h)
    //       . recevoir accusé de réception (cf. master_worker.h)
    // - envoyer l'accusé de réception au père (cf. master_worker.h)
    //END TODO
}


/************************************************************************
 * Boucle principale de traitement
 ************************************************************************/
void loop(Data *data)
{
    bool end = false;

    while (! end)
    {

        int order ;
 //TODO pour que ça ne boucle pas, mais recevoir l'ordre du père
        int read_res = read(data->fdIn , &order , sizeof(int)) ;
        myassert(read_res != -1 , " ") ;
        	data->order = order ;
        	printf("je suis worker et voila l'ordre %d \n" , order ) ;
        	
        switch(order)
        {
          case MW_ORDER_STOP:
            stopAction(data);
            end = true;
            break;
          case MW_ORDER_HOW_MANY:
            howManyAction(data);
            break;
          case MW_ORDER_MINIMUM:
            minimumAction(data);
            break;
          case MW_ORDER_MAXIMUM:
            maximumAction(data);
            break;
          case MW_ORDER_EXIST:
            existAction(data);
            break;
          case MW_ORDER_SUM:
            sumAction(data);
            break;
          case MW_ORDER_INSERT:
            insertAction(data);
            break;
          case MW_ORDER_PRINT:
            printAction(data);
            break;
          default:
            myassert(false, "ordre inconnu");
            exit(EXIT_FAILURE);
            break;
        }

        TRACE3("    [worker (%d, %d) {%g}] : fin ordre\n", getpid(), getppid(), 3.14 /*TODO élément*/);
    }
}


/************************************************************************
 * Programme principal
 ************************************************************************/

int main(int argc, char * argv[])
{
    Data data;
    parseArgs(argc, argv, &data);
    TRACE3("    [worker (%d, %d) {%g}] : début worker\n", getpid(), getppid(), 3.14 /*TODO élément*/);

    //TODO envoyer au master l'accusé de réception d'insertion (cf. master_worker.h)
    //TODO note : en effet si je suis créé c'est qu'on vient d'insérer un élément : moi
    
int fdsG[2];
int fdsD[2];
int fdFilsG_To_Parent[2];
int fdFilsD_To_Parent[2];

pipe(fdsG);
pipe(fdsD);
pipe(fdFilsG_To_Parent);
pipe(fdFilsD_To_Parent);


data.fdsG[0] = fdsG[0];
data.fdsG[1] = fdsG[1];

data.fdsD[0] = fdsD[0];
data.fdsD[1] = fdsD[1];

data.fdFilsG_To_Parent[0] = fdFilsG_To_Parent[0];
data.fdFilsG_To_Parent[1] = fdFilsG_To_Parent[1];

data.fdFilsD_To_Parent[0] = fdFilsD_To_Parent[0];
data.fdFilsD_To_Parent[1] = fdFilsD_To_Parent[1];
    loop(&data);

    //TODO fermer les tubes

    TRACE3("    [worker (%d, %d) {%g}] : fin worker\n", getpid(), getppid(), 3.14 /*TODO élément*/);
    return EXIT_SUCCESS;
}
