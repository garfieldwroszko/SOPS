#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PTHREAD_CHECK(operacja)                                                \
  do {                                                                         \
    int err = (operacja);                                                      \
    if (err != 0) {                                                            \
      fprintf(stderr, "Blad podczas operacji na mutexie: %s\n",                \
              strerror(err));                                                  \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

#define MUTEX_LOCK(mutex_ptr) PTHREAD_CHECK(pthread_mutex_lock(mutex_ptr))
#define MUTEX_UNLOCK(mutex_ptr) PTHREAD_CHECK(pthread_mutex_unlock(mutex_ptr))

#define LICZBA_POWTORZEN_CZYTELNIKOW 8
#define LICZBA_POWTORZEN_PISARZY 5

int in_r = 0, in_w = 0, q_r = 0, q_w = 0;

pthread_mutex_t monitor_mutex;
pthread_cond_t mozna_czytac;
pthread_cond_t mozna_pisac;

void drukuj_stan() {
  printf("ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", q_r, q_w, in_r, in_w);
}

void *czytelnik(void *arg) {
  for (int i = 0; i < LICZBA_POWTORZEN_CZYTELNIKOW; i++) {
    usleep(80000);

    MUTEX_LOCK(&monitor_mutex);
    q_r++;
    drukuj_stan();

    while (in_w > 0) {
      PTHREAD_CHECK(pthread_cond_wait(&mozna_czytac, &monitor_mutex));
    }

    q_r--;
    in_r++;
    drukuj_stan();
    MUTEX_UNLOCK(&monitor_mutex);

    usleep(250000);

    MUTEX_LOCK(&monitor_mutex);
    in_r--;
    drukuj_stan();

    if (in_r == 0) {
      PTHREAD_CHECK(pthread_cond_signal(&mozna_pisac));
    }
    MUTEX_UNLOCK(&monitor_mutex);
  }
  return NULL;
}

void *pisarz(void *arg) {
  for (int i = 0; i < LICZBA_POWTORZEN_PISARZY; i++) {
    usleep(150000);

    MUTEX_LOCK(&monitor_mutex);
    q_w++;
    drukuj_stan();

    while (in_w > 0 || in_r > 0 || q_r > 0) {
      PTHREAD_CHECK(pthread_cond_wait(&mozna_pisac, &monitor_mutex));
    }

    q_w--;
    in_w = 1;
    drukuj_stan();
    MUTEX_UNLOCK(&monitor_mutex);

    usleep(500000);

    MUTEX_LOCK(&monitor_mutex);
    in_w = 0;
    drukuj_stan();

    if (q_r > 0) {
      PTHREAD_CHECK(pthread_cond_broadcast(&mozna_czytac));
    } else if (q_w > 0) {
      PTHREAD_CHECK(pthread_cond_signal(&mozna_pisac));
    }
    MUTEX_UNLOCK(&monitor_mutex);
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  int czytelnicy_suma = 5; // Domyslne wartosci
  int pisarze_suma = 3;
  int opt;

  while ((opt = getopt(argc, argv, "r:w:h")) != -1) {
    switch (opt) {
    case 'r':
      czytelnicy_suma = atoi(optarg);
      break;
    case 'w':
      pisarze_suma = atoi(optarg);
      break;
    case 'h':
      printf("Uzycie: %s [-r liczba_czytelnikow] [-w liczba_pisarzy]\n\n",
             argv[0]);
      printf("Opcje:\n");
      printf(
          "  -r <liczba>   Ustawia liczbe watkow czytelnikow (domyslnie: 5)\n");
      printf("  -w <liczba>   Ustawia liczbe watkow pisarzy (domyslnie: 3)\n");
      printf("  -h            Wyswietla ten komunikat pomocy\n");
      exit(EXIT_SUCCESS);
    default:
      fprintf(stderr, "Sprobuj '%s -h' aby uzyskac wiecej informacji.\n",
              argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  if (optind < argc) {
    fprintf(stderr, "FATAL: Podano nieprawidlowe argumenty!\n");
    fprintf(stderr, "Sprobuj '%s -h' aby uzyskac wiecej informacji.\n",
            argv[0]);
    exit(EXIT_FAILURE);
  }

  if (czytelnicy_suma <= 0 || pisarze_suma <= 0) {
    fprintf(stderr, "Argumenty musza byc liczbami wiekszymi od 0!\n");
    exit(EXIT_FAILURE);
  }

  pthread_t *czytelnicy = malloc(czytelnicy_suma * sizeof(pthread_t));
  pthread_t *pisarze = malloc(pisarze_suma * sizeof(pthread_t));

  if (!czytelnicy || !pisarze)
    exit(EXIT_FAILURE);

  PTHREAD_CHECK(pthread_mutex_init(&monitor_mutex, NULL));
  PTHREAD_CHECK(pthread_cond_init(&mozna_czytac, NULL));
  PTHREAD_CHECK(pthread_cond_init(&mozna_pisac, NULL));

  for (int i = 0; i < czytelnicy_suma; i++)
    PTHREAD_CHECK(pthread_create(&czytelnicy[i], NULL, czytelnik, NULL));
  for (int i = 0; i < pisarze_suma; i++)
    PTHREAD_CHECK(pthread_create(&pisarze[i], NULL, pisarz, NULL));

  for (int i = 0; i < czytelnicy_suma; i++)
    PTHREAD_CHECK(pthread_join(czytelnicy[i], NULL));
  for (int i = 0; i < pisarze_suma; i++)
    PTHREAD_CHECK(pthread_join(pisarze[i], NULL));

  free(czytelnicy);
  free(pisarze);
  PTHREAD_CHECK(pthread_mutex_destroy(&monitor_mutex));
  PTHREAD_CHECK(pthread_cond_destroy(&mozna_czytac));
  PTHREAD_CHECK(pthread_cond_destroy(&mozna_pisac));

  return 0;
}
