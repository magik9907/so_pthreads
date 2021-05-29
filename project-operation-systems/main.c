#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "function.h"
#include <pthread.h>
#include <getopt.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>

extern struct Node
{
    int id;
    pthread_t thread;
    pthread_mutex_t mutex;
    struct Node *next;
};
typedef struct Node QueueElem;

QueueElem *Clients;
QueueElem *ClientsTop;

sem_t barberBusySemaphore;
sem_t clientCountSemaphore;
pthread_mutex_t barberSeat;
pthread_mutex_t barberSleep;
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

void removeClient(QueueElem *client)
{
    char *str = (char *)malloc(sizeof(char) * 100);
    sprintf(str, "client try remove %d \n", client->id);
    write(1, str, strlen(str));
    QueueElem *curr = Clients;

    if (curr->id == client->id)
    {
        if (curr->next != NULL)
            Clients = curr->next;
        else
            Clients = NULL;
    }
    else
    {
        while (curr->next->id != client->id)
        {
            curr = curr->next;
        }
        curr->next = NULL;
    }
    sprintf(str, "client end remove %d \n", client->id);
    write(1, str, strlen(str));
    free(str);
    free(client);
}

void *barberFunc()
{
    char *str = (char *)malloc(sizeof(char) * 100);
    write(1, "barber run\n", 11);
    write(1, "barber lock\n", 12);
    // pthread_mutex_unlock(&barberSleep);
    // write(1, "barber unlock\n", 14);
    int val, time;
    QueueElem *client;
    int semaphoreValue;
    while (1)
    {
        client = Clients;
        if (client == NULL)
        {
            sprintf(str, "wait for client \n");
            write(1, str, strlen(str));

            sem_init(&barberBusySemaphore, 0, 1);
            val = 1;
            do
            {
                sem_getvalue(&barberBusySemaphore, &val);
            } while (val == 1);
            sprintf(str, "barber wakeup client found \n");
            write(1, str, strlen(str));
        }
        else
        {
            sprintf(str, "sleep barber shear %d\n", client->id);
            write(1, str, strlen(str));
            time = 1000000 + rand() % (maxShearTime + 1 - 1000000);
            usleep(time);
            sprintf(str, "remove client from seat %d\n", client->id);
            write(1, str, strlen(str));
            removeClient(client);
            sem_getvalue(&clientCountSemaphore, &semaphoreValue);
            sem_init(&clientCountSemaphore, 0, semaphoreValue - 1);
            sprintf(str, "decrement clinet in queue \n", client->id);
            write(1, str, strlen(str));
        }
        client = NULL;
    }

    free(str);
    pthread_exit(NULL);
}

void *clientFunc(void *arg)
{
    QueueElem *client = (QueueElem *)malloc(sizeof(QueueElem));
    client = (QueueElem *)arg;
    char *str = (char *)malloc(sizeof(char) * 100);
    int semaphoreValue;
    sem_getvalue(&clientCountSemaphore, &semaphoreValue);
    if (semaphoreValue >= 10)
    {
        sprintf(str, "queue full, client exit %d\n", client->id);
        write(1, str, strlen(str));
        removeClient(client);
        free(str);
        pthread_exit(NULL);
    }
    else if (pthread_mutex_trylock(&barberSeat) == 0)
    {
        sprintf(str, "client run %d wakeup barber \n", client->id);
        write(1, str, strlen(str));
    }
    else
    {
        sem_init(&clientCountSemaphore, 0, semaphoreValue + 1);
        sprintf(str, "client run %d queue insert \n", client->id);
        write(1, str, strlen(str));
    }
    free(str);
}

void addClient(int id)
{
    QueueElem *newClient = (QueueElem *)malloc(sizeof(QueueElem));
    newClient->id = id;
    pthread_mutex_init(&(newClient->mutex), NULL);
    if (ClientsTop == NULL)
        ClientsTop = newClient;
    else
        ClientsTop->next = newClient;
    if (Clients == NULL)
        Clients = newClient;
    if (pthread_create(&(newClient->thread), NULL, &clientFunc, (void *)newClient) != 0)
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
    sem_init(&clientCountSemaphore, 0, 0);
    pthread_mutex_init(&barberSleep, NULL);
    pthread_mutex_init(&barberSeat, NULL);

    if (pthread_create(&barber, NULL, &barberFunc, NULL) != 0)
    {
        write(1, "create Thread barber error\n", 31);
        return -1;
    }
    int i = 0;
    int counter;
    int times;
    int minClientTimeArrive = 1000000;
    for (i = 0; i < clientCount; i++)
    {
        times = minClientTimeArrive + rand() % (maxClientArriveTime + 1 - minClientTimeArrive);
        usleep(times);
        addClient(i);
    }

    pthread_join(barber, NULL);
    sem_destroy(&barberBusySemaphore);
    sem_destroy(&clientCountSemaphore);
    free(clients);
    return 0;
}
