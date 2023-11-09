#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MAX_ROUNDS 6
#define MAX_HAND_SIZE 2

// mutex and conditional for handling round end
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t conditional = PTHREAD_COND_INITIALIZER;

// mutex and conditional for handling who the dealer is
pthread_mutex_t mutexDealer = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condDealer = PTHREAD_COND_INITIALIZER;

// mutex and conditional for handling player turns
pthread_mutex_t mutexPlayerTurn = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condPlayerTurn = PTHREAD_COND_INITIALIZER;

// barrier for all threads
pthread_barrier_t init_barrier;

int NUM_PLAYERS = MAX_ROUNDS;
int turn_counter = 0;
int round_number = 1;
int dealer_selected;
int dealer;
int total_cards;       // New variable to hold the total number of cards
int round_winner = -1; // To track the round winner, initialized to -1

typedef struct
{
    const char *suit;
    const char *value;
} Card;

Card *deck = NULL; // Change the deck to a pointer for dynamic allocation
Card greasy_card;

typedef struct
{
    int id;
    Card hand[MAX_HAND_SIZE]; // This is now an array of Card objects
    int chips_eaten;
} Player;

Player players[MAX_ROUNDS]; // Global array of players

// Function prototypes
void initializeDeck(Card *deck, int total_cards);
void shuffleDeck(Card *deck, int size);
void printDeck(Card *deck, int size);
void dealerWork(int dealer_id, Player players[], int num_players);
void *player_thread(void *arg);
Card takeCardFromTop(Card *deck, int *size);
void initializeHand(Player *player);

Card createEmptyCard()
{
    Card empty_card;
    empty_card.suit = "Empty";
    empty_card.value = "Empty";
    return empty_card;
}

void initializeHand(Player *player)
{
    for (int i = 0; i < MAX_HAND_SIZE; i++)
    {
        player->hand[i] = createEmptyCard();
    }
}

void initializeDeck(Card *deck, int total_cards)
{
    const char *suits[] = {"Hearts", "Diamonds", "Clubs", "Spades"};
    const char *values[] = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "Jack", "Queen", "King", "Ace"};

    for (int i = 0; i < total_cards; i++)
    {
        deck[i].suit = suits[i / (total_cards / 4) % 4];
        deck[i].value = values[i % 13];
    }
}

void shuffleDeck(Card *deck, int size)
{
    for (int i = size - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        Card temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
}

void printDeck(Card *deck, int size)
{
    for (int i = 0; i < size; i++)
    {
        printf("%s of %s\n", deck[i].value, deck[i].suit);
    }
}

Card takeCardFromTop(Card *deck, int *size)
{
    Card card;
    if (*size > 0)
    {
        card = deck[*size - 1];
        (*size)--;
    }
    else
    {
        // Return an empty card when the deck is empty
        card = createEmptyCard();
    }
    return card;
}

void dealerWork(int dealer_id, Player players[], int num_players)
{
    // (1) Shuffle deck of cards
    printf("PLAYER %d: shuffling cards \n", dealer_id);
    shuffleDeck(deck, total_cards);

    sleep(1);

    // (2) Choose and display greasy card
    greasy_card = takeCardFromTop(deck, &total_cards);
    printf("PLAYER %d: Greasy card is %s of %s\n", dealer_id, greasy_card.value, greasy_card.suit);

    sleep(1);

    // (3) Deal a single card to each player and display player ID
    for (int i = 0; i < num_players; i++)
    {
        printf("I can see Player %d's hand %s of %s and %s of %s\n", players[i].id, players[i].hand[0].value, players[i].hand[0].suit, players[i].hand[1].value, players[i].hand[1].suit);

        for (int j = 0; j < MAX_HAND_SIZE; j++)
        {
            if (strcmp(players[i].hand[j].suit, "Empty") == 0)
            {
                players[i].hand[j] = takeCardFromTop(deck, &total_cards);
            }
        }
    }

    // (4) Open first bag of chips
    if (dealer_id == 1)
    {
        printf("PLAYER %d: opening first bag of chips \n", dealer_id);
        sleep(1);
    }
}

void printPlayerHand(Player *player)
{
    printf("Player %d's hand:\n", player->id);
    for (int i = 0; i < MAX_HAND_SIZE; i++)
    {
        if (strcmp(player->hand[i].suit, "Empty") == 0)
        {
            printf("Card %d: [empty]\n", i + 1);
        }
        else
        {
            printf("Card %d: %s of %s\n", i + 1, player->hand[i].value, player->hand[i].suit);
        }
    }
}

void handlePlayerTurn(Player *player)
{

    // Check if the player has a card matching greasy_card
    int hasMatchingCard = 0;
    for (int i = 0; i < MAX_HAND_SIZE; i++)
    {
        if (strcmp(player->hand[i].suit, greasy_card.suit) == 0 &&
            strcmp(player->hand[i].value, greasy_card.value) == 0)
        {
            hasMatchingCard = 1;
            break;
        }
    }

    // If they have a matching card, declare themselves as the winner
    if (hasMatchingCard)
    {
        printf("Player %d wins the round!\n", player->id);
        round_winner = player->id;

        // Set other players as losers
        for (int i = 0; i < NUM_PLAYERS; i++)
        {
            if (players[i].id != player->id)
            {
                printf("Player %d loses the round.\n", players[i].id);
            }
        }
    }
    else
    {
        // If they don't have a matching card, set one random card to "Empty"
        int randomIndex = rand() % MAX_HAND_SIZE;
        printf("%d\n", randomIndex);
        player->hand[randomIndex] = createEmptyCard();
    }
}

void *player_thread(void *arg)
{
    Player *player = (Player *)arg;

    // Initialize the hand of the player
    initializeHand(player);
    // Wait at the barrier for all other threads
    pthread_barrier_wait(&init_barrier);

    while (round_number <= MAX_ROUNDS)
    {
        dealer_selected = 0;
        pthread_mutex_lock(&mutexDealer);
        if (player->id == round_number)
        {
            dealer_selected = 1;
            dealer = player->id;

            printf("Player %d is the dealer!\n", dealer);

            dealerWork(player->id, players, NUM_PLAYERS);

            dealer_selected = 1;
            pthread_mutex_unlock(&mutexDealer);
            pthread_cond_broadcast(&condDealer);
        }
        else
        {
            while (!dealer_selected)
            {
                pthread_cond_wait(&condDealer, &mutexDealer);
            }
            pthread_mutex_unlock(&mutexDealer);
        }

        pthread_mutex_lock(&mutexPlayerTurn);

        // Handle the player's turn
        handlePlayerTurn(player);

        // Signal that the player's turn is complete
        pthread_cond_signal(&condPlayerTurn);
        pthread_mutex_unlock(&mutexPlayerTurn);

        sleep(1);

        pthread_mutex_lock(&mutex);
        turn_counter++;

        if (turn_counter == NUM_PLAYERS)
        {
            printf("PLAYER %d: Round ends.\n\n", dealer);
            round_number++;
            turn_counter = 0;
            pthread_cond_broadcast(&conditional);
            pthread_mutex_unlock(&mutex);
        }
        else
        {
            do
            {
                pthread_cond_wait(&conditional, &mutex);
            } while (turn_counter != 0);
            pthread_mutex_unlock(&mutex);
        }
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 2 || (atoi(argv[1]) % 52 != 0))
    {
        fprintf(stderr, "Usage: %s <total_cards>\n", argv[0]);
        fprintf(stderr, "The number of cards must be a multiple of 52.\n");
        return 1;
    }

    total_cards = atoi(argv[1]);
    deck = (Card *)malloc(sizeof(Card) * total_cards);

    if (deck == NULL)
    {
        perror("Failed to allocate memory for deck");
        return 1;
    }

    srand(time(NULL));
    initializeDeck(deck, total_cards);

    pthread_t threads[MAX_ROUNDS];

    // Initialize the barrier for the number of players
    pthread_barrier_init(&init_barrier, NULL, NUM_PLAYERS);

    // Initialize and create threads for all players
    for (int i = 0; i < NUM_PLAYERS; i++)
    {
        players[i].id = i + 1;
        initializeHand(&players[i]); // Initialize the hand of each player
        if (pthread_create(&threads[i], NULL, player_thread, &players[i]) != 0)
        {
            perror("Failed to create thread");
            free(deck);
            return 1;
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < NUM_PLAYERS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // Clean up
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&conditional);
    pthread_mutex_destroy(&mutexDealer);
    pthread_cond_destroy(&condDealer);
    pthread_barrier_destroy(&init_barrier);

    printf("Game ended after %d rounds.\n", round_number - 1);

    free(deck);
    return 0;
}
