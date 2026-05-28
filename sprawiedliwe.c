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

// Zmienna weryfikujaca ilosc czytelnikow w czytelni
int czytelnicy_w_czytelni = 0;

// Zmienne pomocnicze sluzace wylacznie do logowania stanu
int in_r = 0;
int in_w = 0;
int q_r = 0;
int q_w = 0;

// Mutexy realizujace algorytm zapobiegajacy zaglodzeniu
pthread_mutex_t bramka;          // Bramka  wymuszajaca kolejkowanie
pthread_mutex_t mutex_czytelnia; // Mutex chroniacy wejscie do czytelni
pthread_mutex_t
    mutex_aktywni;         // Mutex chroniacy wspoldzielony licznik czytelnikow
pthread_mutex_t mutex_log; // Mutex logowania

// Funkcja zmieniajace wartosci logowania i wypisywanie stanu
void zmien_stan(int dq_r, int dq_w, int din_r, int din_w) {
  MUTEX_LOCK(&mutex_log);
  q_r += dq_r;
  q_w += dq_w;
  in_r += din_r;
  in_w += din_w;
  printf("ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", q_r, q_w, in_r, in_w);
  MUTEX_UNLOCK(&mutex_log);
}

void *czytelnik(void *arg) {
  for (int i = 0; i < LICZBA_POWTORZEN_CZYTELNIKOW; i++) {
    usleep(80000);

    zmien_stan(1, 0, 0, 0); // Rejestracja checi wejscia

    MUTEX_LOCK(&bramka); // Przejscie przez bramke (zapobiega nieskonczonemu
                         // naplywowi czytelnikow)
    MUTEX_LOCK(&mutex_aktywni);

    czytelnicy_w_czytelni++;
    // Pierwszy czytelnik wchodzacy do czytelni blokuje dostep pisarzom
    if (czytelnicy_w_czytelni == 1) {
      MUTEX_LOCK(&mutex_czytelnia);
    }

    MUTEX_UNLOCK(&mutex_aktywni);
    MUTEX_UNLOCK(&bramka); // Zwolnienie bramki dla kolejnych watkow

    zmien_stan(-1, 0, 1, 0);

    // Pobyt w czytelni - czytanie
    usleep(250000);

    MUTEX_LOCK(&mutex_aktywni);
    czytelnicy_w_czytelni--;
    zmien_stan(0, 0, -1, 0);
    // Ostatni wychodzacy czytelnik zwalnia glowna blokade dla pisarzy
    if (czytelnicy_w_czytelni == 0) {
      MUTEX_UNLOCK(&mutex_czytelnia);
    }
    MUTEX_UNLOCK(&mutex_aktywni);
  }
  return NULL;
}

void *pisarz(void *arg) {
  for (int i = 0; i < LICZBA_POWTORZEN_PISARZY; i++) {
    usleep(150000);

    zmien_stan(0, 1, 0, 0); // Rejestracja checi wejscia

    MUTEX_LOCK(
        &bramka); // Pisarz blokuje bramke, ucinajac strumien nowych czytelnikow
    MUTEX_LOCK(&mutex_czytelnia); // Pisarz oczekuje na opuszczenie czytelni
                                  // przez aktualnych czytelnikow
    MUTEX_UNLOCK(&bramka);        // Zwolnienie bramki po zabezpieczeniu wejscia

    zmien_stan(0, -1, 0, 1);

    // Pobyt w czytelni - pisanie
    usleep(500000);

    zmien_stan(0, 0, 0, -1);
    MUTEX_UNLOCK(&mutex_czytelnia); // Zwolnienie wylacznego dostepu do czytelni
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  int czytelnicy_suma = 5;
  int pisarze_suma = 3;

  if (argc == 3) {
    czytelnicy_suma = atoi(argv[1]);
    pisarze_suma = atoi(argv[2]);
    if (czytelnicy_suma <= 0 || pisarze_suma <= 0) {
      fprintf(stderr, "Liczba watkow musi byc wieksza od 0.\n");
      return EXIT_FAILURE;
    }
  }

  pthread_t *czytelnicy = malloc(czytelnicy_suma * sizeof(pthread_t));
  pthread_t *pisarze = malloc(pisarze_suma * sizeof(pthread_t));
  int *id_czytelnikow = malloc(czytelnicy_suma * sizeof(int));
  int *id_pisarzy = malloc(pisarze_suma * sizeof(int));

  if (!czytelnicy || !pisarze || !id_czytelnikow || !id_pisarzy) {
    fprintf(stderr, "FATAL: Brak pamieci.\n");
    exit(EXIT_FAILURE);
  }

  // Inicjalizacja mutexow realizujacych mechanizm synchronizacji
  PTHREAD_CHECK(pthread_mutex_init(&bramka, NULL));
  PTHREAD_CHECK(pthread_mutex_init(&mutex_aktywni, NULL));
  PTHREAD_CHECK(pthread_mutex_init(&mutex_czytelnia, NULL));
  PTHREAD_CHECK(pthread_mutex_init(&mutex_log, NULL));

  for (int i = 0; i < czytelnicy_suma; i++) {
    id_czytelnikow[i] = i + 1;
    PTHREAD_CHECK(
        pthread_create(&czytelnicy[i], NULL, czytelnik, &id_czytelnikow[i]));
  }

  for (int i = 0; i < pisarze_suma; i++) {
    id_pisarzy[i] = i + 1;
    PTHREAD_CHECK(pthread_create(&pisarze[i], NULL, pisarz, &id_pisarzy[i]));
  }

  for (int i = 0; i < czytelnicy_suma; i++) {
    PTHREAD_CHECK(pthread_join(czytelnicy[i], NULL));
  }

  for (int i = 0; i < pisarze_suma; i++) {
    PTHREAD_CHECK(pthread_join(pisarze[i], NULL));
  }

  free(czytelnicy);
  free(pisarze);
  free(id_czytelnikow);
  free(id_pisarzy);

  PTHREAD_CHECK(pthread_mutex_destroy(&bramka));
  PTHREAD_CHECK(pthread_mutex_destroy(&mutex_aktywni));
  PTHREAD_CHECK(pthread_mutex_destroy(&mutex_czytelnia));
  PTHREAD_CHECK(pthread_mutex_destroy(&mutex_log));

  return 0;
}
