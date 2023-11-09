#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_PLAYERS 20
#define MAX_CARDS 52
#define MAX_CHIPS 100

typedef struct
{
    int cards[MAX_CARDS];
    int top_card;
} Deck;

typedef struct
{
    int id;
    int hand[MAX_CARDS]; // Each player can hold up to MAX_CARDS
    int hand_size;       // Number of cards in hand
    int chips_eaten;
} Player;

typedef struct
{
    Deck deck;
    int greasy_card;
    int chips;
    int current_round;
    int player_count;
    pthread_mutex_t lock;
    pthread_cond_t cond_round;
    pthread_cond_t cond_chip;
} GameState;

GameState game_state;

// Shuffle the deck of cards
void shuffle_deck()
{
    for (int i = 0; i < MAX_CARDS; i++)
    {
        int j = rand() % (i + 1);
        int temp = game_state.deck.cards[i];
        game_state.deck.cards[i] = game_state.deck.cards[j];
        game_state.deck.cards[j] = temp;
    }
    game_state.deck.top_card = 0; // Reset top card after shuffle
}

// Deal a card to a player
void deal_card(Player *player)
{
    if (game_state.deck.top_card < MAX_CARDS)
    {
        player->hand[player->hand_size++] = game_state.deck.cards[game_state.deck.top_card++];
    }
    else
    {
        printf("No more cards in the deck to deal.\n");
    }
}

void initialize_game_state(int player_count, int chips_per_bag, int seed)
{
    srand(seed);
    game_state.player_count = player_count;
    game_state.chips = chips_per_bag;
    game_state.current_round = 0;
    pthread_mutex_init(&game_state.lock, NULL);
    pthread_cond_init(&game_state.cond_round, NULL);
    pthread_cond_init(&game_state.cond_chip, NULL);

    // Initialize deck and shuffle it
    for (int i = 0; i < MAX_CARDS; i++)
    {
        game_state.deck.cards[i] = i + 1; // Assign card values 1-52
    }
    shuffle_deck(); // Shuffle the deck before the game starts
}

void *player_thread(void *arg)
{
    Player *player = (Player *)arg;
    player->hand_size = 0; // Initialize hand size

    while (game_state.current_round < game_state.player_count)
    {
        pthread_mutex_lock(&game_state.lock);

        // Dealer's actions
        if (player->id == game_state.current_round)
        {
            printf("Player %d is the dealer\n", player->id);

            // Deal one card to each player including themselves
            for (int i = 0; i < game_state.player_count; i++)
            {
                deal_card(&game_state.players[i]);
                printf("Player %d dealt card %d to player %d\n", player->id, game_state.players[i].hand[game_state.players[i].hand_size - 1], i);
            }
        }

        pthread_mutex_unlock(&game_state.lock);
        // Signal next player's turn or round end
        pthread_cond_broadcast(&game_state.cond_round);

        // Wait for your turn
        while (player->id != game_state.current_round)
        {
            pthread_cond_wait(&game_state.cond_round, &game_state.lock);
        }

        game_state.current_round = (game_state.current_round + 1) % game_state.player_count;

        // Simulate time taken for actions
        sleep(1);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    // ... (rest of your main function)
}
