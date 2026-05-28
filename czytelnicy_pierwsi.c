#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Makro do obslugi bledow funkcji pthreads
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

// Zmienne stanu pelniace jednoczesnie role zmiennych warunkowych dla monitora
int in_r = 0, in_w = 0, q_r = 0, q_w = 0;

// Elementy monitora
pthread_mutex_t monitor_mutex; // Mutex chroniacy dostep do monitora
pthread_cond_t mozna_czytac;   // Zmienna warunkowa dla oczekujacych czytelnikow
pthread_cond_t mozna_pisac;    // Zmienna warunkowa dla oczekujacych pisarzy

// Funkcja odpowiedzialna za formatowane wypisywanie stanu algorytmu
void drukuj_stan() {
  printf("ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", q_r, q_w, in_r, in_w);
}

void *czytelnik(void *arg) {
  for (int i = 0; i < LICZBA_POWTORZEN_CZYTELNIKOW; i++) {
    usleep(80000);

    MUTEX_LOCK(&monitor_mutex);
    q_r++; // Dodanie do kolejki oczekujacych
    drukuj_stan();

    // Problem Czytelnikow i Pisarzy I: Faworyzowanie czytelnikow.
    // Czytelnik czeka TYLKO wtedy, gdy w czytelni znajduje sie pisarz.
    while (in_w > 0) {
      PTHREAD_CHECK(pthread_cond_wait(&mozna_czytac, &monitor_mutex));
    }

    q_r--;
    in_r++; // Wejscie do czytelni
    drukuj_stan();
    MUTEX_UNLOCK(&monitor_mutex);

    // Pobyt w czytelni - czytanie
    usleep(250000);

    MUTEX_LOCK(&monitor_mutex);
    in_r--; // Opuszczenie czytelni
    drukuj_stan();

    // Ostatni wychodzacy czytelnik wybudza oczekujacego pisarza
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
    q_w++; // Dodanie do kolejki oczekujacych
    drukuj_stan();

    // Pisarz czeka, gdy czytelnia nie jest pusta lub gdy czekaja nowi
    // czytelnicy (faworyzowanie R)
    while (in_w > 0 || in_r > 0 || q_r > 0) {
      PTHREAD_CHECK(pthread_cond_wait(&mozna_pisac, &monitor_mutex));
    }

    q_w--;
    in_w = 1; // Wejscie do czytelni (blokada wylaczna)
    drukuj_stan();
    MUTEX_UNLOCK(&monitor_mutex);

    // Pobyt w czytelni - pisanie
    usleep(500000);

    MUTEX_LOCK(&monitor_mutex);
    in_w = 0; // Opuszczenie czytelni
    drukuj_stan();

    // Wybudzanie zgodne z priorytetem: w pierwszej kolejnosci wszyscy
    // oczekujacy czytelnicy
    if (q_r > 0) {
      PTHREAD_CHECK(pthread_cond_broadcast(&mozna_czytac));
    } else if (q_w > 0) {
      // Wybudzenie pisarza tylko w przypadku braku czytelnikow
      PTHREAD_CHECK(pthread_cond_signal(&mozna_pisac));
    }
    MUTEX_UNLOCK(&monitor_mutex);
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  int czytelnicy_suma = 5;
  int pisarze_suma = 3;

  // Parsowanie argumentow linii polecen
  if (argc == 3) {
    czytelnicy_suma = atoi(argv[1]);
    pisarze_suma = atoi(argv[2]);
    if (czytelnicy_suma <= 0 || pisarze_suma <= 0) {
      printf("Zle podane argumenty\n");
      exit(EXIT_FAILURE);
    }
  }

  // Dynamiczna alokacja watkow
  pthread_t *czytelnicy = malloc(czytelnicy_suma * sizeof(pthread_t));
  pthread_t *pisarze = malloc(pisarze_suma * sizeof(pthread_t));
  if (!czytelnicy || !pisarze)
    exit(EXIT_FAILURE);

  // Inicjalizacja narzedzi synchronizacji
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

  // Zwalnianie zasobow
  free(czytelnicy);
  free(pisarze);
  PTHREAD_CHECK(pthread_mutex_destroy(&monitor_mutex));
  PTHREAD_CHECK(pthread_cond_destroy(&mozna_czytac));
  PTHREAD_CHECK(pthread_cond_destroy(&mozna_pisac));

  return 0;
}
