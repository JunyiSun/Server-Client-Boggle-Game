#define _XOPEN_SOURCE
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>   
#include "game.h"
#include "read_line.h"
#include "board_generator.h"
#include "dictionary.h"
#include "construct_word_dict.h"
#include "word_checker.h"

#ifndef PORT
  #define PORT 53515
#endif

#define QUEUE_LENGTH 5
#define BUF_LENGTH 1000
#define INTERVAL 120000        /* time of one game session in millisecon */

struct active_client {

	int fd;
	// username can be at most MAX_NAME - 1 characters long (null terminator at end)
	char name[MAX_NAME];
	struct in_addr ipaddr;
	struct active_client *next;
	// record the last command of the client to satisfy 1.8 client-server interaction
	int played; // 1 denotes issued new_game, 0 denotes not
	int submitted_score; // 1 denotes submitted, 0 denotes not
    int in_game;    // 1 denotes waiting for input word, 0 denotes not 
    int current_game_score;
    DNode **appeared_words;

} *top_client = NULL;

struct player* top_player = NULL;

char login_info[] = "Please input your username:\r\n"
    	"Your username must be less than %d characters without spaces.\r\n"
    	"If exceeds, it will be truncated.\r\n"
    	"If no input is given by the end of this game session, you will be disconnected.\r\n"
    	"If you give an empty name, your name will be unknown.\r\n";

char overflow_note[] = "Too much information. Experiencing buffer overflow. \r\n"
							   "Please update the capacity of buffer.\r\n";

char invalid_syntax[] = "Incorrect syntax\r\n";

char not_played[] = "You can't add a score, since you haven't played a new game yet!\r\n";

char not_submit[] = "You can't start a new game yet, since you haven't submitted your score!\r\n";

char board_changed[] = "Current game is over\r\n" 
"Do you want to: \r\n"
"1. start a new game? [new_game]\r\n"
"2. view all players? [all_players]\r\n"
"3. see top scores? [top_3]\r\n"
"4. quit? [quit]\r\n";

static int listenfd;
static void bindandlisten();
static void newconnection();
static void process_client(struct active_client *p, char **board, DNode **dictionary);
static struct active_client* addclient(int fd, struct in_addr addr);
static void removeclient(int fd);
static void notify(struct active_client *p, char *s, int size);
static void record_username_for_client(struct active_client *new_p, char *buf);	

void all_players(struct active_client *p);
void top_3(struct active_client *p);
void new_game(struct active_client *p, char **board);
void quit(struct active_client *p);
void update_player(struct active_client *p);
void overflow_notify(struct active_client *p);
void add_info_to_buf(struct player *cur, char **buf_ptr, char *buf);
void sig_handler(int signo);		
void set_alarm();
void valid_word_operate(struct active_client *p, char *word, DNode **appeared_words);
void invalid_word_operate(struct active_client *p, char *word, DNode **appeared_words, DNode **dictionary);
char **board;
struct active_client *p;
time_t starttime;


int main(int argc, char **argv) {
	DNode **dictionary = construct_dictionary_with_path("./dictionary.txt");
   
   board = construct_board();
   
   set_alarm();
   
   sigset_t signal_set;
   sigemptyset(&signal_set);
   sigaddset(&signal_set, SIGALRM);

   struct timeval timeout;
	timeout.tv_sec = 1;      
	timeout.tv_usec = 0;
   
	
	//struct active_client *p;   //make it global so that signal handler can access
	bindandlisten();
	while (1) {
		fd_set fdlist;
		// record the value of maximum file descriptor for sake of first arg of select()
		int maxfd = listenfd;
		

		FD_ZERO(&fdlist);
		FD_SET(listenfd, &fdlist);
        

		for (p = top_client; p; p = p->next) {
	    	FD_SET(p->fd, &fdlist);
	    	if (p->fd > maxfd)
			maxfd = p->fd;
		}
	
	    sigprocmask(SIG_BLOCK, &signal_set, NULL);
		
        int select_result = select(maxfd + 1, &fdlist, NULL, NULL, &timeout);

	    sigprocmask(SIG_UNBLOCK, &signal_set, NULL); 
		
		if (select_result < 0) {
	    	perror("select");
	    	exit(1);
		} 
		else if (select_result == 0){
			//printf("timeout\n");
			//break;
		}
		else {

	    	for (p = top_client; p; p = p->next)
			if (FD_ISSET(p->fd, &fdlist))
		    	break;
	    	if (p) {
	    		// p has something to read, since its file descriptor is in bit set after select
	    		process_client(p, board, dictionary);
	    		printf("out of process_client\n");
	    	}
	    	if (FD_ISSET(listenfd, &fdlist)){
				newconnection();
			    printf("out of new_connection\n");
			}
		}
		
	}
    free_board(board);
	return 0;
}

static void bindandlisten() {
	/* This part of code is adapted from muffinman, since
	 * the technical details are similar
	 */
	struct sockaddr_in r;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
    }

    // Make sure we can reuse the port immediately after the server terminates.
    // This part is different from muffinman.
    int on = 1;
    int status = setsockopt(listenfd, SOL_SOCKET, 
    	SO_REUSEADDR, (const char *) &on, sizeof(on));

  	if (status == -1) {
    	perror("setsockopt -- REUSEADDR");
  	}

    memset(&r, '\0', sizeof r);
    r.sin_family = AF_INET;
    r.sin_addr.s_addr = INADDR_ANY;
    r.sin_port = htons(PORT);  // make sure this is PORT, capitalized!

    if (bind(listenfd, (struct sockaddr *)&r, sizeof r)) {
		perror("bind");
		exit(1);
    }

    // threshold set to macro constant, so that we can change easily later if desired
    if (listen(listenfd, QUEUE_LENGTH)) {
		perror("listen");
		exit(1);
    }
}


static void newconnection() {
	printf("in new connection\n");
	/* accept connection, sing to them, get response, update linked list */
	// This part of code is adapted from muffinman.
    int fd;
    struct sockaddr_in r;
    socklen_t socklen = sizeof(r);
    int len, c;
    // this buffer is for notifying clients
    char buf[BUF_LENGTH];
    char *buf_ptr = buf;

    if ((fd = accept(listenfd, (struct sockaddr *)&r, &socklen)) < 0) {
		perror("accept");
		exit(1);
    } else {
    	time_t *start_pt = &starttime;
				  
		printf("connection from %s\n", inet_ntoa(r.sin_addr));
		fflush(stdout);
		struct active_client *new_p = addclient(fd, r.sin_addr);
		
		if (write(new_p->fd, start_pt, sizeof(start_pt)) == -1){
            perror("write start time");
            exit(1);
    	}
		/* notify */
		sprintf(buf, login_info, MAX_NAME - 1);
		notify(new_p, buf, strlen(buf));

		/* wait for and get response -- look for first non-whitespace char */
		/* (buf is a good enough size for reads -- it probably will all fit
	 	* into one read() for any non-trivial size.) */
		c = -2;  // initial state				
		while (c == -2) {
	        
	    	int nbytes;
	        if ((nbytes = read(fd, buf, BUF_LENGTH)) > 0) {
                   buf[nbytes] = '\0';
                   if (strlen(buf) == 0){			    		
		    			strcpy(buf, "unknown");	
		    		}
                   printf("have read name: %s\n",  buf);	
                   c = 0;
            }
            else{
            	if (errno != EINTR)			    		
			    	c = -1;		
            }
		}
		/* react to response */
		if (c == 0) {
	    	printf("Received username input from %s\n", inet_ntoa(r.sin_addr));
	    	printf("The current user is %s\n", buf);
	    	fflush(stdout);
	    	record_username_for_client(new_p, buf);
		} else if(c == -1){
			printf("Communication error or timeout\n");
			fflush(stdout);
			removeclient(new_p->fd);
		} else{
			printf("no username is given\n");
		}
    }
}

static struct active_client* addclient(int fd, struct in_addr addr) {
	printf("in add client\n");
	// adapted from muffinman
    struct active_client *p = malloc(sizeof(struct active_client));
    if (!p) {
    	/* highly unlikely to happen */
		fprintf(stderr, "out of memory!\n");
		exit(1);
    }
    printf("Adding client %s\n", inet_ntoa(addr));
    fflush(stdout);
    p->fd = fd;
    p->ipaddr = addr;
    p->next = top_client;
    p->played = 0;
    p->submitted_score = 1;
    p->in_game = 0; 
    p->current_game_score = 0;
    p->appeared_words = construct_empty_dictionary();
    top_client = p;
    return p;
}

static void removeclient(int fd)
{    printf("in remove client\n");
	// This part of code is adapted from muffinman
    struct active_client **p;
    for (p = &top_client; *p && (*p)->fd != fd; p = &(*p)->next)
	;
    if (*p) {
		struct active_client *t = (*p)->next;
		printf("Removing client %s\n", inet_ntoa((*p)->ipaddr));
		fflush(stdout);
		free(*p);
		*p = t;
		if (close(fd) < 0) {
			perror("close");
			exit(1);
		}
    } else {
		fprintf(stderr, "Trying to remove fd %d, but I don't know about it\n", fd);
		fflush(stderr);
    }
}

static void notify(struct active_client *p, char *s, int size) {
	if (write(p->fd, s, size) == -1) {
		perror("write");
		exit(1);
	}
}

static void process_client(struct active_client *p, char **board, DNode **dictionary) {
	printf("in process_client\n");
	char buf[BUF_LENGTH];
	char *buf_ptr = buf;
	int len = read(p->fd, buf, sizeof(buf));
	printf("have read: %s\n",  buf);


	if (len > 0) {
		// could be just the newline character
		if (buf[0] == '\r') {
			// do nothing, and wait till the next message
			return;
		} else {
			// input could be from file or stdin
			// newline character may or may not be at the end
			if (buf[len - 1] == '\n') buf[len - 2] = '\0';
			else buf[len] = '\0';

			if (strlen(p->name) == 0) {
				record_username_for_client(p, buf);
			} else if (strcmp(buf, "all_players") == 0) {
				all_players(p);
			} else if (strcmp(buf, "top_3") == 0) {
				top_3(p);
			} else if (strncmp(buf, "add_score", 9) == 0) {
				if (p->played == 0) {
					notify(p, not_played, strlen(not_played));
				} else {
					update_player(p);
				}
			} else if (strcmp(buf, "new_game") == 0) {
				if (p->submitted_score == 0){
					notify(p, not_submit, strlen(not_submit));
				} else {
					new_game(p, board);
				}
			} else if (strcmp(buf, "quit") == 0) {
				quit(p);
			} else {
				if(p->in_game == 1){
					
					buf_ptr[strcspn(buf, "\r\n")] = '\0';  
					strupr(&buf_ptr);
                    if (check_word_valid(buf_ptr, p->appeared_words, dictionary, board)) {
						valid_word_operate(p, buf_ptr, p->appeared_words);
					} else {
						invalid_word_operate(p, buf_ptr, p->appeared_words, dictionary);
					}	

				}else{
					notify(p, invalid_syntax, strlen(invalid_syntax));
				}	
			}
		}
	} else if (len == 0) {
		printf("Removing client: %s\n", p->name);
		removeclient(p->fd);
    } else {
		perror("read");
		printf("Removing client: %s\n", p->name);
		removeclient(p->fd);
	}
	//printf("finish process end\n");
}

static void record_username_for_client(struct active_client *new_p, char *buf) {
    printf("in record username for client\n");		
	int mode = add_player(buf, &top_player);		
	char *buf_ptr = buf;		
	if (mode == 0) {		
		// this player has not appeared before		
		strcpy(new_p->name, buf);		
		buf_ptr += sprintf(buf_ptr, "Welcome.\r\n");		
	} else if (mode == 1) {		
		// this player has appeared before		
		strcpy(new_p->name, buf);		
		buf_ptr += sprintf(buf_ptr, "Welcome back.\r\n");		
	} else {		
		// username too long, truncated		
		strncpy(new_p->name, buf, MAX_NAME);		
		new_p->name[MAX_NAME - 1] = '\0';		
		buf_ptr += sprintf(buf_ptr, "Username too long, truncated to %d chars.\r\n", MAX_NAME - 1);		
		int next_mode = add_player(new_p->name, &top_player);		
		if (next_mode == 0)		
			buf_ptr += sprintf(buf_ptr, "Welcome.\r\n");		
		else if (next_mode == 1)		
			buf_ptr += sprintf(buf_ptr, "Welcome back.\r\n");		
		else {		
			fprintf(stderr, "Exception during adding player...\n");		
			fflush(stderr);		
		}		
	}		
	sprintf(buf_ptr, "Go ahead and enter user commands>\r\n");		
	notify(new_p, buf, strlen(buf));		
}

void all_players(struct active_client *p) {
	
	char buf[BUF_LENGTH];
	char *buf_ptr = buf;
	struct player *cur = top_player;
	while (cur && buf_ptr - buf < BUF_LENGTH - 1) {
		add_info_to_buf(cur, &buf_ptr, buf);
		cur = cur->next;
	}
	if (buf_ptr - buf == BUF_LENGTH - 1) {
		overflow_notify(p);
	}
	notify(p, buf, strlen(buf));
}

void top_3(struct active_client *p) {
	char buf[BUF_LENGTH];
	char *buf_ptr = buf;
	struct player *cur;
	int l = length(top_player);
	l = l < 3 ? l : 3;
	int remain = l;
	int mark[3] = {-1, -1, -1};

	while (remain-- > 0) {
		int counter = 0, max_score = -1, max_index = -1;
		struct player *cur = top_player;
		struct player *max_player = NULL;
		while (cur) {
			int checked = 0;
			for (int i = 0; i < l - remain; i++) {
				if (counter == mark[i]) {
					checked = 1;
					break;
				}
			}
			if (cur->max_score > max_score && !checked) {
				max_index = counter;
				max_score = cur->max_score;
				max_player = cur;
			}
			cur = cur->next;
			counter++;
		}
		mark[l - 1 - remain] = max_index;
		if (max_player == NULL) {
			fprintf(stderr, "did not find the %d highest player\n", l - remain);
		}
		buf_ptr += snprintf(buf_ptr, (buf-buf_ptr)+BUF_LENGTH-1, "%d.User: %s, ", l - remain, max_player->name);
		buf_ptr += snprintf(buf_ptr, (buf-buf_ptr)+BUF_LENGTH-1, "Max score: %d\r\n", max_player->max_score);
	}
	if (buf_ptr - buf == BUF_LENGTH - 1) {
		overflow_notify(p);
	}
	notify(p, buf, strlen(buf));
}

void update_player(struct active_client *p) {
	int score = p->current_game_score;
	char buf[BUF_LENGTH];
	char *buf_ptr = buf;
	struct player *cur = top_player;
	printf("in update_player\n");
	
	while (cur) {
		if (strcmp(cur->name, p->name) == 0) {
			
			cur->total_games++;
			cur->total_score += score;
			cur->max_score = cur->max_score > score ? cur->max_score : score;
			break;
		}
		cur = cur->next;
	}
	
	buf_ptr += snprintf(buf_ptr, (buf-buf_ptr)+BUF_LENGTH-1, "Updated player %s, ", cur->name);
	buf_ptr += snprintf(buf_ptr, (buf-buf_ptr)+BUF_LENGTH-1, "now the statistics are:\r\n");
	
	add_info_to_buf(cur, &buf_ptr, buf);
	printf("after add info\n");
	if (buf_ptr - buf == BUF_LENGTH - 1)
		overflow_notify(p);
	notify(p, buf, strlen(buf));
	printf("after notify info\n");	
	p->played = 0;
	p->submitted_score = 1;
	p->in_game = 0;
	p->current_game_score = 0;
	printf("end of update\n");
}

void new_game(struct active_client *p, char **board) {   
	char board_content[LENGTH*(LENGTH+2)+1];
	ncontent(board, board_content);
	notify(p, board_content, strlen(board_content));
	p->played = 1;
	p->submitted_score = 0;
	p->in_game = 1;
}

void quit(struct active_client *p) {
	
	char buf[BUF_LENGTH];
	sprintf(buf, "Bye! Ending your connection...\r\n");
	notify(p, buf, strlen(buf));

	printf("Removing client: %s\n", p->name);
	removeclient(p->fd);
}

void overflow_notify(struct active_client *p) {
	fprintf(stderr, "Buffer overflow\n");
	notify(p, overflow_note, strlen(overflow_note));
}

void add_info_to_buf(struct player *cur, char **buf_ptr, char *buf) {
	printf("print scores\n");
	
	// use snprintf instead of sprintf to avoid buffer overflow
	*buf_ptr += snprintf(*buf_ptr, (buf-*buf_ptr)+BUF_LENGTH-1, "User: %s\r\n", cur->name);
	buf_ptr += snprintf(*buf_ptr, (buf-*buf_ptr)+BUF_LENGTH-1, "Games played: %d\r\n", cur->total_games);
	//buf_ptr += snprintf(*buf_ptr, (buf-*buf_ptr)+BUF_LENGTH-1, "Total score: %d\r\n", cur->total_score);
	//buf_ptr += snprintf(*buf_ptr, (buf-*buf_ptr)+BUF_LENGTH-1, "Max score: %d\r\n\r\n", cur-> max_score);
	printf("print scores success\n");
}

void sig_handler(int signo)		
{		
	printf("in sig handler\n");
  if (signo == SIGALRM){		
  		free_board(board);		
  	    board = construct_board();		
  	    for (p = top_client; p; p = p->next){	

  	         free_dictionary(p->appeared_words, SMALL_HASH_SIZE);
  	    	 p->appeared_words = construct_empty_dictionary(); 	

  	    	 update_player(p);	
  	    	 printf("in signal client list\n"); 
             notify(p, board_changed, strlen(board_changed));		
  	    }		
  }		
}		
void set_alarm(){
	
	struct sigaction sa;
	sa.sa_handler = sig_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGALRM, &sa, NULL);

	struct itimerval it_val;
	it_val.it_value.tv_sec = INTERVAL/1000;		
    it_val.it_value.tv_usec = (INTERVAL*1000) % 1000000;		
    it_val.it_interval = it_val.it_value;		
	if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {		
	  perror("error calling setitimer()");		
	  exit(1);		
	}
	starttime = time(NULL);
		
}

void valid_word_operate(struct active_client *p, char *word, DNode **appeared_words) {
	char buf[BUF_LENGTH];
	char *buf_ptr = buf;

	int i, j;
	for (i = 0; i < LENGTH; i++) {
		for (j = 0; j < LENGTH; j++) {
			buf_ptr += sprintf(buf_ptr, "%c", board[i][j]);
		}
		buf_ptr += sprintf(buf_ptr, "\r\n");
	}

	
	buf_ptr += sprintf(buf_ptr, "Yay! You found a valid word!!!\r\n");
	//notify(p, correct, strlen(correct));
	
	insert(appeared_words, SMALL_HASH_SIZE, word);
    
    int length = strlen(word);
    if(length < 3){
    	buf_ptr += sprintf(buf_ptr, "But the word is too short to score\r\n");
    	//notify(p, tooshort, strlen(tooshort));
    }
	else if (length >= 3 && length <= 4)
		p->current_game_score += 1;
	else if (length == 5)
		p->current_game_score += 2;
	else if (length == 6)
		p->current_game_score += 3;
	else if (length == 7)
		p->current_game_score += 5;
	else if (length >= 8)
		p->current_game_score += 11;

	notify(p, buf, strlen(buf));
}


void invalid_word_operate(struct active_client *p, char *word, DNode **appeared_words, DNode **dictionary) {
	char buf[BUF_LENGTH];
	char *buf_ptr = buf;

	int i, j;
	for (i = 0; i < LENGTH; i++) {
		for (j = 0; j < LENGTH; j++) {
			buf_ptr += sprintf(buf_ptr, "%c", board[i][j]);
		}
		buf_ptr += sprintf(buf_ptr, "\r\n");
	}

    buf_ptr += sprintf(buf_ptr, "incorrect word\r\n");
	//notify(p, incorrect, strlen(incorrect));

	if (lookup(appeared_words, SMALL_HASH_SIZE, word) != NULL) {
		buf_ptr += sprintf(buf_ptr, "You've already found this word before\r\n");
		//notify(p, foundbefore, strlen(foundbefore));
	} else if (lookup(dictionary, BIG_HASH_SIZE, word) == NULL) {
		buf_ptr += sprintf(buf_ptr, "The word is not a valid english word according to our dictionary\r\n");
		//notify(p, notword, strlen(notword));
	} else {
		buf_ptr += sprintf(buf_ptr, "The word cannot be found on the board\r\n");
		//notify(p, notboard, strlen(notboard));
	}

	notify(p, buf, strlen(buf));
}
