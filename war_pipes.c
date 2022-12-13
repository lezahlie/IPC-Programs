/*
 * Author: Leslie Horace
 * File: war_pipes.c
 * Purpose: Uses inter-process communication with pipes for two child processes to play a game of war
 * Version 1.0
 */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#define DECK_SIZE 17

// struct for a card
typedef struct Card{
    char * name;
    int points;
}deck, draw; // deck and draws from deck

/*  Selects a random card index, passes back card points, and returns the name */
draw randomCard(deck * cards){
    int index = rand() % 13;
    draw c = cards[index];
    return c;
}

/*  Selects a random suite index, passes back suite points, and returns the name */
draw randomSuite(deck * suites){
    int index = (rand() % 4) + 13;
    draw s = suites[index];
    return s;
}

/*  Child reads each 'next' request from parent in pipe2 and continues until parent ends game, 
    if the request is a card aka 'c', get a random card and write back to parent in pipe1
    otherwise if the request is a suit aka 's', get a random suit and write back to parent in pipe1*/
void playWar(deck * cards, draw card, draw suit, int * fd_1, int * fd_2){
    char next;
    while(read(fd_1[0], &next, sizeof(next)) != -1){
        if(next == 'c'){
            card = randomCard(cards);   
            write(fd_2[1], &card, sizeof(card));
        }
        else if(next == 's'){
            suit = randomSuite(cards);      //  get a random card for child A
            write(fd_2[1], &suit, sizeof(suit));
        }
    }
}

/* Parent writes 'next' request into pipe2, reads the 'draw' from child in pipe1, and returns the draw */ 
draw getDraw(char next, int * fd_1, int * fd_2){
    draw new_draw;
    write(fd_1[1], &next, sizeof(next));
    read(fd_2[0], &new_draw, sizeof(draw));
    return new_draw;
}

/* Determines if child won the round, adds a point to the winners score,
    passes back both scores, and returns a boolean for tie or no tie */
int roundWinner(int pa, int pb, int * sa, int * sb){
    int tied = 1;
    if(pa > pb){
        *sa+=1;
        tied=0;
        printf("\nChild A wins!\n");
    }else if(pa < pb){
        *sb+=1;
        tied=0;
        printf("\nChild B wins!\n");
    }
    return tied;
}

/* Determines the winner of the game with the ending scores */
void gameWinner(int sa, int sb){
    printf("\n===============================================\n");
    printf("\n[Game Results]\n\nChild A score = %d\n\nChild B score = %d\n",sa,sb);
    if(sa > sb){
        printf("\nChild A won the game!\n");
    }else if(sa < sb){
        printf("\nChild B won the game!\n");
    }else{
        printf("\nScores are tied, no winner today kids.\n");
    }
    printf("\n===============================================\n");
}

int main(int argc, char *argv[]) {

    pid_t child_a, child_b; 
    int fd_a2[2], fd_a1[2], fd_b2[2] , fd_b1[2];  
    draw card_a, card_b, suit_a, suit_b;   
    int count=0, rounds = 0, score_a=0, score_b=0;  
    char  *rp, card ='c', suit ='s';

    // a deck of cards and their point values
    deck cards[DECK_SIZE]={
        {"Ace", 13}, {"Two", 1}, {"Three", 2}, {"Four", 3}, {"Five", 4}, {"Six", 5},
        {"Seven", 6},{"Eight", 7}, {"Nine", 8}, {"Ten", 9}, {"Jack", 10}, {"Queen", 11},
        {"King", 12}, {"Clubs", 1}, {"Diamonds", 2}, {"Hearts", 3}, {"Spades", 4}
    };

    // checks if two args are not entered, if so print usage and exit
    if (argc != 2) {    
        printf("Usage: %s num_rounds\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    rounds = strtol(argv[1], &rp, 10);  // checks if round arg is a valid base 10 integer
    // if round entered is not valid, print error and exit
    if(*rp != '\0' || rounds < 0){
        printf("Error: argument num_round = '%s' is not a positive non-zero integer\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    // checks if all 4 pipes can be opened, if not print error and exit
    if(pipe(fd_a1) == -1 || pipe(fd_a2) == -1 || pipe(fd_b1) == -1  || pipe(fd_b2) == -1){
        printf("Error: Cannot create all pipes for reading and writing\n");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));     // seed rand() by the current time
    // fork child A
    switch(child_a = fork()){
        case -1: // child A fork error
            printf("\nError: Cannot fork child A\n");
            exit(EXIT_FAILURE);
        case 0: // child A is executing
            close(fd_a1[1]);    
            close(fd_a2[0]);   
            playWar(cards, card_a, suit_a, fd_a1, fd_a2);   // child A plays game
            child_a = wait(NULL);    // wait for child A to terminate
            break;
    }

    srand(time(NULL)+1);    // seed again since child processes run concurrently
    // fork child B
    switch(child_b = fork()){
        case -1:    // child B fork error
            printf("\nError: Cannot fork child B\n");
            exit(EXIT_FAILURE);
        case 0:     // child B is executing
            close(fd_b1[1]);    
            close(fd_b2[0]);   
            playWar(cards, card_b, suit_b, fd_b1, fd_b2);   // child B plays game
            child_b = wait(NULL);   // wait for child B to terminate
            break;
    }

    // Parent executes here
    close(fd_a1[0]);  
    close(fd_b1[0]);   
    close(fd_a2[1]);   
    close(fd_b2[1]);  

    // display game starting info
    printf("\nChild A [PID] = %d\n\nChild B [PID] = %d\n\nStarting %d rounds...\n\nLet's Go!\n", child_a, child_b, rounds);

    while(count != rounds){  
        // diplay each round and increase count
        printf("\n===============================================\n\n[Round %d]\n", (count++)+1); 
        card_a = getDraw(card, fd_a1, fd_a2);   // get child A card & display draw
        printf("\nChild A draws %s\n", card_a.name);   
        card_b = getDraw(card, fd_b1, fd_b2);   // get child B card & display draw
        printf("\nChild B draws %s\n", card_b.name);

        // check if round was a tie (otherwise winner score increases)
        if(roundWinner(card_a.points, card_b.points, &score_a, &score_b) == 1){
            printf("\nChecking Suit...\n");

            suit_a = getDraw(suit, fd_a1, fd_a2);   // get child A suit and display result
            printf("\nChild A has a %s of %s\n", card_a.name, suit_a.name);

            suit_b = getDraw(suit, fd_b1, fd_b2);   // get child B suit and display result
            printf("\nChild B has a %s of %s\n", card_b.name, suit_b.name);

            // check for tie again (otherwise winner score increases)
            if(roundWinner(suit_a.points, suit_b.points, &score_a, &score_b) == 1){
                printf("\nNo winner, maybe next round?\n");
            }
        }
    }

    gameWinner(score_a, score_b); // determines winner based on final scores
    exit(EXIT_SUCCESS);
}