#pragma once
#include <pthread.h>
#include "cards.h"

#define SHMEM_SEMAPHORE_NAME "/sop-shmem-sem"
#define SHMEM_NAME "/sop-shmem"

/**
 * Struktura opisujaca zrzutki oraz prymitywy do synchronizacji graczy
 */
typedef struct table
{
    /**
     * Baeriera sluzaca do synchronizacji graczy (np. oczekiwaniu na dolaczenie do stolika, zrzucenie kart, itp)
     */
    pthread_barrier_t players_barrier;
    /**
     * Blokada licznika graczy
     */
    pthread_mutex_t players_counter_lock;
    /**
     * Licznik graczy dolaczajacy do stolika (inicjalizacja przez 0)
     */
    int players_counter;

    /**
     * CV dla informowania pozostalych graczy o zrzuceniu kart
     */
    pthread_cond_t placed_cond;
    /**
     * Blokada uzywania w parze z CV placed_cond
     */
    pthread_mutex_t placed_lock;

    /**
     * miejsce na stole to umiejscowienia zrzutki kazdego z grzaczy (indeksowanie numerem gracza)
     */
    card_t trick[PLAYERS_COUNT];
    /**
     * Potasowane karty do wziecia przez gracza, ktory dolaczyl
     */
    card_t cards[CARDS_COUNT];
} table_t;

/**
 * Inicjalizacja stolika. Pierwszy gracz powinien stworzyc obiekt pamieci dzielonej.
 * Przygotuj wszystkie prymitywy synchronizacji oraz przetasuj karty dla graczy.
 * Zaloz, ze zamkniecie deskryptora dla pamieci dzielonej jest
 * @param player_idx Przez ta zmienna zwroc indeks gracza po dolaczeniu.
 * @return Wskaznik do stalu dla procesu gracza.
 */
table_t* table_init( int* player_idx);

/**
 *  Zamknij obiekt pamieci dzielonej bez niszczenia obiektu.
 * @param shmem
 * @param shmem_fd
 */
void table_close(table_t* shmem);

/**
 * Zamknij obiekt pamieci dzielonej oraz zniszcz go.
 * @param shmem
 * @param shmem_fd
 */
void table_destroy(table_t* shmem);
