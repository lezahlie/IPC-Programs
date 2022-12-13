```
Author: Leslie Horace
Purpose: C programs utilizing interprocess communication
Version: 1.0
```
# IPC Programs

## Included files
- war_pipes.c
    - Implementation of pipes and processes
- war_sockets.c
    - Implementation of sockets and threads
- Makefile 
    - Compiling program(s) and cleaning out files

## Compiling the program(s)
> make all

> make war_pipes

> make war_sockets

## Cleaning compiled files
> make clean

## Running the program(s)
usage: <outfile> <# rounds>

*# rounds argument is the number of rounds to play war*

## About the program(s)
1. war_pipes.c
    - Forks two child processes to play rounds of war
    - Parent process is the game keeper and makes all requests
    - Message passing is implemented with uni-directional pipes
2.  war_sockets.c
    - Creates two child threads to play rounds of war
    - Parent process is the game keeper and makes all requests
    - Message passing is implemented with local host socket
3. War game logic
    - Each round, the parent request each child to draw a card
    - In the case of a tie, the parents request each childs suit
        - If the suits are also tied, the round has no winner
    - After each round, the winning child gets +1 point 
        -  Card scoring values are the same for a typical card game
    - Once all rounds are completed, the results are tallied

