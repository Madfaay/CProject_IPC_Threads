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
    int cardinality; // la cardinalite de l'element .
    float elt ;
    int fdIn ; //le fd du pere vers ce worker .
    int fdOut ; //le fd de ce worker vers son perer
    int fdToMaster ; // un fd direct vers le matser .
    int fdsG[2] ; // un tube anonyme avec son fils gauche .
    int fdsD[2] ; // un tube anonyme avec sont fils droit .
    int fdFilsG_To_Parent[2] ; // un tube anonyme du fils gauche (s'il existe) vers ce worker .
    int fdFilsD_To_Parent[2] ; // un tube anonyme du fils droit (s'il existe) vers ce worker .
    bool fg; // on conserve l'infomation d'existence d'un fils gauche ou pas par un bool.
    bool fd; // pareil pour la droit .
    float sumRes ; // utile pour l'ordre sum ou on a le resultat de worker + celui des ses enfnats .


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
    int fdToMaster = strtol(argv[4], NULL, 10);
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
    TRACE3("    [worker (%d, %d) {%g}] : ordre stop\n", getpid(), getppid(),  data->elt/*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");
    
    //TODO
    if(data->fd ==true)
    {

        int write_res = write(data->fdsD[1], &(data->order), sizeof(int)) ;
        myassert(write_res != -1, "" ) ;
        wait(NULL) ;

    }

    if(data->fg ==true)
    {   
       int write_res = write(data->fdsG[1], &(data->order), sizeof(int)) ;
        myassert(write_res != -1, "" ) ;
        wait(NULL) ;
    }





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
    TRACE3("    [worker (%d, %d) {%g}] : ordre how many\n", getpid(), getppid(), data->elt /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");
    int write_res ;
    int read_res ;
    int nb_elements = 0;
    int nb_all_elements = 0 ;
    int accuse ;
    if(data->fd == true)
    {
        int nb_elements_d ;
        int nb_all_elements_d ;
        write_res = write(data->fdsD[1], &(data->order), sizeof(int)) ;
        myassert(write_res != -1, "" ) ;
        read_res = read(data->fdFilsD_To_Parent[0], &accuse, sizeof(int)) ;
        myassert(read_res != -1, "" ) ;
        read_res = read(data->fdFilsD_To_Parent[0], &nb_elements_d, sizeof(int)) ;
        myassert(read_res != -1, "" ) ;
        read_res = read(data->fdFilsD_To_Parent[0], &nb_all_elements_d, sizeof(int)) ;
        myassert(read_res != -1, "" ) ;
        nb_elements+= nb_elements_d ;
        nb_all_elements+= nb_all_elements_d ;

    }


    if(data->fg == true)
    {
        int nb_elements_g ;
        int nb_all_elements_g ;
        write_res = write(data->fdsG[1], &(data->order), sizeof(int)) ;
        myassert(write_res != -1, "" ) ;
        
        read_res = read(data->fdFilsG_To_Parent[0], &accuse, sizeof(int)) ;
        myassert(read_res != -1, "" ) ;
        read_res = read(data->fdFilsG_To_Parent[0], &nb_elements_g, sizeof(int)) ;
        myassert(read_res != -1, "" ) ;
        read_res = read(data->fdFilsG_To_Parent[0], &nb_all_elements_g, sizeof(int)) ;
        myassert(read_res != -1, "" ) ;
        nb_elements+= nb_elements_g ;
        nb_all_elements+= nb_all_elements_g ;
    }

    nb_elements ++ ;
    nb_all_elements+= data->cardinality ;
    accuse =MW_ANSWER_HOW_MANY ;
    write_res = write(data->fdOut, &accuse, sizeof(int)) ;
    myassert(write_res != -1, "" ) ;
    write_res = write(data->fdOut, &nb_elements, sizeof(int)) ;
    myassert(write_res != -1, "" ) ;
    write_res = write(data->fdOut, &nb_all_elements, sizeof(int)) ;
    myassert(write_res != -1, "" ) ;



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
    TRACE3("    [worker (%d, %d) {%g}] : ordre minimum\n", getpid(), getppid(), data->elt /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");
    int write_res ;
    if(data->fg==false)
    {
        int accuse = MW_ANSWER_MINIMUM ;
        write_res = write(data->fdToMaster, &accuse, sizeof(float)) ;
        myassert(write_res != -1, "" ) ;
        float res = data->elt ;
        write_res = write(data->fdToMaster, &res, sizeof(float)) ;
        myassert(write_res != -1, "" ) ;

    }
    else
    {
        write_res = write(data->fdsG[1], &(data->order), sizeof(int)) ;
        myassert(write_res != -1, "" ) ;
    }

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
    TRACE3("    [worker (%d, %d) {%g}] : ordre maximum\n", getpid(), getppid(), data->elt /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");
    int write_res ;
    if(data->fd==false)
    {

        float res = data->elt ;
        write_res = write(data->fdToMaster, &res, sizeof(float)) ;
        myassert(write_res != -1, "" ) ;

    }
    else
    {
        write_res = write(data->fdsD[1], &(data->order), sizeof(int)) ;
        myassert(write_res != -1, "" ) ;
    }
    //TODO
    // cf. explications pour le minimum
    //END TODO
}


/************************************************************************
 * Existence
 ************************************************************************/
static void existAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre exist\n", getpid(), getppid(), data->elt/*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    float elt ;

    int read_res = read(data->fdIn, &elt, sizeof(float)) ;
    myassert(read_res != -1, " ") ;
    if(elt == data->elt)
    {
        int accuse = MW_ANSWER_EXIST_YES ;
        int write_res = write(data->fdToMaster, &accuse, sizeof(int));
        myassert(write_res != -1, " ") ;
        int card = data->cardinality ;
        write_res =write(data->fdToMaster, &card, sizeof(int));
        myassert(write_res != -1, " ") ;
    }

    else
    {
        int order = data->order ;
        int write_res ;
        if(elt < data->elt)
        {
            if(data->fg==true)
            {
                write_res = write(data->fdsG[1], &(order), sizeof(int)) ;
                myassert(write_res != -1, "" ) ;
                write_res = write(data->fdsG[1], &(elt), sizeof(float)) ;
                myassert(write_res != -1, "" ) ;
            }
            else
            {
                int accuse = MW_ANSWER_EXIST_NO;
                write_res = write(data->fdToMaster, &accuse, sizeof(int));
            }

        }
        if(elt > data->elt)
        {
            if(data->fd==true)
            {
                write_res = write(data->fdsD[1], &(order), sizeof(int)) ;
                myassert(write_res != -1, "" ) ;
                write_res = write(data->fdsD[1], &(elt), sizeof(float)) ;
                myassert(write_res != -1, "" ) ;
            }
            else
            {
                int accuse = MW_ANSWER_EXIST_NO;
                write_res = write(data->fdToMaster, &accuse, sizeof(int));
            }


        }
    }








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
    TRACE3("    [worker (%d, %d) {%g}] : ordre sum\n", getpid(), getppid(),data->elt /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");
    int write_res ;
    int read_res ;
    int accuse ;
    if(data->fd == true)
    {

        write_res = write(data->fdsD[1], &(data->order), sizeof(int)) ;
        myassert(write_res != -1, "" ) ;
        float resd ;
        read_res = read(data->fdFilsD_To_Parent[0], &accuse, sizeof(int)) ;
        myassert(read_res != -1, "" ) ;
        read_res = read(data->fdFilsD_To_Parent[0], &(resd), sizeof(float)) ;
        myassert(read_res != -1, "" ) ;
        data->sumRes+=resd;

    }

    if(data->fg==true)
    {
        write_res = write(data->fdsG[1], &(data->order), sizeof(int)) ;
        myassert(write_res != -1, "" ) ;
        float resg ;
        read_res = read(data->fdFilsG_To_Parent[0], &accuse, sizeof(int)) ;
        myassert(read_res != -1, "" ) ;
        read_res = read(data->fdFilsG_To_Parent[0], &(resg), sizeof(float)) ;
        myassert(read_res != -1, "" ) ;
        data->sumRes+=resg;

    }

    
    data->sumRes+=(data->elt)*data->cardinality;
    accuse = MW_ANSWER_SUM   ;
    write_res = write(data->fdOut, &accuse, sizeof(int)) ;
    myassert(write_res != -1, "" ) ;
    write_res = write(data->fdOut, &(data->sumRes), sizeof(float)) ;
    myassert(write_res != -1, "" ) ;




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
    TRACE3("    [worker (%d, %d) {%g}] : ordre insert\n", getpid(), getppid(), data->elt /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - recevoir l'élément à insérer en provenance du père
    float elt ;
    int read_res = read(data->fdIn, &elt, sizeof(float)) ;
    myassert(read_res != -1, " ") ;
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

                data->fd = true ;
                int res = fork() ;

                if(res ==0)

                {

                    sprintf(stringfds1, "%d", data->fdsD[0]);
                    sprintf(stringfds2, "%d", data->fdFilsD_To_Parent[1]);
                    sprintf(stringElement, "%f", elt);
                    sprintf(fdWorker, "%d", data->fdToMaster);


                    char * argv[] = {"./worker", stringElement, stringfds1, stringfds2, fdWorker,  NULL } ;
                    char *path = "./worker" ;
                    //TRACE3("    [worker (%d, %d) {%g}] : ordre insert\n", getpid(), getppid(), data->elt) ;
                    execv(path, argv) ;


                }
                
                
         }   


            else

            {
                
                int order = data->order ;
                write(data->fdsD[1], &order, sizeof(int)) ;
                write(data->fdsD[1], &elt, sizeof(int)) ;


            }

}
        
        else if(elt <= data->elt)
        {

            if(data->fg== false)
            {

                data->fg = true ;
                int res = fork() ;

                if(res ==0)

                {

                    sprintf(stringfds1, "%d", data->fdsG[0]);
                    sprintf(stringfds2, "%d", data->fdFilsG_To_Parent[1]);
                    sprintf(stringElement, "%f", elt);
                    sprintf(fdWorker, "%d", data->fdToMaster);


                    
                    char * argv[] = {"./worker", stringElement, stringfds1, stringfds2, fdWorker,  NULL } ;
                    char *path = "./worker" ;


                    execv(path, argv) ;


                }
                




            }

            else
            {





                int order = data->order ;
                write(data->fdsG[1], &order, sizeof(int)) ;
                write(data->fdsG[1], &elt, sizeof(int)) ;

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
    TRACE3("    [worker (%d, %d) {%g}] : ordre print\n", getpid(), getppid(), data->elt/*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");
    int accuse ;
    int write_res;
    int read_res;

    if(data->fg == true)
    {

        write_res = write(data->fdsG[1], &(data->order), sizeof(int)) ;
        myassert(write_res != -1, "" ) ;

        read_res = read(data->fdFilsG_To_Parent[0], &accuse, sizeof(int)) ;
        myassert(read_res != -1, "" ) ;

    }

    printf("j'ai l'element %f , de cardinalite %d\n ", data->elt, data->cardinality) ;


    if(data->fd == true)
    {

        write_res = write(data->fdsD[1], &(data->order), sizeof(int)) ;
        myassert(write_res != -1, "" ) ;
        read_res = read(data->fdFilsD_To_Parent[0], &accuse, sizeof(int)) ;
        myassert(read_res != -1, "" ) ;

    }

    accuse =MW_ANSWER_PRINT ;
    write_res = write(data->fdOut, &accuse, sizeof(int)) ;
    myassert(write_res != -1, "" ) ;




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
        int read_res = read(data->fdIn, &order, sizeof(int)) ;
        myassert(read_res != -1, " ") ;
        data->order = order ;

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

        TRACE3("    [worker (%d, %d) {%g}] : fin ordre\n", getpid(), getppid(), data->elt/*TODO élément*/);
    }
}


/************************************************************************
 * Programme principal
 ************************************************************************/

int main(int argc, char * argv[])
{
    Data data;

    //TRACE3("    [worker (%d, %d) {%g}] : début worker\n", getpid(), getppid(), 3.14 /*TODO élément*/);

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
    data.fd = false ;
    data.fg = false ;
    parseArgs(argc, argv, &data);
    loop(&data);

    //TODO fermer les tubes



    TRACE3("    [worker (actuelpid : %d, perepid :%d) {%f}] : fin worker\n", getpid(), getppid(), data.elt /*TODO élément*/);
    return EXIT_SUCCESS;
}

                

