/*
 * Author: Leslie Horace
 * File: war_sockets.c
 * Purpose: Uses inter-process communication with sockets for two child threads to play a game of war
 * Version 1.0
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/syscall.h>
#define DECK_SIZE 17
#define SOCKET_PATH "Game_socket"

// struct for a card
typedef struct Card{
    char * name;
    int points;
}deck, draw; // deck and draws from deck

/*  Selects a random card/suit index, passes back card/suit points, and returns the name */
draw randomDraw(char next, deck * cards){
    int index;
    if(next == 'c'){    
        index = rand() % 13;
    }else if(next == 's'){
        index = (rand() % 4) + 13;
    }
    return cards[index];
}

/* Parent writes 'next' request into pipe2, reads the 'draw' from child in pipe1, and returns the draw */ 
draw getDraw(char next, int child){
    draw new_draw;
    write(child, &next, sizeof(next));
    read(child, &new_draw, sizeof(draw));
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
    printf("\n[Game Results]\n\nchild A score = %d\n\nchild B score = %d\n",sa,sb);
    if(sa > sb){
        printf("\nChild A won the game!\n");
    }else if(sa < sb){
        printf("\nChild B won the game!\n");
    }else{
        printf("\nScores are tied, no winner today kids.\n");
    }
    printf("\n===============================================\n");
}

/* Handles errors pertaining to scokets and threads, make exit() optional */
void handleError(char * msg, int stop){
    perror(msg);
    if(stop == 1){
        exit(EXIT_FAILURE); 
    }
}

/* Child thread start function to play the game */
void * playGame(){
    int sockfd, conn_check; 
    draw draw;    
    char next;  

    // a deck of cards and their point values
    deck cards[DECK_SIZE]={
        {"Ace", 13}, {"Two", 1}, {"Three", 2}, {"Four", 3}, {"Five", 4}, {"Six", 5},
        {"Seven", 6},{"Eight", 7}, {"Nine", 8}, {"Ten", 9}, {"Jack", 10}, {"Queen", 11},
        {"King", 12}, {"Clubs", 1}, {"Diamonds", 2}, {"Hearts", 3}, {"Spades", 4}
    };

    // create socket and check for error
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if( sockfd < 0){
        handleError("playGame(): child socket error...", 1);
    }

    // set up local sockaddr 
    struct sockaddr_un svr_addr;
    svr_addr.sun_family = AF_UNIX;
    strcpy(svr_addr.sun_path, SOCKET_PATH);

    // try connect to server 5 times, break on success
    for(int i=0; i<5; i++){
        usleep(1000); // sleep here, for child thread to catch up
        conn_check = connect(sockfd, (struct sockaddr *) &svr_addr, sizeof(svr_addr));
        if(conn_check == 0){
            break;
        }
        handleError("playGame(): child connect failed, trying again...", 0);
        
    }

    // check child connection to socket
    if(conn_check == -1){
        handleError("playGame(): unable to connect child to socket...", 1);
    }

    // get the childs thread id
    pid_t tid = syscall(SYS_gettid);
    void * tid_ptr = &tid;  
    write(sockfd,&tid_ptr, sizeof(tid_ptr));  

    // wait for parent to write a message, loops until parent stops
    while(read(sockfd, &next, sizeof(next)) != -1){
        draw = randomDraw(next, cards);     
        write(sockfd, &draw, sizeof(draw));   
    }

    close(sockfd);  //close socket file desc

    exit(EXIT_SUCCESS); // exit child thread start function
}

/* Parent sets up socket and manages the game */ 
void manageGame(int rounds, pthread_t ch_a, pthread_t ch_b){

    char next_c = 'c', next_s = 's';    
    int sockfd, conn_a, conn_b;     
    int count = 0, score_a = 0, score_b = 0;    
    draw card_a, card_b, suit_a, suit_b;  
    void *tid_a, *tid_b;   
    
    // create socket file desc
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(sockfd < 0){
        handleError("gameKeeper(): socket error...", 1);
    }

    // set up sockaddrr
    struct sockaddr_un svr_addr;
    svr_addr.sun_family = AF_UNIX;
    strcpy(svr_addr.sun_path, SOCKET_PATH);

    // initialize the socket file
    unlink(SOCKET_PATH);

    // bind to server socket
    if(bind(sockfd, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) < 0){
        handleError("gameKeeper(): bind error...", 1);
    }

    // listen for 2 connections
    if(listen(sockfd, 2) < 0){
        handleError("gameKeeper(): listen error...", 1);
    }

    // accept connection for child_a
    conn_a = accept(sockfd, NULL, NULL);
    if(conn_a < -1){
        handleError("gameKeeper(): accept error for Child A...", 1);
    }   

    // accept connection for child_b
    conn_b = accept(sockfd, NULL, NULL);
    if(conn_b < -1){
        handleError("gameKeeper(): accept error for Child B...", 1);
    }   

    // read thread id's from both child threads
    read(conn_a, &tid_a, sizeof(tid_a));
    read(conn_b, &tid_b, sizeof(tid_a));

    printf("\n===============================================\n");  // display game starting info
    printf("\nChild A [TID] = %d\n\nChild B [TID] = %d\n\nStarting %d rounds...\n\nLet's Go!\n", *(int *)tid_a, *(int *)tid_b, rounds);

    // loop until all rounds are played
    while(count != rounds){
        printf("\n===============================================\n\n[Round %d]\n", (count++)+1); 
        card_a = getDraw(next_c, conn_a);       // get child A card and display result
        printf("\nChild A draws %s\n", card_a.name);
        card_b = getDraw(next_c, conn_b);       // get child B card and display result
        printf("\nChild B draws %s\n", card_b.name);

        // check for a tie (otherwise winner score increases)
        if(roundWinner(card_a.points, card_b.points, &score_a, &score_b) == 1){
            printf("\nChecking Suit...\n");
            suit_a = getDraw(next_s, conn_a);       // get child A suit and display result
            printf("\nChild A has a %s of %s\n", card_a.name, suit_a.name);
            suit_b = getDraw(next_s, conn_b);       // get child B suit and display result
            printf("\nChild B has a %s of %s\n", card_b.name, suit_b.name);

            // check for tie again (otherwise winner score increases)
            if(roundWinner(suit_a.points, suit_b.points, &score_a, &score_b) == 1){
                printf("\nNo winner, maybe next round?\n");
            }
        }
    }

    // cancel thread for child A 
    if(pthread_cancel(ch_a) != 0){
        handleError("gameKeeper(): pthread_cancel error for child_a", 1);
    }

    // cancel thread for child B
    if(pthread_cancel(ch_b) != 0){
        handleError("gameKeeper(): pthread_cancel error for child_b", 1);
    }

    // determines winner based on final scores
    gameWinner(score_a, score_b); 

    // close the socket file desc
    close(sockfd);  

    // remove socket file since we are done
    unlink(SOCKET_PATH);
}


int main(int argc, char *argv[]) {

    pthread_t child_a, child_b;
    int rounds = 0;  
    char *rp;   

    if (argc != 2) {    
        printf("Usage: %s num_rounds\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    rounds = strtol(argv[1], &rp, 10);  // checks if round arg is a valid base 10 integer
    // if round entered is not valid, print error and exit
    if(*rp != '\0' || rounds < 0){
        printf("main(): argument num_round = '%s' is not a positive non-zero integer\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));  // seed the time for getting reandom cards/suites
    
    //  create the first child thread to play game and check for error
    if (pthread_create(&child_a, NULL, playGame, NULL) != 0) {
        handleError("main(): pthread_create failed for child A...", 1);
    }

    //  create the second child thread to play game and check for error
    if (pthread_create(&child_b, NULL, playGame, NULL) != 0) {
        handleError("main(): pthread_create failed for child B...", 1);
    }

    // parent creates socket and manages game
    manageGame(rounds, child_a, child_b);   

    // wait for child A thread to terminate, check for error
    if(pthread_join(child_a, NULL) != 0){
        handleError("main(): pthread_join error for child_a...", 1);;
    }

    // wait for child B thread to terminate, check for error
    if(pthread_join(child_b, NULL) != 0){
        handleError("main(): pthread_join error for child_b...", 1);
    }

    // exit with success
    exit(EXIT_SUCCESS); 
}