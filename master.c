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



#include <sys/ipc.h>
#include <sys/sem.h>
#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/************************************************************************
 * Données persistantes d'un master
 ************************************************************************/

static int compteur = 0 ; //C'est un compteur si = 0 cela veut dire pas de worker , sinon premier worker exist .

typedef struct
{

    int openRes ; // c'est le fd qui permet l'envoie au client
    int fdFromClient ; //c'est un fd qu'on utilise dans insertmany pour recevoir le tableau d'elemenet de client
    int fds_To_Master[2] ; // c'est le tube qui permet au premier worker d'envoyer au master
    int fds_To_Worker[2] ; //le tube qui permet le master d'envoyer des commandes au premier worker 
    int fdWorker_To_Master[2]; // le tube qui permet au workers d'envoyer directement le res au master comme dans min et max .
    int precedence ; // l'identifiant de la semaphore qui bloque le master tant qu'il a pas recu d'order .

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

        int open_CTM = open(FD_CTOM, O_RDONLY) ;
        myassert(open_CTM != -1, " ") ;
        int open_MTC = open(FD_MTOC, O_WRONLY) ;
        myassert(open_MTC != -1, " ") ;
        data->openRes = open_MTC ;
        data->fdFromClient = open_CTM ;

}


/************************************************************************
 * fin du master
 ************************************************************************/
void orderStop(Data *data)
{
    TRACE0("[master] ordre stop\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");
    int write_res ;

    if(compteur!=0)
    {
        int order = CM_ORDER_STOP;
        write_res = write(data->fds_To_Worker[1], &order, sizeof(int)) ;
        myassert(write_res != -1," ") ;
        wait(NULL) ;

    }
    
        int reponse = CM_ANSWER_STOP_OK ;
        write_res = write(data->openRes, &reponse, sizeof(int) );
        myassert(write_res != -1, " ") ;


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
    int nb_diff_elts  = 0; 
    int nb_all_elements = 0 ;
    int fd = data->openRes ;
    
    int accuse = CM_ANSWER_HOW_MANY_OK ;
    write_res = write(fd, &accuse, sizeof(int)) ;
    if(compteur==0) //dans le cas d'un ensemble vide la rep est (0, 0 ) 
    {
        write_res = write(fd, &nb_all_elements, sizeof(int)) ;
        myassert(write_res!=0, " ") ;
        write_res = write(fd, &nb_diff_elts, sizeof(int)) ;
        myassert(write_res!=0, " ") ;       

    }
    else
    {
        int order = CM_ORDER_HOW_MANY;
        write_res = write(data->fds_To_Worker[1], &order, sizeof(int)) ;
        myassert(write_res != -1," ") ;
        int read_res = read(data->fds_To_Master[0], &accuse, sizeof(int)) ;
        myassert(read_res != -1," ") ;
        read_res = read(data->fds_To_Master[0], &nb_diff_elts, sizeof(int)) ;
        myassert(read_res != -1," ") ;
        read_res = read(data->fds_To_Master[0], &nb_all_elements, sizeof(int)) ;
        myassert(read_res != -1," ") ;
        write_res = write(fd, &nb_all_elements, sizeof(int)) ;
        myassert(write_res!=0, " ") ;
        write_res = write(fd, &nb_diff_elts, sizeof(int)) ;
        myassert(write_res!=0, " ") ;
        printf("voici le resultat de how many a master (%d , %d) \n " , nb_diff_elts , nb_all_elements) ;


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
    int write_res ;
    int fd = data->openRes ;
    int accuse ;
    if(compteur==0)
    {

        accuse = CM_ANSWER_MINIMUM_EMPTY  ;
        write_res = write(fd, &accuse, sizeof(int) );
        myassert(write_res != -1, " ") ;


    }

    else

    {


        int order = data->order;
        write_res = write(data->fds_To_Worker[1], &order, sizeof(int)) ;
        myassert(write_res != -1," ") ;
        int read_res = read(data->fdWorker_To_Master[0], &accuse, sizeof(int)) ;
        myassert(read_res != -1," ") ;
        float rep ;
        read_res = read(data->fdWorker_To_Master[0], &rep, sizeof(float)) ;
        myassert(read_res != -1," ") ;
        accuse =CM_ANSWER_MINIMUM_OK ;
        write_res = write(fd, &accuse, sizeof(int) );
        myassert(write_res != -1, " ") ;
        write_res = write(fd, &rep, sizeof(float) );
        myassert(write_res != -1, " ") ;


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
    int accuse;
    int write_res ;
    int fd = data->openRes ;

    TRACE0("[master] ordre maximum\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");
    if(compteur==0)
    {


        accuse = CM_ANSWER_MAXIMUM_EMPTY  ;
        write_res = write(fd, &accuse, sizeof(int) );
        myassert(write_res != -1, " ") ;


    }

    else

    {
        accuse = CM_ANSWER_MAXIMUM_OK  ;
        write_res = write(fd, &accuse, sizeof(int) );
        myassert(write_res != -1, " ") ;
        int order = CM_ORDER_MAXIMUM ;
        write_res = write(data->fds_To_Worker[1], &order, sizeof(int)) ;
        myassert(write_res != -1," ") ;
        float rep ;
        int read_res = read(data->fdWorker_To_Master[0], &rep, sizeof(float)) ;
        myassert(read_res != -1," ") ;
        write_res = write(fd, &rep, sizeof(float) );
        myassert(write_res != -1, " ") ;


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
    int accuse ;
    int write_res ;
    int fd = data->openRes ;

    if (compteur==0)
    {
        accuse = CM_ANSWER_EXIST_NO ;
        write_res = write(fd, &accuse, sizeof(int) );
        myassert(write_res != -1, " ") ;
    }
    else
    {
        int order = MW_ORDER_EXIST ;
        write_res = write(data->fds_To_Worker[1], &order, sizeof(int)) ;
        myassert(write_res != -1," ") ;
        write_res = write(data->fds_To_Worker[1], &(data->elt), sizeof(float)) ;
        myassert(write_res != -1," ") ;
        int read_res = read(data->fdWorker_To_Master[0], &accuse, sizeof(int)) ;
        myassert(read_res != -1," ") ;


        if(accuse == MW_ANSWER_EXIST_YES)
        {
            accuse = CM_ANSWER_EXIST_YES ;
            write_res = write(fd, &accuse, sizeof(int)) ;
            myassert(write_res != -1, " ") ;
            int cardinality ;
            read_res = read(data->fdWorker_To_Master[0], &cardinality, sizeof(int)) ;
            myassert(read_res != -1, " ") ;
            write_res = write(fd, &cardinality, sizeof(int)) ;
            myassert(write_res != -1, " ") ;
        }
        else
        {
            accuse = CM_ANSWER_EXIST_NO ;
            write_res = write(fd, &accuse, sizeof(int) );
            myassert(write_res != -1, " ") ;
        }


    }

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
    int write_res ;
    int fd = data->openRes ;
    //TODO
    if(compteur==0)
    {
        float reponse = 0. ;
        write_res = write(fd, &reponse, sizeof(float) );
        myassert(write_res != -1, " ") ;

    }
    else
    {
        int order = CM_ORDER_SUM;
        int accuse ;
        write_res= write(data->fds_To_Worker[1], &order, sizeof(int)) ;
        myassert(write_res != -1, " ") ;
        int read_res = read(data->fds_To_Master[0], &accuse, sizeof(int)) ;
        myassert(write_res != -1," ") ;
        float rep ;
        read_res = read(data->fds_To_Master[0], &rep, sizeof(float)) ;
        myassert(read_res != -1," ") ;
        write_res = write(fd, &accuse, sizeof(int)) ;
        myassert(write_res != -1, " ") ;
        write_res = write(fd, &rep, sizeof(float) );
        myassert(write_res != -1, " ") ;

    }



    // - traiter le cas ensemble vide (pas de premier worker) : la somme est alors 0
    // - envoyer au premier worker ordre sum (cf. master_worker.h)
    // - recevoir accusé de réception venant du premier worker (cf. master_worker.h)
    // - recevoir résultat (la somme) venant du premier worker
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    // - envoyer le résultat au client
    //END TODO
}
int my_semget(int nbreTokens, char * kEY,int projectID)
{
    key_t key;
    int semId;
    int ret;

    key = ftok(kEY, projectID);
    assert(key != -1);
    semId = semget(key, 1, IPC_CREAT | IPC_EXCL | 0641);
    myassert(semId != -1, " ");

    ret = semctl(semId, 0, SETVAL, nbreTokens);
    myassert(ret != -1, " ");


    return semId;
}

//-----------------------------------------------------------------
static void my_destroy(int semId)
{
    int ret;

    ret = semctl(semId, -1, IPC_RMID);
    myassert(ret != -1, " ");
}


/************************************************************************
 * insertion d'un élément
 ************************************************************************/

//TODO voir si une fonction annexe commune à orderInsert et orderInsertMany est justifiée
void orderInsertLocal(Data * data)
{
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

            char * argv[] = {"./worker", stringElement, stringfds1, stringfds2, fdWorker,  NULL } ;
            char *path = "./worker" ;

            execv(path, argv) ;


        }
        
       
     }

     


    else
    {

        int order = CM_ORDER_INSERT ;
        float elt = data->elt ;
        int write_res = write(data->fds_To_Worker[1], &order, sizeof(int)) ;
        myassert(write_res != -1," ") ;
        write_res =write(data->fds_To_Worker[1], &elt, sizeof(float)) ;
        myassert(write_res != -1," ") ;



    }

}


void orderInsert(Data *data)
{
    TRACE0("[master] ordre insertion\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");
    printf("j'ai recu l'ordre %d \n", data->order ) ;
    int write_res ;
    orderInsertLocal(data) ;
    int fd = data->openRes ;
    int reponse = CM_ANSWER_INSERT_OK ;
    write_res = write(fd, &reponse, sizeof(int) );
    myassert(write_res != -1, " ") ;




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
    int taille ;
    int read_res = read(data->fdFromClient,  &taille, sizeof(int)) ;
    myassert(read_res != -1, " " ) ;
    printf("j'ai bien recu la taille %d \n ", taille) ;
    float val ;

    for (int i =0 ; i < taille ; i++)
    {
        read_res = read(data->fdFromClient,  &val, sizeof(int)) ;
        myassert(val != -1 , " ") ;
        printf("l'element recu %f \n ", val) ;
        data->elt = val ;
        orderInsertLocal(data) ;
    }
    int accuse = CM_ANSWER_INSERT_MANY_OK ;
    printf("l'accuse %d \n ", accuse) ;
    int write_res = write(data->openRes, &accuse, sizeof(int)) ;
    myassert(write_res != -1, " " ) ;

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


    if(compteur!=0)
    {
        int order = MW_ORDER_PRINT;
        write_res = write(data->fds_To_Worker[1], &order, sizeof(int)) ;
        myassert(write_res != -1," ") ;
        int accuse;
        int read_res ;
        read_res = read(data->fds_To_Master[0], &accuse, sizeof(int)) ;
        myassert(read_res !=0, " " ) ;
        printf("accuse de worker %d \n", accuse) ;

    }

    int rep = CM_ANSWER_PRINT_OK ;
    int fd = data->openRes ;
    write_res = write(fd, &rep, sizeof(int)) ;
    myassert(write_res !=0, " " ) ;

    // - envoyer au premier worker ordre print (cf. master_worker.h)
    // - recevoir accusé de réception venant du premier worker (cf. master_worker.h)
    //   note : ce sont les workers qui font les affichages
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    //END TODO
}

static void entrerSC(int semId)
{
    struct sembuf operation = {0, -1, 0};
    int ret = semop(semId, &operation, 1);
    assert(ret != -1);
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


        init(data) ;

        //TODO pour que ça ne boucle pas, mais recevoir l'ordre du client
        int order_recieve = read(data->fdFromClient, &(data->order), sizeof(int)) ;
        myassert(order_recieve != -1, " " ) ;
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
            order_recieve = read(data->fdFromClient, &(data->elt), sizeof(float)) ;
            myassert(order_recieve != -1, " " ) ;
            orderExist(data);
            break;
        case CM_ORDER_SUM:
            orderSum(data);
            break;
        case CM_ORDER_INSERT:
            order_recieve = read(data->fdFromClient, &(data->elt), sizeof(float)) ;
            myassert(order_recieve != -1, " " ) ;
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
        int close_res =  close(data->openRes) ;
        myassert(close_res != -1," ") ;
        close_res =close(data->fdFromClient) ;
        myassert(close_res != -1," ") ;

        //TODO attendre ordre du client avant de continuer (sémaphore pour une précédence)
        entrerSC(data->precedence) ;

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
    int mutexid = my_semget(1, SC_CLIENTS, SC_ID) ;
    int secondMutex = my_semget(0, MUTEX_PRECEDENCE, PRECEDENCE_ID) ;
    data.precedence = secondMutex ;
    // - création des tubes nommés
    int Master_To_Client = mkfifo(FD_MTOC, 0644) ;
    assert(Master_To_Client !=-1) ;
    int Client_To_Master = mkfifo(FD_CTOM, 0644) ;
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
    my_destroy(mutexid) ;
    my_destroy(data.precedence) ;


    //TODO destruction des tubes nommés, des sémaphores, ...

    TRACE0("[master] terminaison\n");
    return EXIT_SUCCESS;
}

