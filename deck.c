#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Define a structure to represent a card
typedef struct
{
    char *suit;
    char *value;
} Card;

// Initialize the deck with 52 cards
void initializeDeck(Card *deck)
{
    char *suits[] = {"Hearts", "Diamonds", "Clubs", "Spades"};
    char *values[] = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "Jack", "Queen", "King", "Ace"};

    for (int i = 0; i < 52; i++)
    {
        deck[i].suit = suits[i / 13];
        deck[i].value = values[i % 13];
    }
}

// Shuffle the deck of cards
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

// Function to print the deck
void printDeck(Card *deck, int size)
{
    for (int i = 0; i < size; i++)
    {
        printf("%s of %s\n", deck[i].value, deck[i].suit);
    }
}

int main()
{
    // Seed the random number generator
    srand(time(NULL));

    Card deck[52];

    initializeDeck(deck);
    shuffleDeck(deck, 52);
    printDeck(deck, 52);

    return 0;
}
