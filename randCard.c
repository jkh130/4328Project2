#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Declare a global variable for the random seed
unsigned int globalRandomSeed = 0;

// Function to initialize the global random seed
void initializeGlobalRandomSeed()
{
    // You can use any integer value as the seed here
    // For example, you can use the current time as the seed
    globalRandomSeed = (unsigned int)time(NULL);
    srand(globalRandomSeed);
}

// Function to generate a random integer between min and max (inclusive)
int generateRandomNumber(int min, int max)
{
    if (min > max)
    {
        fprintf(stderr, "Error: min cannot be greater than max.\n");
        exit(1);
    }
    return min + rand() % (max - min + 1);
}

int main()
{
    // Initialize the global random seed
    initializeGlobalRandomSeed();

    // Generate and print random numbers using the global seed
    for (int i = 0; i < 5; i++)
    {
        int randomNumber = generateRandomNumber(1, 100);
        printf("Random number %d: %d\n", i + 1, randomNumber);
    }

    return 0;
}
