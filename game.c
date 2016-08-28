#include "game.h"



/*
 * Create a new player with the given name.  Insert it at the head of the list 
 * of players whose head is pointed to by *head_player_ptr_ptr.
 *
 * Return:
 *   - 0 on success.
 *   - 1 if a player by this name already exists in this list.
 *   - 2 if the given name cannot fit in the 'name' array
 *       (don't forget about the null terminator).
 */
int add_player(const char *name, Player **head_player_ptr_ptr) {
    if (strlen(name) >= MAX_NAME) {
        return 2;
    }
    if(search_player(name, *head_player_ptr_ptr) == 1){
        return 1;
    }
    else{
        Player *new_player = malloc(sizeof(Player));
        if (new_player == NULL) {
            perror("malloc");
            exit(1);
        }
        strncpy(new_player->name, name, MAX_NAME); // name has max length MAX_NAME - 1
        new_player->total_games =0;
        new_player->total_score =0;
        new_player->max_score =0;
        new_player->next = NULL;  
        if(*head_player_ptr_ptr == NULL){
            *head_player_ptr_ptr = new_player;
            new_player->next = NULL;
        }else{
            new_player->next = *head_player_ptr_ptr;
            *head_player_ptr_ptr = new_player;
        }
        return 0;
    }
}

/* 
 * Return 1 if the player with this name in
 * the list. Return 0 if no such player exists.
 */
int search_player(const char *name, Player *head){
     Player *current = head;
     while (current != NULL){
        if(strcmp(current->name, name) == 0){
            return 1;
        }
        current = current->next;
     }
     return 0;
}

/* 
 * Return a pointer to the player with this name in
 * the list starting with head. Return NULL if no such player exists.
 *
 * NOTE: You'll likely need to cast a (const Player *) to a (Player *)
 * to satisfy the prototype without warnings.
 */
Player *find_player(const char *name, const Player *head) {

    while (head != NULL && strcmp(name, head->name) != 0) {
        head = head->next;
    }

    return (Player *)head;
}


/*
 * Print the playernames of all players in the list starting at curr.
 * Names should be printed to standard output, one per line.
 */
void list_players(const Player *curr) {
    printf("Player List\n");
    while (curr != NULL) {
        printf("\t%s\n",curr->name);
        curr = curr->next;
    }
}



/* 
 * Print player stats * 
 *   - 0 on success.
 *   - 1 if the player is NULL.
 */
int print_player ( Player *p) {
    if (p == NULL) {
        return 1;
    }

    // Print name
    printf("Name: %s\n\n", p->name);
    printf("------------------------------------------\n");

    // Print player stats.
    printf("total games:%d, total points:%d best score:%d\n", p->total_games, p->total_score, p->max_score );
   
    printf("------------------------------------------\n");
   

    return 0;
}

/*
 * Finds the player and updates player score
 *
 * Return:
 *   - 0 on success
 *   - 1 if player is not in the list
 */
int add_score(char *name, int score, const Player *player_list){
    Player *player = find_player (name, player_list );
    if (player == NULL){
        return 1;
    } 
    player->total_games ++;
    player->total_score += score;
    if (player->max_score < score)
        player->max_score = score;
    return 0;
}

/*
 *generates a new game board abd prints it
 */
void print_board (){
    //static gameboard to be returned - to be replaced by randomly generated boards
    int i=0, j=0;
    char board[4][4] = {{'a','c','s','m'},{'b','r','a','n'},{'d','y','s','e'},{'n','t','r','a'}};
    for (i=0; i<4; i++){
        for ( j=0; j<4; j++)
            printf("%c ",board[i][j]);
        printf ("\n");
    }
}

/*
 * computes the length of the list
 */
int length(Player *head) {
    Player *cur = head;
    int counter = 0;
    while (cur) {
        cur = cur->next;
        counter++;
    }
    return counter;
}