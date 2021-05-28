#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "function.h"
#include <pthread.h>
#include <getopt.h>
#include <semaphore.h>
#include <string.h>

extern struct Node
{
    int id;
    pthread_t thread;
    struct Node *next;
};
typedef struct Node QueueElem;
QueueElem *Clients;
QueueElem *ClientsTop;

sem_t barberBusySemaphore;
sem_t clientCountSemaphor;
pthread_mutex_t barberSeat;
pthread_t *clients;
pthread_t barber;
//liczba miejsc w po+czekalni
int clientQueue = 10;
//liczba klientów
int clientCount = 100;
//czas strzyzenia
int maxShearTime = 1000000;
//czas przeybycia klientóœ
int maxClientArriveTime = 1 * 1000000;
//tryb debugowania
int isDebug = 0;

void *barberFunc()
{
    write(1, "barber run\n", 11);

    pthread_exit(NULL);
}

void *clientFunc(void *arg)
{
    int *i = (int *)malloc(sizeof(int));
    *i = (int *)arg;
    char *str = (char *)malloc(sizeof(char) * 100);

    sprintf(str, "client run %d\n", *i);
    write(1, str, strlen(str));

    free(i);
    free(str);
    pthread_exit(NULL);
}

void addClient(int id)
{
    QueueElem *newClient = (QueueElem *)malloc(sizeof(QueueElem));
    newClient->id = id;
    if (ClientsTop == NULL)
        ClientsTop = newClient;
    else
        ClientsTop->next = newClient;
    if (Clients == NULL)
        Clients = newClient;
    ClientsTop = newClient;
    if (pthread_create(&(newClient->thread), NULL, &clientFunc, (void *)id) != 0)
    {
        write(1, "create Thread client error\n", 31);
    }
}

int main(int argc, char *argv[])
{

    srand(time(NULL));
    int option;
    while ((option = getopt(argc, argv, "q:s:c:t:d") != -1))
    {
        switch (option)
        {
        case 'q':
            clientQueue = atoi(optarg) + 1;
            break;
        case 's':
            maxShearTime = atoi(optarg);
            break;
        case 'c':
            clientCount = atoi(optarg);
            break;
        case 't':
            maxClientArriveTime = atoi(optarg) * 1000000;
            break;
        case 'd':
            isDebug = 1;
            break;
        }
    }

    //alokacja pamieci pthread
    clients = (pthread_t *)malloc(sizeof(pthread_t) * clientQueue);
    sem_init(&barberBusySemaphore, 0, 0);
    sem_init(&clientCountSemaphor, 0, 0);
    if (pthread_create(&barber, NULL, &barberFunc, NULL) != 0)
    {
        write(1, "create Thread barber error\n", 31);
        return -1;
    }
    int i = 0;
    int counter;
    int time;
    int minClientTimeArrive = 1000000;
    for (i = 0; i < clientCount; i++)
    {
        time = minClientTimeArrive + rand() % (maxClientArriveTime + 1 - minClientTimeArrive);
        usleep(time);
        addClient(i);
    }

    pthread_join(barber, NULL);
    sem_destroy(&barberBusySemaphore);
    sem_destroy(&clientCountSemaphor);
    free(clients);
    return 0;
}
