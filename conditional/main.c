#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <getopt.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>

struct Node
{
    int id;
    pthread_t thread;
    // pthread_mutex_t mutex;
    struct Node *next;
};
typedef struct Node QueueElem;

QueueElem *Clients;
QueueElem *ClientsTop;
QueueElem *Resigned = NULL;
QueueElem *ResignedTop = NULL;
pthread_mutex_t QueueMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ClientMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t BarberMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t BarberCond = PTHREAD_COND_INITIALIZER;
pthread_t barber;

int CLIENTCOUNT = 0;
int RESIGNEDCOUNT = 0;
//liczba miejsc w po+czekalni
volatile int clientQueue = 10;
//liczba klientów
volatile int clientCount = 100;
//czas strzyzenia
volatile int maxShearTime = 9000000;
//czas przeybycia klientóœ
volatile int maxClientArriveTime = 1 * 1000000;
//tryb debugowania
volatile int isDebug = 0;

int allClients = 0;

void printDebug()
{
    char *str = (char *)malloc(sizeof(char) * 300);
    strcpy(str, "Clients: ");
    QueueElem *elem = Clients;
    while (elem != NULL)
    {
        sprintf(str, "%s -> %d", str, elem->id);
        elem = elem->next;
    }

    strcat(str, " Resigned clients: ");
    elem = Resigned;
    while (elem != NULL)
    {
        sprintf(str, "%s -> %d", str, elem->id);
        elem = elem->next;
    }
    write(1, str, strlen(str));
    free(str);
}

void removeClient(QueueElem *client)
{
    pthread_mutex_lock(&QueueMutex);
    char *str = (char *)malloc(sizeof(char) * 100);
    // sprintf(str, "client try remove %d \n", client->id);
    // write(1, str, strlen(str));
    QueueElem *curr = Clients;

    if (curr->id == client->id)
    {
        if (curr->next != NULL)
        {
            Clients = curr->next;
        }
        else
        {
            ClientsTop = NULL;
            Clients = NULL;
        }
    }
    else
    {
        while (curr->next->id != client->id)
        {
            curr = curr->next;
        }
        curr->next = NULL;
    }
    CLIENTCOUNT--;

    pthread_mutex_unlock(&QueueMutex);
    free(str);
    free(client);
}

void *barberFunc()
{
    int i;
    char *str = (char *)malloc(sizeof(char) * 100);
    long long times;
    while (!allClients || Clients != NULL)
    {
        pthread_mutex_lock(&BarberMutex);
        if (Clients == NULL)
        {
            pthread_cond_wait(&BarberCond, &BarberMutex);
        }
        times = 100000 + (rand() / ((maxShearTime + 1)) * 10000);
        usleep(times);

        // sprintf(str, "\nclient run %lld queue insert ", Clients->id);
        // write(1, str, strlen(str));
        removeClient(Clients);
        pthread_mutex_unlock(&BarberMutex);
    }
    free(str);
    pthread_exit(NULL);
}

void *clientFunc(void *arg)
{
    pthread_mutex_lock(&ClientMutex);
    QueueElem *curr = (QueueElem *)arg;

    char *str = (char *)malloc(sizeof(char) * 100);
    // sprintf(str, "add newClient %d\n", curr->id);
    // write(1, str, strlen(str));
    // int i;
    CLIENTCOUNT++;
    sprintf(str, "\nRes:%d WRomm: %d/%d [in: %d]", RESIGNEDCOUNT, CLIENTCOUNT, clientQueue, Clients->id);
    write(1, str, strlen(str));
    if (isDebug == 1)
    {
        printDebug();
    }
    pthread_cond_signal(&BarberCond);

    // sprintf(str, "add newClient %d\n", curr->id);
    //     write(1, str, strlen(str));
    pthread_mutex_unlock(&ClientMutex);

    free(str);
    pthread_exit(NULL);
}

void addClient(int id)
{
    pthread_mutex_lock(&QueueMutex);
    char *str = (char *)malloc(sizeof(char) * 100);
    QueueElem *newClient = (QueueElem *)malloc(sizeof(QueueElem));
    newClient->id = id;
    newClient->next = NULL;
    // sprintf(str, "newclient run %d  \n", newClient->id);
    // write(1, str, strlen(str));
    // pthread_mutex_init(&(newClient->mutex), NULL);

    if (CLIENTCOUNT >= 10)
    {
        if (Resigned == NULL)
            Resigned = newClient;
        else
        {
            ResignedTop->next = newClient;
        }
        ResignedTop = newClient;
        RESIGNEDCOUNT++;
        sprintf(str, "\nRes:%d WRomm: %d/%d [in: %d]", RESIGNEDCOUNT, CLIENTCOUNT, clientQueue, (Clients != NULL) ? Clients->id : 0);
        write(1, str, strlen(str));
        if (isDebug == 1)
        {
            printDebug();
        }
        // write(1, "create Thread client error\n", 31);
        // sem_post(&resignedClientsSemaphore);
        pthread_mutex_unlock(&QueueMutex);
    }
    else
    {
        write(1, str, strlen(str));
        if (ClientsTop == NULL)
        {
            Clients = newClient;
        }
        else
        {
            ClientsTop->next = newClient;
        }
        ClientsTop = newClient;
        pthread_mutex_unlock(&QueueMutex);
        if (pthread_create(&(newClient->thread), NULL, &clientFunc, (void *)newClient) != 0)
        {
            write(1, "create Thread client error\n", 31);
        }
    }
    free(str);
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    int option;
    while ((option = getopt(argc, argv, "q:s:c:t:d")) != -1)
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
    // pthread_mutex_init(&barberSeat, NULL);

    if (pthread_create(&barber, NULL, &barberFunc, NULL) != 0)
    {
        write(1, "create Thread barber error\n", 31);
        return -1;
    }
    char *str = (char *)malloc(sizeof(char) * 100);

    int i = 0;
    int counter;
    long long times;
    int minClientTimeArrive = 100;
    for (i = 0; i < clientCount; i++)
    {
        times = minClientTimeArrive + (rand() / ((maxClientArriveTime + 1)) * 1000);
        usleep(times);
        addClient(i);
    }
    allClients = 1;
    pthread_join(barber, NULL);
    return 0;
}
// char *str = (char *)malloc(sizeof(char) * 100);
// sprintf(str, "client run %d queue insert \n", client->id);
// write(1, str, strlen(str));
// free(str);
