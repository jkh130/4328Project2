#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

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

int NUM_PLAYERS = MAX_ROUNDS;
int turn_counter = 0;
int round_number = 1;
int dealer_selected;
int dealer;
int total_cards; // New variable to hold the total number of cards

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
    Card *hand[MAX_HAND_SIZE]; // This is now an array of Card pointers
    int chips_eaten;
} Player;

// Function prototypes
void initializeDeck(Card *deck, int total_cards);
void shuffleDeck(Card *deck, int size);
void printDeck(Card *deck, int size);
void dealerWork(int player);
void *player_thread(void *arg);
void takeCardFromTop(Card *deck, int *size);
void initializeHand(Player *player);

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
void initializeHand(Player *player)
{
    for (int i = 0; i < MAX_HAND_SIZE; i++)
    {
        player->hand[i] = NULL; // Initialize the player's hand to NULL
    }
}

void shuffleDeck(Card *deck, int size)
{
    printDeck(deck, size);
    for (int i = size - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        Card temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
    printDeck(deck, size);
}

void printDeck(Card *deck, int size)
{
    for (int i = 0; i < size; i++)
    {
        printf("%s of %s\n", deck[i].value, deck[i].suit);
    }
}

void takeCardFromTop(Card *deck, int *size)
{
    if (*size > 0)
    {
        greasy_card = deck[*size - 1];
        (*size)--;
    }
    else
    {
        printf("The deck is empty, no more cards to take.\n");
    }
}

void dealerWork(int player)
{
    printf("PLAYER %d: shuffling cards \n", player);
    shuffleDeck(deck, total_cards);

    sleep(1);

    takeCardFromTop(deck, &total_cards);
    printf("PLAYER %d: Greasy card is %s of %s\n", player, greasy_card.value, greasy_card.suit);

    sleep(1);

    if (player == 1)
    {
        printf("PLAYER %d: opening first bag of chips \n", player);
        sleep(1);
    }
}

void printPlayerHand(Player *player)
{
    printf("Player %d's hand:\n", player->id);
    for (int i = 0; i < MAX_HAND_SIZE; i++)
    {
        if (player->hand[i] != NULL)
        {
            printf("Card %d: %s of %s\n", i + 1, player->hand[i]->value, player->hand[i]->suit);
        }
        else
        {
            printf("Card %d: [empty]\n", i + 1);
        }
    }
}

void *player_thread(void *arg)
{
    Player *player = (Player *)arg;
    initializeHand(player);
    printPlayerHand(player);

    while (round_number <= MAX_ROUNDS)
    {
        dealer_selected = 0;
        pthread_mutex_lock(&mutexDealer);
        if (player->id == round_number)
        {
            dealer_selected = 1;
            dealer = player->id;

            printf("Player %d is the dealer!\n", dealer);

            dealerWork(dealer);

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

    initializeDeck(deck, total_cards);

    pthread_t threads[NUM_PLAYERS];
    Player players[NUM_PLAYERS];

    for (int i = 0; i < NUM_PLAYERS; i++)
    {
        players[i].id = i + 1;
        if (pthread_create(&threads[i], NULL, player_thread, &players[i]) != 0)
        {
            perror("Failed to create thread");
            free(deck);
            return 1;
        }
    }

    for (int i = 0; i < NUM_PLAYERS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&conditional);

    printf("Game ended after %d rounds.\n", round_number - 1);

    free(deck);
    return 0;
}
