#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define LICZBA_CZYTELNIKOW 5
#define LICZBA_PISARZY 3
#define LICZBA_POWTORZEN 5

int aktywni_czytelnicy = 0;
int aktywny_pisarz = 0;
int czekajacy_pisarze = 0;

pthread_mutex_t mutex;
pthread_cond_t mozna_czytac;
pthread_cond_t mozna_pisac;

void *czytelnik(void *arg)
{
    int id = *(int *)arg;

    for (int i = 0; i < LICZBA_POWTORZEN; i++)
    {
        printf("Czytelnik %d chce wejsc do czytelni.\n", id);

        pthread_mutex_lock(&mutex);

        while (aktywny_pisarz || czekajacy_pisarze > 0)
        {
            printf("Czytelnik %d czeka, bo pisarz pisze albo czeka.\n", id);
            pthread_cond_wait(&mozna_czytac, &mutex);
        }

        aktywni_czytelnicy++;

        printf("Czytelnik %d wchodzi. Aktywni czytelnicy: %d\n",
               id, aktywni_czytelnicy);

        pthread_mutex_unlock(&mutex);

        usleep(300000);

        pthread_mutex_lock(&mutex);

        aktywni_czytelnicy--;

        printf("Czytelnik %d wychodzi. Aktywni czytelnicy: %d\n",
               id, aktywni_czytelnicy);

        if (aktywni_czytelnicy == 0)
        {
            pthread_cond_signal(&mozna_pisac);
        }

        pthread_mutex_unlock(&mutex);

        usleep(200000);
    }

    return NULL;
}

void *pisarz(void *arg)
{
    int id = *(int *)arg;

    for (int i = 0; i < LICZBA_POWTORZEN; i++)
    {
        usleep(100000);

        printf("\n>>> Pisarz %d chce wejsc do czytelni.\n", id);

        pthread_mutex_lock(&mutex);

        czekajacy_pisarze++;

        while (aktywny_pisarz || aktywni_czytelnicy > 0)
        {
            printf(">>> Pisarz %d czeka. Aktywni czytelnicy: %d, aktywny pisarz: %d\n",
                   id, aktywni_czytelnicy, aktywny_pisarz);

            pthread_cond_wait(&mozna_pisac, &mutex);
        }

        czekajacy_pisarze--;
        aktywny_pisarz = 1;

        printf("\n>>> Pisarz %d wchodzi i PISZE. Czekajacy pisarze: %d\n",
               id, czekajacy_pisarze);

        pthread_mutex_unlock(&mutex);

        usleep(500000);

        pthread_mutex_lock(&mutex);

        aktywny_pisarz = 0;

        printf("\n>>> Pisarz %d wychodzi z czytelni.\n", id);

        if (czekajacy_pisarze > 0)
        {
            pthread_cond_signal(&mozna_pisac);
        }
        else
        {
            pthread_cond_broadcast(&mozna_czytac);
        }

        pthread_mutex_unlock(&mutex);

        usleep(100000);
    }

    return NULL;
}

int main()
{
    pthread_t czytelnicy[LICZBA_CZYTELNIKOW];
    pthread_t pisarze[LICZBA_PISARZY];

    int id_czytelnikow[LICZBA_CZYTELNIKOW];
    int id_pisarzy[LICZBA_PISARZY];

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&mozna_czytac, NULL);
    pthread_cond_init(&mozna_pisac, NULL);

    for (int i = 0; i < LICZBA_CZYTELNIKOW; i++)
    {
        id_czytelnikow[i] = i + 1;
        pthread_create(&czytelnicy[i], NULL, czytelnik, &id_czytelnikow[i]);
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

    pthread_cond_destroy(&mozna_czytac);
    pthread_cond_destroy(&mozna_pisac);
    pthread_mutex_destroy(&mutex);

    return 0;
}
