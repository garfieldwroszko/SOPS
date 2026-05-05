#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define LICZBA_CZYTELNIKOW 8
#define LICZBA_PISARZY 1
#define LICZBA_POWTORZEN_CZYTELNIKOW 20
#define LICZBA_POWTORZEN_PISARZY 3

int liczba_czytelnikow = 0;

pthread_mutex_t mutex_licznik;
pthread_mutex_t mutex_czytelnia;

void *czytelnik(void *arg)
{
    int id = *(int *)arg;

    for (int i = 0; i < LICZBA_POWTORZEN_CZYTELNIKOW; i++)
    {
        printf("Czytelnik %d chce wejsc do czytelni.\n", id);

        pthread_mutex_lock(&mutex_licznik);

        liczba_czytelnikow++;

        if (liczba_czytelnikow == 1)
        {
            pthread_mutex_lock(&mutex_czytelnia);
        }

        printf("Czytelnik %d wchodzi. Liczba czytelnikow: %d\n",
               id, liczba_czytelnikow);

        pthread_mutex_unlock(&mutex_licznik);

        usleep(300000 + (id * 30000)); // symulacja czytania: ok. 0.3-0.54 s

        pthread_mutex_lock(&mutex_licznik);

        liczba_czytelnikow--;

        printf("Czytelnik %d wychodzi. Liczba czytelnikow: %d\n",
               id, liczba_czytelnikow);

        if (liczba_czytelnikow == 0)
        {
            pthread_mutex_unlock(&mutex_czytelnia);
        }

        pthread_mutex_unlock(&mutex_licznik);

        usleep(20000); // bardzo krotka przerwa przed ponownym wejsciem
    }

    return NULL;
}

void *pisarz(void *arg)
{
    int id = *(int *)arg;

    sleep(1); // pisarz pojawia sie, gdy czytelnicy juz dzialaja

    for (int i = 0; i < LICZBA_POWTORZEN_PISARZY; i++)
    {
        printf("\n>>> Pisarz %d chce wejsc do czytelni.\n", id);

        pthread_mutex_lock(&mutex_czytelnia);

        printf("\n>>> Pisarz %d PISZE.\n", id);

        sleep(1);

        printf("\n>>> Pisarz %d wychodzi z czytelni.\n", id);

        pthread_mutex_unlock(&mutex_czytelnia);

        sleep(1);
    }

    return NULL;
}

int main()
{
    pthread_t czytelnicy[LICZBA_CZYTELNIKOW];
    pthread_t pisarze[LICZBA_PISARZY];

    int id_czytelnikow[LICZBA_CZYTELNIKOW];
    int id_pisarzy[LICZBA_PISARZY];

    pthread_mutex_init(&mutex_licznik, NULL);
    pthread_mutex_init(&mutex_czytelnia, NULL);

    for (int i = 0; i < LICZBA_CZYTELNIKOW; i++)
    {
        id_czytelnikow[i] = i + 1;
        pthread_create(&czytelnicy[i], NULL, czytelnik, &id_czytelnikow[i]);

        usleep(50000); // male opoznienie startu kolejnych czytelnikow
    }

    for (int i = 0; i < LICZBA_PISARZY; i++)
    {
        id_pisarzy[i] = i + 1;
        pthread_create(&pisarze[i], NULL, pisarz, &id_pisarzy[i]);
    }

    for (int i = 0; i < LICZBA_CZYTELNIKOW; i++)
    {
        pthread_join(czytelnicy[i], NULL);
    }

    for (int i = 0; i < LICZBA_PISARZY; i++)
    {
        pthread_join(pisarze[i], NULL);
    }

    pthread_mutex_destroy(&mutex_licznik);
    pthread_mutex_destroy(&mutex_czytelnia);

    return 0;
}
