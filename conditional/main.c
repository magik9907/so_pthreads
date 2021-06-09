#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <getopt.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>

//struktura elementu kolejki
struct Node
{
    int id;
    pthread_t thread;
    struct Node *next;
};
typedef struct Node QueueElem;

// kolejka użytkowników którzy zrezygnowali
QueueElem *Resigned = NULL;
//wskaźnik na ostatni element kolejki
QueueElem *ResignedTop = NULL;
int currentClientID = -1;
//Kolejka klientów w poczekalni
QueueElem *Clients;
//wskaźnik na ostatni element kolejki
QueueElem *ClientsTop;
//mutex blokujący modyfikacje kolejki dla jednej osoby
pthread_mutex_t QueueMutex = PTHREAD_MUTEX_INITIALIZER;
//mutex blokujący funckje klienta
pthread_mutex_t ClientMutex = PTHREAD_MUTEX_INITIALIZER;
//mutex blokujący barbera
pthread_mutex_t BarberMutex = PTHREAD_MUTEX_INITIALIZER;
//zmienna warunkowa
pthread_cond_t BarberCond = PTHREAD_COND_INITIALIZER;
//wątek fryzjera
pthread_t barber;

//licznik klientow
int CLIENTCOUNT = 0;
//licznik klientów którzy zrezygnowali
int RESIGNEDCOUNT = 0;
//liczba miejsc w poczekalni
int clientQueue = 10;
//liczba klientów
int clientCount = 100;
//czas strzyzenia
int maxShearTime = 1500000;
//czas przeybycia klientóœ
int maxClientArriveTime = 1000000;
//tryb debugowania
int isDebug = 0;
//liczba wszsytkich klientów
int allClients = 0;

//wypisywanie kolejek z trybei debugowania
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

//usowuanie klienta z kolejki Clients
QueueElem *removeClient(QueueElem *client)
{
    pthread_mutex_lock(&QueueMutex);
    char *str = (char *)malloc(sizeof(char) * 100);
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
    currentClientID = client->id;
    sprintf(str, "\nRes:%d WRoom: %d/%d [in: %d]", RESIGNEDCOUNT, CLIENTCOUNT, clientQueue, currentClientID);
    write(1, str, strlen(str));
    if (isDebug)
        printDebug();
    free(str);
    pthread_mutex_unlock(&QueueMutex);
    return client;
}

//funckja barbera, oczekiwanie na klienta, strzyżenie,
void *barberFunc()
{
    int i;
    char *str = (char *)malloc(sizeof(char) * 100);
    long long times;
    while (!allClients || Clients != NULL)
    {
        pthread_mutex_lock(&BarberMutex);
        //While instead of if
        //Because the variable like to change by itself every now and then
        while (Clients == NULL)
        {
            //wątek fryzjera
            pthread_cond_wait(&BarberCond, &BarberMutex);
        }
        //We're removing the client from queue
        //And keeping him in a room with the barber
        QueueElem *currentClient = removeClient(Clients);
        times = 900000 + (rand() % (maxShearTime - 900000 + 1));
        usleep(times);
        //Client's hair has been cut - we can let him go
        free(currentClient);
        currentClientID = -1;
        // sprintf(str, "\nclient run %lld queue insert ", Clients->id);
        // write(1, str, strlen(str));
        pthread_mutex_unlock(&BarberMutex);
    }
    free(str);
    pthread_exit(NULL);
}

//funckja klienta, obudzenie barbera (oczekiwanie na swoją kolej),
void *clientFunc(void *arg)
{
    pthread_mutex_lock(&ClientMutex);
    QueueElem *curr = (QueueElem *)arg;

    char *str = (char *)malloc(sizeof(char) * 100);

    CLIENTCOUNT++;
    sprintf(str, "\nRes:%d WRoom: %d/%d [in: %d]", RESIGNEDCOUNT, CLIENTCOUNT, clientQueue, currentClientID);
    write(1, str, strlen(str));
    if (isDebug == 1)
    {
        printDebug();
    }
    //wątek fryzjera
    pthread_cond_signal(&BarberCond);

    pthread_mutex_unlock(&ClientMutex);
    free(str);
    pthread_exit(NULL);
}

//dodanie klienta lub jego rezygnacja
void addClient(int id)
{
    pthread_mutex_lock(&QueueMutex);
    char *str = (char *)malloc(sizeof(char) * 100);
    QueueElem *newClient = (QueueElem *)malloc(sizeof(QueueElem));
    newClient->id = id;
    newClient->next = NULL;

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
        sprintf(str, "\nRes:%d WRoom: %d/%d [in: %d]", RESIGNEDCOUNT, CLIENTCOUNT, clientQueue, currentClientID);
        write(1, str, strlen(str));
        if (isDebug == 1)
        {
            printDebug();
        }
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
    currentClientID = -1;
    srand(time(NULL));
    int option;
    while ((option = getopt(argc, argv, "q:s:c:t:d")) != -1)
    {
        switch (option)
        {
        case 'q':
            clientQueue = atoi(optarg);
            if (clientQueue < 1)
            {
                write(1, "client queue size must be greater than 0", strlen("client queue size must be greater than 0"));
                return -1;
            }
            break;
        case 's':
            maxShearTime = atoi(optarg);
            if (maxShearTime < 900000)
            {
                write(1, "max shear time must be greater than 900000", strlen("max shear time must be greater than 900000"));
                return -1;
            }
            break;
        case 'c':
            clientCount = atoi(optarg);
            if (clientCount < 1)
            {
                write(1, "client count must be greater than 0", strlen("client count must be greater than 0"));
                return -1;
            }
            break;
        case 't':
            maxClientArriveTime = atoi(optarg);
            if (maxClientArriveTime < 1000000)
            {
                write(1, "max client time arrival must be greater than 1000000", strlen("max client time arrival must be greater than 1000000"));
                return -1;
            }
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
        times = minClientTimeArrive + (rand() % (maxClientArriveTime - minClientTimeArrive + 1));
        usleep(times);
        addClient(i);
    }
    allClients = 1;
    pthread_join(barber, NULL);
    // sprintf(str, "add newClient %d\n", curr->id);
    // write(1, str, strlen(str));
    QueueElem *curr = Clients, *next;
    while (curr != NULL)
    {
        next = curr->next;
        curr->next = NULL;
        free(curr);
        curr =next;
    }
    curr = Resigned;
    next = NULL;
    while (curr != NULL)
    {
        next = curr->next;
        curr->next = NULL;
        free(curr);
        curr=next;
    }
    return 0;
}