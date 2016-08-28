#define _XOPEN_SOURCE
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include <time.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#include "dictionary.h"
#include "construct_word_dict.h"
#include "word_checker.h"


#ifndef PORT
  #define PORT 53515
#endif

#define QUEUE_LENGTH 5
#define BUF_LENGTH 1000
#define INTERVAL 120            //in second 

char buf[BUF_LENGTH];
int soc; 

void sig_handler(int signo)		
{		
	if (signo == SIGALRM){	
		for(int i = 0; i < BUF_LENGTH; i++){
			buf[i] = '\0';
		}
		read(soc, buf, sizeof(buf));
		printf("%s\n", buf);		
		alarm(INTERVAL);    
	}		
}		
void set_alarm(time_t starttime){
	
	struct sigaction sa;
	sa.sa_handler = sig_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGALRM, &sa, NULL);
    
    time_t currenttime = time(NULL);
    int diff = currenttime - starttime;
    printf("time diff is %d\n", diff);
    int trueINTERVAL = INTERVAL - diff;
    alarm(trueINTERVAL);
		
}




int main(int argc, char** argv) {

  struct sockaddr_in peer;

  if ((soc = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("randclient: socket");
    exit(1);
  }

  peer.sin_family = AF_INET;
  peer.sin_port = htons(PORT);
  if (inet_pton(AF_INET, argv[1], &peer.sin_addr) < 1) {
    perror("randclient: inet_pton");
    close(soc);
    exit(1);
  }

  if (connect(soc, (struct sockaddr *)&peer, sizeof(peer)) == -1) {
    perror("randclient: connect");
    exit(1);
  }
  
  time_t starttime;
  time_t *start_pt = &starttime;
  if(read(soc, start_pt, sizeof(start_pt)) == -1){
  	perror("read start time");
  	exit(1);
  }

  set_alarm(starttime);
  

  char m[1000];
  char name[100];
  char command[100];

  read(soc, m, sizeof(m));
  printf("%s", m);
  scanf("%s", name);
  write(soc, name, sizeof(name));
  
  //printf("here should print welcom go ahead message: \n");
  read(soc, buf, sizeof(buf));
  printf("%s\n", buf);
  
  //printf("here we submit first command: \n");
  scanf("%s", command);
  write(soc, command, sizeof(command));

  while(strcmp(command, "quit") != 0){
    
    for(int i = 0; i < BUF_LENGTH; i++){
    	buf[i] = '\0';
    }
    read(soc, buf, sizeof(buf));
    printf("%s\n", buf);
    
    for(int i = 0; i < 100; i++){
    	command[i] = '\0';
    }

    printf("Please give your input: \n");
    scanf("%s", command);
  	write(soc, command, sizeof(command)); 	
    
  }

  
  close(soc);
  return 0;
}


