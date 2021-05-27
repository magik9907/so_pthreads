#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "function.h"
#include <pthread.h>
#include <getopt.h>
#include <semaphore.h>



void *client(){
    printf("run");

    thread_exit(NULL);
}

int main(int argc, char *argv[])
{

    srand(time(NULL));
    //liczba miejsc w poczekalni
    int clientQueue = 10;
    //liczba obecnie klientóœ
    int clientCurrCount = 0;
    //czas strzyzenia
    int maxShearTime = 1000000;
    //czas przeybycia klientóœ
    int maxClientArriveTime = 1000000;
    int isDebug = 0;
    int option;
    while ((option = getopt(argc, argv, "q:s:c:t:d") != -1))
    {
        switch (option)
        {
        case 'q':
            clientQueue = atoi(optarg);
            break;
        case 's':
            maxShearTime = atoi(optarg);
            break;
        case 'c':
            clientCurrCount = atoi(optarg);
            break;
        case 't':
            maxShearTime = atoi(optarg);
            break;
        case 'd':
            isDebug = 1;
            break;
        }
    }



    return 0;
}
