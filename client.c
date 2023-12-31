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
#include "string.h"
#include <pthread.h>

#include <sys/ipc.h>
#include <sys/sem.h>

#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/************************************************************************
 * chaines possibles pour le premier paramètre de la ligne de commande
 ************************************************************************/
#define TK_STOP        "stop"             // arrêter le master
#define TK_HOW_MANY    "howmany"          // combien d'éléments dans l'ensemble
#define TK_MINIMUM     "min"              // valeur minimale de l'ensemble
#define TK_MAXIMUM     "max"              // valeur maximale de l'ensemble
#define TK_EXIST       "exist"            // test d'existence d'un élément, et nombre d'exemplaires
#define TK_SUM         "sum"              // somme de tous les éléments
#define TK_INSERT      "insert"           // insertion d'un élément
#define TK_INSERT_MANY "insertmany"       // insertions de plusieurs éléments aléatoires
#define TK_PRINT       "print"            // debug : demande aux master/workers d'afficher les éléments
#define TK_LOCAL       "local"            // lancer un calcul local (sans master) en multi-thread


/************************************************************************
 * structure stockant les paramètres du client
 * - les infos pour communiquer avec le master
 * - les infos pour effectuer le travail (cf. ligne de commande)
 *   (note : une union permettrait d'optimiser la place mémoire)
 ************************************************************************/
typedef struct
{
    // communication avec le master
    //TODO
    int openRes ; // On conserve le fd utile pour la communication .
    int accuse ; // pour recevoir l'accuse

    // infos pour le travail à faire (récupérées sur la ligne de commande)
    int order;     // ordre de l'utilisateur (cf. CM_ORDER_* dans client_master.h)
    float elt;     // pour CM_ORDER_EXIST, CM_ORDER_INSERT, CM_ORDER_LOCAL
    int nb;        // pour CM_ORDER_INSERT_MANY, CM_ORDER_LOCAL
    float min;     // pour CM_ORDER_INSERT_MANY, CM_ORDER_LOCAL
    float max;     // pour CM_ORDER_INSERT_MANY, CM_ORDER_LOCAL
    int nbThreads; // pour CM_ORDER_LOCAL


} Data ;



/************************************************************************
 * Usage
 ************************************************************************/
static void usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usages : %s <ordre> [[[<param1>] [<param2>] ...]]\n", exeName);
    fprintf(stderr, "   $ %s " TK_STOP "\n", exeName);
    fprintf(stderr, "          arrêt master\n");
    fprintf(stderr, "   $ %s " TK_HOW_MANY "\n", exeName);
    fprintf(stderr, "          combien d'éléments dans l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_MINIMUM "\n", exeName);
    fprintf(stderr, "          plus petite valeur de l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_MAXIMUM "\n", exeName);
    fprintf(stderr, "          plus grande valeur de l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_EXIST " <elt>\n", exeName);
    fprintf(stderr, "          l'élement <elt> est-il présent dans l'ensemble ?\n");
    fprintf(stderr, "   $ %s " TK_SUM "\n", exeName);
    fprintf(stderr, "           somme des éléments de l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_INSERT " <elt>\n", exeName);
    fprintf(stderr, "          ajout de l'élement <elt> dans l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_INSERT_MANY " <nb> <min> <max>\n", exeName);
    fprintf(stderr, "          ajout de <nb> élements (dans [<min>,<max>[) aléatoires dans l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_PRINT "\n", exeName);
    fprintf(stderr, "          affichage trié (dans la console du master)\n");
    fprintf(stderr, "   $ %s " TK_LOCAL " <nbThreads> <elt> <nb> <min> <max>\n", exeName);
    fprintf(stderr, "          combien d'exemplaires de <elt> dans <nb> éléments (dans [<min>,<max>[)\n"
            "          aléatoires avec <nbThreads> threads\n");

    if (message != NULL)
        fprintf(stderr, "message :\n    %s\n", message);

    exit(EXIT_FAILURE);
}


/************************************************************************
 * Analyse des arguments passés en ligne de commande
 ************************************************************************/
static void parseArgs(int argc, char * argv[], Data *data)
{
    data->order = CM_ORDER_NONE;

    if (argc == 1)
        usage(argv[0], "Il faut préciser une commande");

    // première vérification : la commande est-elle correcte ?
    if (strcmp(argv[1], TK_STOP) == 0)
        data->order = CM_ORDER_STOP;
    else if (strcmp(argv[1], TK_HOW_MANY) == 0)
        data->order = CM_ORDER_HOW_MANY;
    else if (strcmp(argv[1], TK_MINIMUM) == 0)
        data->order = CM_ORDER_MINIMUM;
    else if (strcmp(argv[1], TK_MAXIMUM) == 0)
        data->order = CM_ORDER_MAXIMUM;
    else if (strcmp(argv[1], TK_EXIST) == 0)
        data->order = CM_ORDER_EXIST;
    else if (strcmp(argv[1], TK_SUM) == 0)
        data->order = CM_ORDER_SUM;
    else if (strcmp(argv[1], TK_INSERT) == 0)
        data->order = CM_ORDER_INSERT;
    else if (strcmp(argv[1], TK_INSERT_MANY) == 0)
        data->order = CM_ORDER_INSERT_MANY;
    else if (strcmp(argv[1], TK_PRINT) == 0)
        data->order = CM_ORDER_PRINT;
    else if (strcmp(argv[1], TK_LOCAL) == 0)
        data->order = CM_ORDER_LOCAL;
    else
        usage(argv[0], "commande inconnue");

    // deuxième vérification : nombre de paramètres correct ?
    if ((data->order == CM_ORDER_STOP) && (argc != 2))
        usage(argv[0], TK_STOP " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_HOW_MANY) && (argc != 2))
        usage(argv[0], TK_HOW_MANY " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_MINIMUM) && (argc != 2))
        usage(argv[0], TK_MINIMUM " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_MAXIMUM) && (argc != 2))
        usage(argv[0], TK_MAXIMUM " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_EXIST) && (argc != 3))
        usage(argv[0], TK_EXIST " : il faut un et un seul argument après la commande");
    if ((data->order == CM_ORDER_SUM) && (argc != 2))
        usage(argv[0], TK_SUM " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_INSERT) && (argc != 3))
        usage(argv[0], TK_INSERT " : il faut un et un seul argument après la commande");
    if ((data->order == CM_ORDER_INSERT_MANY) && (argc != 5))
        usage(argv[0], TK_INSERT_MANY " : il faut 3 arguments après la commande");
    if ((data->order == CM_ORDER_PRINT) && (argc != 2))
        usage(argv[0], TK_PRINT " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_LOCAL) && (argc != 7))
        usage(argv[0], TK_LOCAL " : il faut 5 arguments après la commande");

    // extraction des arguments
    if (data->order == CM_ORDER_EXIST)
    {
        data->elt = strtof(argv[2], NULL);
    }
    else if (data->order == CM_ORDER_INSERT)
    {
        data->elt = strtof(argv[2], NULL);
    }
    else if (data->order == CM_ORDER_INSERT_MANY)
    {
        data->nb = strtol(argv[2], NULL, 10);
        data->min = strtof(argv[3], NULL);
        data->max = strtof(argv[4], NULL);
        if (data->nb < 1)
            usage(argv[0], TK_INSERT_MANY " : nb doit être strictement positif");
        if (data->max < data->min)
            usage(argv[0], TK_INSERT_MANY " : max ne doit pas être inférieur à min");
    }
    else if (data->order == CM_ORDER_LOCAL)
    {
        data->nbThreads = strtol(argv[2], NULL, 10);
        data->elt = strtof(argv[3], NULL);
        data->nb = strtol(argv[4], NULL, 10);
        data->min = strtof(argv[5], NULL);
        data->max = strtof(argv[6], NULL);
        if (data->nbThreads < 1)
            usage(argv[0], TK_LOCAL " : nbThreads doit être strictement positif");
        if (data->nb < 1)
            usage(argv[0], TK_LOCAL " : nb doit être strictement positif");
        if (data->max <= data->min)
            usage(argv[0], TK_LOCAL " : max ne doit être strictement supérieur à min");
    }
}

static void entrerSC(int semId)
{
    struct sembuf operation = {0, -1, 0};
    int ret = semop(semId, &operation, 1);
    myassert(ret != -1, " ");
}

//-----------------------------------------------------------------
// On sort de la  SC
static void sortirSC(int semId)
{
    // TODO
    struct sembuf operation = {0, +1, 0};
    int ret = semop(semId, &operation, 1);
    myassert(ret != -1, " ");
}

static int my_semget(char * kEY, int projectID)
{
    // TODO
    key_t cle = ftok(kEY, projectID);
    myassert(cle != -1, " ");
    int semId = semget(cle, 1, 0);
    myassert(semId != -1," ");
    return semId;
}


/************************************************************************
 * Partie multi-thread
 ************************************************************************/
//TODO Une structure pour les arguments à passer à un thread (aucune variable globale autorisée)

typedef struct
{

    float elt ;
    int indice ;
    float * tab ;
    int *res ;
    int size ;
    pthread_mutex_t *mutex ;
} thData;
//TODO
// Code commun à tous les threads
// Un thread s'occupe d'une portion du tableau et compte en interne le nombre de fois
// où l'élément recherché est présent dans cette portion. On ajoute alors,
// en section critique, ce nombre au compteur partagé par tous les threads.
// Le compteur partagé est la variable "result" de "lauchThreads".
// A vous de voir les paramètres nécessaires  (aucune variable globale autorisée)
//END TODO


void *thread_function(void *arg)
{
    thData *thdata = (thData *)arg;

    for (int i = thdata->indice; i < thdata->indice + thdata->size; i++)
    {
        if (thdata->elt == thdata->tab[i])
        {
            pthread_mutex_lock(thdata->mutex);
            *(thdata->res) += 1;
            pthread_mutex_unlock(thdata->mutex);
        }
    }

    return NULL;
}

void lauchThreads(const Data *data)
{
    int result = 0;
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    float *tab = ut_generateTab(data->nb, data->min, data->max, 0);

    pthread_t th[data->nbThreads];
    int range = data->nb / data->nbThreads;
    int rest_range = data->nb % data->nbThreads;
    thData *d = (thData *)malloc(sizeof(thData) * data->nbThreads);

    int avance = 0;
    int premierindice = 0;

    for (int i = 0; i < data->nbThreads; i++)
    {
        d[i].elt = data->elt;
        d[i].res = &result;
        d[i].mutex = &mutex;

        int indice, taille;
        if (rest_range > 0)
        {
            indice = (range * i) + avance;
            taille = range + 1;
            rest_range--;
            avance++;
        }
        else
        {
            indice = (range * i) + avance;
            taille = range;
        }

        d[i].indice = indice;
        d[i].size = taille;
        d[i].tab = tab;

        pthread_create(&th[i], NULL, &thread_function, &d[i]);
    }

    for (int i = 0; i < data->nbThreads; i++)
    {
        int join_res = pthread_join(th[i], NULL);
        myassert(join_res != -1, "Erreur lors de la jointure du thread");
    }

    // Affichage du tableau si pas trop gros
    if (data->nb <= 20)
    {
        printf("[");
        for (int i = 0; i < data->nb; i++)
        {
            if (i != 0)
                printf(" ");
            printf("%g", tab[i]);
        }
        printf("]\n");
    }

    // Recherche linéaire pour vérifier
    int nbVerif = 0;
    for (int i = 0; i < data->nb; i++)
    {
        if (tab[i] == data->elt)
            nbVerif++;
    }

    printf("Elément %g présent %d fois (%d attendu)\n", data->elt, result, nbVerif);
    if (result == nbVerif)
        printf("=> ok ! le résultat calculé par les threads est correct\n");
    else
        printf("=> PB ! le résultat calculé par les threads est incorrect\n");

    pthread_mutex_destroy(&mutex);
    free(tab);
    free(d);
}

/************************************************************************
 * Partie communication avec le master
 ************************************************************************/
// envoi des données au master
void sendData(const Data *data)
{


    //TODO


    int order = data->order ;
    int write_res = write(data->openRes, &(order), sizeof(int)) ;
    myassert(write_res != -1, " ") ;
    printf("L'order est %d \n", order) ;

    if(order == CM_ORDER_EXIST)
    {
        write_res = write(data->openRes, &(data->elt), sizeof(float)) ;
        myassert(write_res != -1, " ") ;
    }

    if(order == CM_ORDER_INSERT)
    {
        printf("l'elt %f en client \n", data->elt) ;
        float num = data->elt ;
        write_res = write(data->openRes, &(num), sizeof(float)) ;
        myassert(write_res != -1, " ") ;
    }

    if(order == CM_ORDER_INSERT_MANY)
    {

        float * tab = ut_generateTab(data->nb, data->min, data->max, 0);
        int taille = data->nb ;
        write_res = write(data->openRes, &(taille), sizeof(int)) ;
        for(int i =0 ; i< taille ; i ++)
        {
            write(data->openRes, &(tab[i]), sizeof(float)) ;

        }


        free(tab) ;
    }

    // - envoi de l'ordre au master (cf. CM_ORDER_* dans client_master.h)
    // - envoi des paramètres supplémentaires au master (pour CM_ORDER_EXIST,
    //   CM_ORDER_INSERT et CM_ORDER_INSERT_MANY)
    //END TODO
}

// attente de la réponse du master
void receiveAnswer(const Data *data)
{
    myassert(data != NULL, "pb !");   //TODO à enlever (présent pour éviter le warning)
    int read_res ;
    int accuse ;

    read_res = read(data->openRes, &accuse, sizeof(int) );
    myassert(read_res != -1, " ") ;
    printf("j'ai bien recu l'accuse : %d \n", accuse) ;
    if(data->order == MW_ORDER_MAXIMUM)
    {
        float rep ;
        read_res = read(data->openRes, &rep, sizeof(float) );
        printf("j'ai bien recu la reponse le max est : %f \n", rep) ;
        myassert(read_res != -1, " ") ;
    }
    if(data->order == MW_ORDER_MINIMUM)
    {
        float rep ;
        read_res = read(data->openRes, &rep, sizeof(float) );
        printf("j'ai bien recu la reponse le min est : %f \n", rep) ;
        myassert(read_res != -1, " ") ;
    }

    if(data->order == MW_ORDER_SUM)
    {
        float rep ;
        read_res = read(data->openRes, &rep, sizeof(float) );
        printf("j'ai bien recu la reponse la somme est : %f \n", rep) ;
        myassert(read_res != -1, " ") ;

    }

    if(data->order ==CM_ORDER_HOW_MANY)
    {
        int nb_elements ;
        int nb_all_elements ;

        read_res = read(data->openRes, &nb_elements, sizeof(int) );
        myassert(read_res != 0, " ") ;
        read_res = read(data->openRes, &nb_all_elements, sizeof(int) );
        myassert(read_res != 0, " ") ;
        printf("les nb d'elements differents : %d , nb total d'elements %d\n", nb_all_elements, nb_elements) ;

    }


    if(data->order ==CM_ORDER_EXIST)
    {
        if (accuse==CM_ANSWER_EXIST_NO)
        {
            printf("il n'existe pas \n") ;

        }

        else
        {
            int cardinality ;
            read_res = read(data->openRes, &cardinality, sizeof(int) );
            myassert(read_res != 0, " ") ;
            printf("il existe avec une cardinalite %d\n ", cardinality) ;

        }

    }








    //TODO
    // - récupération de l'accusé de réception du master (cf. CM_ANSWER_* dans client_master.h)
    // - selon l'ordre et l'accusé de réception :
    //      . récupération de données supplémentaires du master si nécessaire
    // - affichage du résultat
    //END TODO
}


/************************************************************************
 * Fonction principale
 ************************************************************************/
int main(int argc, char * argv[])
{

    Data data;
    parseArgs(argc, argv, &data);


    if (data.order == CM_ORDER_LOCAL)
        lauchThreads(&data);
    else
    {
        //TODO
        // - entrer en section critique :
        //       . pour empêcher que 2 clients communiquent simultanément
        //       . le mutex est déjà créé par le master
        // - ouvrir les tubes nommés (ils sont déjà créés par le master)
        int semId = my_semget(SC_CLIENTS,SC_ID);
        int precedence =my_semget(MUTEX_PRECEDENCE, PRECEDENCE_ID) ;
        entrerSC(semId) ;
        printf("je suis en SC \n") ;
        int open_CTM = open(FD_CTOM, O_WRONLY) ;
        myassert(open_CTM != -1, " ") ;

        data.openRes = open_CTM ;
        if(data.order == CM_ORDER_INSERT || data.order==CM_ORDER_EXIST)
        {
            data.elt = strtof(argv[2], NULL);
        }

        sendData(&data) ;
        int close_res =close(open_CTM);
        myassert(open_CTM != -1, " ") ;

        int open_MTC = open(FD_MTOC, O_RDONLY) ;
        myassert(open_MTC != -1," ") ;
        data.openRes = open_MTC ;
        receiveAnswer(&data) ;
        close_res = close(open_MTC) ;
        myassert(close_res != -1, " ") ;


        printf("fin client\n") ;

        sortirSC(semId) ;
        sortirSC(precedence) ;
        //       . les ouvertures sont bloquantes, il faut s'assurer que
        //         le master ouvre les tubes dans le même ordre
        //END TODO


        //TODO
        // - sortir de la section critique
        // - libérer les ressources (fermeture des tubes, ...)
        // - débloquer le master grâce à un second sémaphore (cf. ci-dessous)
        //


        // Une fois que le master a envoyé la réponse au client, il se bloque
        // sur un sémaphore ; le dernier point permet donc au master de continuer
        //
        // N'hésitez pas à faire des fonctions annexes ; si la fonction main
        // ne dépassait pas une trentaine de lignes, ce serait bien.
    }

    return EXIT_SUCCESS;
}

