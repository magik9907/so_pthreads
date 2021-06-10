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
//mutex poczekalni
pthread_mutex_t waitingRoom;
//mutex zajętego miejsca u fryzjera
pthread_mutex_t barberSeat;
//semfar wskazujący czy fryzjer jest zajety
sem_t barberBusySemaphore;
//licznik klientów w poczekalni
sem_t waitingClientSemaphore;
pthread_t *clients;
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
    char *str = (char *)malloc(sizeof(char) * 30);
    write(1, "Clients: ", 9);
    QueueElem *elem = Clients;
    while (elem != NULL)
    {
        strcpy(str, "");
        sprintf(str, "%s -> %d", str, elem->id);
        write(1, str, strlen(str));
        strcpy(str, "");
        elem = elem->next;
    }

    strcpy(str, "");
    write(1, " Resigned clients: ", strlen(" Resigned clients: "));
    elem = Resigned;
    while (elem != NULL)
    {
        strcpy(str, "");
        sprintf(str, "%s -> %d", str, elem->id);
        write(1, str, strlen(str));
        elem = elem->next;
    }
    free(str);
}

//usowuanie klienta z kolejki Clients
QueueElem *removeClient(QueueElem *client)
{
    char *str = (char *)malloc(sizeof(char) * 100);
    QueueElem *curr = Clients;
    pthread_mutex_lock(&waitingRoom);

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
    pthread_mutex_unlock(&waitingRoom);
    free(str);
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
        sem_wait(&waitingClientSemaphore);
        sem_getvalue(&waitingClientSemaphore, &i);
        QueueElem *currentClient = removeClient(Clients);
        times = 900000 + (rand() % (maxShearTime - 900000 + 1));
        usleep(times);
        free(currentClient);
        currentClientID = -1;
    }
    free(str);
    pthread_exit(NULL);
}

//funckja klienta, obudzenie barbera (oczekiwanie na swoją kolej),
void *clientFunc(void *arg)
{
    QueueElem *curr = (QueueElem *)arg;

    pthread_mutex_lock(&waitingRoom);

    char *str = (char *)malloc(sizeof(char) * 100);
    sem_post(&waitingClientSemaphore);
    if (currentClientID != -1)
    {
        sprintf(str, "\nRes:%d WRoom: %d/%d [in: %d]", RESIGNEDCOUNT, CLIENTCOUNT, clientQueue, currentClientID);
        write(1, str, strlen(str));
        if (isDebug == 1)
        {
            printDebug();
        }
    }
    pthread_mutex_unlock(&waitingRoom);
    free(str);
    pthread_exit(NULL);
}

//dodanie klienta lub jego rezygnacja
void addClient(int id)
{
    char *str = (char *)malloc(sizeof(char) * 100);
    QueueElem *newClient = (QueueElem *)malloc(sizeof(QueueElem));
    newClient->id = id;
    newClient->next = NULL;
    pthread_mutex_lock(&waitingRoom);
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
    }
    else
    {
        CLIENTCOUNT++;
        if (ClientsTop == NULL)
        {
            Clients = newClient;
        }
        else
        {
            ClientsTop->next = newClient;
        }
        ClientsTop = newClient;
        if (pthread_create(&(newClient->thread), NULL, &clientFunc, (void *)newClient) != 0)
        {
            write(1, "create Thread client error\n", 31);
        }
    }
    pthread_mutex_unlock(&waitingRoom);
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
    clients = (pthread_t *)malloc(sizeof(pthread_t) * clientQueue);
    sem_init(&barberBusySemaphore, 0, 0);
    sem_init(&waitingClientSemaphore, 0, 0);
    pthread_mutex_init(&waitingRoom, NULL);
    pthread_mutex_init(&barberSeat, NULL);

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
    sem_destroy(&barberBusySemaphore);

    QueueElem *curr = Clients, *next;
    while (curr != NULL)
    {
        next = curr->next;
        curr->next = NULL;
        free(curr);
        curr = next;
    }
    curr = Resigned;
    next = NULL;
    while (curr != NULL)
    {
        next = curr->next;
        curr->next = NULL;
        free(curr);
        curr = next;
    }
    return 0;
}