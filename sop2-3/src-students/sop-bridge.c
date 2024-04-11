#include <stdio.h>
#include <string.h>

#include "hand.h"
#include "macros.h"
#include "table.h"

/* Use it in qsort() to sort received cards in your hand. */
int compare_cards_sort(const void* lhs, const void* rhs)
{
    return (*(card_t*)lhs > *(card_t*)rhs) - (*(card_t*)lhs < *(card_t*)rhs);
}

void start_game(player_hand_t* hand, table_t* table)
{
    memcpy(hand->hand_cards, table->cards + hand->idx * PLAYER_HAND_COUNT, sizeof(card_t) * PLAYER_HAND_COUNT);
    qsort(hand->hand_cards, PLAYER_HAND_COUNT, sizeof(card_t), compare_cards_sort);
    memset(table->cards + hand->idx * PLAYER_HAND_COUNT, INVALID_CARD, sizeof(card_t) * PLAYER_HAND_COUNT);
}

void play_trick(player_hand_t* hand, table_t* table)
{
    
}

void asses_result(player_hand_t* hand, table_t* table)
{
    
}

int main(int argc, char* argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    srand(13);
    //srand(time(NULL));

    int player_idx;
    table_t* table = table_init(&player_idx);

    // wait for four players
    printf("Waiting for players!\n");
    pthread_barrier_wait(&table->players_barrier);
    printf("Four players ready. Starting the game.\n");

    player_hand_t hand;
    player_hand_init(&hand, player_idx);


    return EXIT_SUCCESS;
}