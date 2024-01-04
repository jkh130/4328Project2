**Synchronized Card Game Simulator**

**Project Overview**
This project implements a card game with a focus on parallel implementation and ensuring mutual exclusion between threads while actions are performed in parallel.

**Game Rules**

In each round, the dealer shuffles the deck, selects a "Greasy Card" deals cards to each player, opens a bag of potato chips, and waits for the round to end.
Players draw cards in a round-robin fashion, comparing their hand to the Greasy Card. If there's no match, they discard a card randomly. If there's a match, the player declares victory, and the round ends.
After playing, players eat a random number of chips between [1 and 5] before the next turn. If a player runs out of chips, they open a new bag.

**Implementation**

Program is implemented in C using POSIX threads to create synchronization among players.
The main function creates n threads (one for each player) and ensures proper syncrhonization to protect shared objects.
The program takes three arguments: a seed for random number generation, the number of players, and the number of potato chips in a bag. 


