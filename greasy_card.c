#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>

#define MAX_HAND_SIZE 2

// mutex and cond for handling round end
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// barrier for all threads
pthread_barrier_t init_barrier;

int NUM_PLAYERS;
int turn_counter = 0;
int round_number = 1;
int dealer_selected;
int dealer;
int total_cards = 52;  // New variable to hold the total number of cards
int round_winner = -1; // To track the round winner, initialized to -1
int numChips;
int seed;
int chips_max;
int turn = 0;
int temp_roundnumber;

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

Player *players; // Global array of players

// Function prototypes
void initializeDeck(Card *deck, int total_cards);
void shuffleDeck(Card *deck, int size);
void printDeck(Card *deck, int size);
void dealerWork(int dealer_id, Player players[], int num_players);
void *player_thread(void *arg);
Card takeCardFromTop(Card *deck, int *size);
void initializeHand(Player *player);
void addCardToDeckBack(Card *deck, int *total_cards, Card new_card);
void handlePlayerTurn(Player *player);
void handleChips(Player *player);
Card createEmptyCard();

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

void addCardToDeckBack(Card *deck, int *total_cards, Card new_card)
{
    // Check if the deck is full
    if (*total_cards >= 52)
    {
        // Handle the case where the deck is already full
        printf("Deck is full. Cannot add a new card.\n");
        return;
    }

    // Shift all cards to the right by 1 position
    for (int i = *total_cards; i > 0; i--)
    {
        deck[i] = deck[i - 1];
    }

    // Add the new card at the 0th index
    deck[0] = new_card;

    // Increase the size of the deck
    *total_cards += 1;
}

void dealerWork(int dealer_id, Player players[], int num_players)
{
    // (1) Shuffle deck of cards
    printf("PLAYER %d: shuffling cards \n", dealer_id);
    shuffleDeck(deck, total_cards);

    // (2) Choose and display greasy card
    greasy_card = takeCardFromTop(deck, &total_cards);
    printf("PLAYER %d: Greasy card is %s of %s\n", dealer_id, greasy_card.value, greasy_card.suit);

    // (3) Deal a single card to each player and display player ID
    for (int i = 0; i < num_players; i++)
    {
        // printf("I can see Player %d's hand %s of %s and %s of %s\n", players[i].id, players[i].hand[0].value, players[i].hand[0].suit, players[i].hand[1].value, players[i].hand[1].suit);

        for (int j = 0; j < MAX_HAND_SIZE; j++)
        {
            if (strcmp(players[i].hand[j].suit, "Empty") == 0 && players[i].id != dealer_id)
            {
                printf("PLAYER %d: deals a card to PLAYER %d\n", dealer_id, players[i].id);
                players[i].hand[j] = takeCardFromTop(deck, &total_cards);
                j = MAX_HAND_SIZE;
            }
        }
    }

    // (4) Open first bag of chips
    if (dealer_id == 1)
    {
        printf("PLAYER %d: opening first bag of chips \n", dealer_id);
        numChips = chips_max;
        printf("BAG: %d chips left\n", numChips);
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
    int randomIndex = rand() % MAX_HAND_SIZE;

    if (round_winner == -1)
    {

        for (int j = 0; j < MAX_HAND_SIZE; j++)
        {
            if (strcmp(player->hand[j].suit, "Empty") == 0)
            {

                player->hand[j] = takeCardFromTop(deck, &total_cards);
                printf("PLAYER %d: draws a %s of %s\n", player->id, player->hand[j].value, player->hand[j].suit);
                j = MAX_HAND_SIZE;
            }
        }
        // Check for matching card
        int hasMatchingCard = 0;

        for (int i = 0; i < MAX_HAND_SIZE; i++)
        {
            // Check if the card in the player's hand matches the number value of the greasy card
            if (strcmp(player->hand[i].value, greasy_card.value) == 0)
            {
                hasMatchingCard = 1;
                round_winner = player->id;
                break;
            }
            // Check if the greasy card is "Jack," "Queen," "King," or "Ace" and the player has a matching card
            if (strcmp(greasy_card.value, "Jack") == 0 || strcmp(greasy_card.value, "Queen") == 0 ||
                strcmp(greasy_card.value, "King") == 0 || strcmp(greasy_card.value, "Ace") == 0)
            {
                if (strcmp(player->hand[i].value, greasy_card.value) == 0)
                {
                    hasMatchingCard = 1;
                    round_winner = player->id;
                    break;
                }
            }
        }
        // Get rid of random card
        if (hasMatchingCard == 0)
        {

            printf("PLAYER %d: discards %s of %s at random\n", player->id, player->hand[randomIndex].value, player->hand[randomIndex].suit);
            // add card to back of deck and make player hand empty
            addCardToDeckBack(deck, &total_cards, player->hand[randomIndex]);
            player->hand[randomIndex] = createEmptyCard();
        }
    }
}

void handleChips(Player *player)
{
    int randChips = (rand() % 5) + 1;

    if (numChips <= randChips)
    {
        printf("PLAYER %d: Eats %d chips\n", player->id, numChips);

        randChips = randChips - numChips;

        printf("PLAYER %d: Opens a new bag of chips\n", player->id);
        numChips = 20;
        if (randChips != 0)
        {

            printf("PLAYER %d: Eats %d chips\n", player->id, (randChips));
            numChips = numChips - randChips;
        }

        printf("BAG: %d Chips left\n", (numChips));
    }
    else
    {

        printf("PLAYER %d: Eats %d chips\n", player->id, randChips);
        numChips = numChips - randChips;
        printf("BAG: %d Chips left\n", numChips);
    }
}

void *player_thread(void *arg)
{
    Player *player = (Player *)arg;
    srand(seed * (player->id));

    // Initialize the hand of the player
    initializeHand(player);
    // Wait at the barrier for all other threads
    pthread_barrier_wait(&init_barrier);

    while (round_number <= NUM_PLAYERS)
    {
        dealer_selected = 0;
        pthread_mutex_lock(&mutex);
        if (player->id == round_number)
        {
            dealer_selected = 1;
            dealer = player->id;

            printf("\nPlayer %d is the dealer!\n", dealer);

            dealerWork(player->id, players, NUM_PLAYERS);

            dealer_selected = 1;
            pthread_mutex_unlock(&mutex);
            pthread_cond_broadcast(&cond);
        }
        else
        {
            while (!dealer_selected)
            {
                pthread_cond_wait(&cond, &mutex);
            }
            pthread_mutex_unlock(&mutex);
        }

        // Handle the player's turn
        // Dealer does not participate in round

        do
        {
            pthread_mutex_lock(&mutex);
            if ((player->id == (((round_number + turn) % NUM_PLAYERS)) + 1) && player->id != round_number /*ensure dealer does not increment turn*/)
            {
                // printf("handle player turn\n");
                handlePlayerTurn(player);
                if (player->id == round_winner)
                {
                    printf("PLAYER %d: hand (%s of %s, %s of %s) <> Greasy card is %s of %s\n", player->id, player->hand[0].value, player->hand[0].suit, player->hand[1].value, player->hand[1].suit, greasy_card.value, greasy_card.suit);
                    printf("PLAYER %d: wins round %d\n", player->id, round_number);
                }

                turn++;
            }

            pthread_mutex_unlock(&mutex);

        } while (round_winner == -1 && turn != NUM_PLAYERS - 1);

        pthread_barrier_wait(&init_barrier);
        if (player->id != round_number)
        {
            if (player->id != round_winner)
            {
                printf("PLAYER %d: lost round %d\n", player->id, round_number);
            }
        }

        pthread_barrier_wait(&init_barrier);

        if (player->id == round_number)
        {
            printf("PLAYER %d: Placing greasy in the back of the deck\n", player->id);
            addCardToDeckBack(deck, &total_cards, greasy_card);
        }

        // Wait for all threads to complete

        pthread_barrier_wait(&init_barrier);

        pthread_mutex_lock(&mutex);

        turn_counter++;

        if (turn_counter == NUM_PLAYERS)
        {
            printf("PLAYER %d: Round ends\n", dealer);
            round_winner = -1;
            temp_roundnumber = round_number;
            round_number++;
            turn_counter = 0;
            turn = 0;

            pthread_cond_broadcast(&cond);
            pthread_mutex_unlock(&mutex);
        }
        else
        {

            do
            {
                pthread_cond_wait(&cond, &mutex);
            } while (turn_counter != 0);
            pthread_mutex_unlock(&mutex);
        }

        // eat chips
        pthread_mutex_lock(&mutex);
        // Dealer does not eat chips
        do
        {

            if ((player->id == (((temp_roundnumber + turn) % NUM_PLAYERS)) + 1) && player->id != temp_roundnumber)
            {
                handleChips(player);
                turn++;
            }

            pthread_mutex_unlock(&mutex);
            pthread_barrier_wait(&init_barrier);
        } while (turn != NUM_PLAYERS - 1);

        // Wait for all threads to complete
        pthread_barrier_wait(&init_barrier);
    }

    if (player->id = round_number)
    {
        turn = 0;
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 4) // Expecting exactly 3 additional arguments: seed, num_players, num_chips
    {
        fprintf(stderr, "Usage: %s <seed> <num_players> <num_chips>\n", argv[0]);
        return 1;
    }

    // Convert arguments to integers
    seed = atoi(argv[1]);

    NUM_PLAYERS = atoi(argv[2]);
    players = (Player *)malloc(NUM_PLAYERS * sizeof(Player));
    if (players == NULL)
    {
        perror("Failed to allocate memory for players");
        free(deck);
        return 1;
    }
    chips_max = atoi(argv[3]);

    deck = (Card *)malloc(sizeof(Card) * total_cards);

    if (deck == NULL)
    {
        perror("Failed to allocate memory for deck");
        return 1;
    }

    initializeDeck(deck, total_cards);

    pthread_t threads[NUM_PLAYERS];

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

    printf("\nGame ended after %d rounds.\n", round_number - 1);
    return 0;
}
