/* demo_server.c - code for example server program that uses TCP */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "trie.h"

#define QLEN 6 /* size of request queue */
int visits = 0; /* counts client connections */
uint8_t length;
uint8_t period;

/*------------------------------------------------------------------------
* Program: demo_server
*
* Purpose: allocate a socket and then repeatedly execute the following:
* (1) wait for the next connection from a client
* (2) send a short message to the client
* (3) close the connection
* (4) go back to step (1)
*
* Syntax: ./demo_server port
*
* port - protocol port number to use
*
*------------------------------------------------------------------------
*/

char buf[10]; /* buffer for string the server sends */
char* dictionary;
struct TrieNode* dictRoot;
struct TrieNode* guessRoot;

void playGame(int sd2, int sd3, uint8_t length, uint8_t period);

void createBoard(char* board, uint8_t length);

void makeDictionary();

bool checkChars(char* guess, char* board);

void checkConnection(int status, int sd2, int sd3);

int main(int argc, char **argv) {
	struct protoent *ptrp; /* pointer to a protocol table entry */
	struct sockaddr_in sad; /* structure to hold server's address */
	struct sockaddr_in cad; /* structure to hold client's address */
	int sd, sd2, sd3; /* socket descriptors */
	int port; /* protocol port number */
	int alen; /* length of address */
	int optval = 1; /* boolean value when we set socket option */

	if( argc != 5 ) {
		fprintf(stderr,"Error: Wrong number of arguments\n");
		fprintf(stderr,"usage:\n");
		fprintf(stderr,"./server server_port word\n");
		exit(EXIT_FAILURE);
	}

	dictRoot = getNode();
	guessRoot = getNode();

	length = (uint8_t) atoi(argv[2]);
	period = (uint8_t) atoi(argv[3]);
	dictionary = argv[4];
	makeDictionary();

	memset((char *)&sad,0,sizeof(sad)); /* clear sockaddr structure */
	sad.sin_family = AF_INET; /* set family to Internet */
	sad.sin_addr.s_addr = INADDR_ANY; /* set the local IP address */

	port = atoi(argv[1]); /* convert argument to binary */
	if (port > 0) { /* test for illegal value */
		sad.sin_port = htons((u_short)port);
	} else { /* print error message and exit */
		fprintf(stderr,"Error: Bad port number %s\n",argv[1]);
		exit(EXIT_FAILURE);
	}

	/* Map TCP transport protocol name to protocol number */
	if ( ((long int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	/* Create a socket */
	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sd < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/* Allow reuse of port - avoid "Bind failed" issues */
	if( setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0 ) {
		fprintf(stderr, "Error Setting socket option failed\n");
		exit(EXIT_FAILURE);
	}

	/* Bind a local address to the socket */
	if (bind(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
		fprintf(stderr,"Error: Bind failed\n");
		exit(EXIT_FAILURE);
	}

	/* Specify size of request queue */
	if (listen(sd, QLEN) < 0) {
		fprintf(stderr,"Error: Listen failed\n");
		exit(EXIT_FAILURE);
	}

	/* Main server loop - accept and handle requests */
	while (1) {
		alen = sizeof(cad);
		if ( (sd2=accept(sd, (struct sockaddr *)&cad, &alen)) < 0) {
			fprintf(stderr, "Error: Accept failed\n");
			exit(EXIT_FAILURE);
		}

		char player = '1';
		send(sd2, &player, 1, 0);
		send(sd2, &length, sizeof(uint8_t),0);
		send(sd2, &period, sizeof(uint8_t), 0);
		sd3 = 0;
		while(sd3 == 0){
			if ( (sd3=accept(sd, (struct sockaddr *)&cad, &alen)) < 0) {
				fprintf(stderr, "Error: Accept failed\n");
				exit(EXIT_FAILURE);
			}
		}
		player = '2';
		send(sd3, &player, 1, 0);
		send(sd3, &length, sizeof(uint8_t),0);
		send(sd3, &period, sizeof(uint8_t), 0);
		signal(SIGCHLD,SIG_IGN); //kill zombies/children
		pid_t p = fork();

		if( p < 0 ) {
			fprintf(stderr,"fork failed\n");
			return 1;
		} else if( p == 0 ) { //child
			playGame(sd2,sd3,length,period);
		} else {	//parent

		}
	}
}
void playGame(int sd2, int sd3, uint8_t length, uint8_t period){
	uint8_t score1 = 0;
	uint8_t score2 = 0;
	uint8_t around = 1;
	int connected1 = 1;
	int connected2 = 1;
	int active = sd2;
	int inactive = sd3;
	char y = 'Y';
	char n = 'N';
	char board[length];
	char guess[length];
	// run this once, before you call recv:

	while(score1 <= 3 && score2 <= 3 && connected1 > 0 && connected2 > 0){
		checkConnection(send(sd2, &score1, sizeof(uint8_t),MSG_NOSIGNAL), sd2, sd3);
		send(sd2, &score2, sizeof(uint8_t), 0);
		send(sd2, &around, sizeof(uint8_t), 0);
		checkConnection(send(sd3, &score1, sizeof(uint8_t),MSG_NOSIGNAL), sd2, sd3);
		send(sd3, &score2, sizeof(uint8_t), 0);
		send(sd3, &around, sizeof(uint8_t), 0);

		if(score1 == 3 || score2 == 3){
			break;
		}

		createBoard(board, length);

		checkConnection(send(sd2, &board, sizeof(board), MSG_NOSIGNAL), sd2, sd3);
		checkConnection(send(sd3, &board, sizeof(board), MSG_NOSIGNAL), sd2, sd3);

		if (around % 2 == 1){
			active = sd2;
			inactive = sd3;
		}
		else{
			active = sd3;
			inactive = sd2;
		}

		int mistake = 1;
		int turn = active;
		int notturn = inactive;

		while(mistake != 0 && connected1 > 0 && connected2 > 0){
			checkConnection(send(turn, &y, 1, MSG_NOSIGNAL), sd2, sd3); // Sends Y to active player
			checkConnection(send(notturn, &n, 1, MSG_NOSIGNAL), sd2, sd3); // Sends N to inactive player
			uint8_t temp = 1;

			struct timeval tv;
			tv.tv_sec = period;  // numSeconds is the turn length
			tv.tv_usec = 0;
			setsockopt(turn,SOL_SOCKET,SO_RCVTIMEO, &tv, sizeof(struct timeval));

			// call recv as you normally do, but make sure to store the return value:
			uint8_t guessLength = 0;
			recv(turn, &guessLength, sizeof(uint8_t), 0); // How many characters are guessed by client
			int n = recv(turn, &guess, guessLength, 0);//get word from client and compare to dictionary
			int optval = 1;
			setsockopt(turn, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

			if(errno == EAGAIN && n < 0) { //timed out
				mistake = 0;
				temp = 0;
				checkConnection(send(sd2, &temp, sizeof(uint8_t), MSG_NOSIGNAL), sd2, sd3);
				checkConnection(send(sd3, &temp, sizeof(uint8_t), MSG_NOSIGNAL), sd2, sd3);
				around++;
				if(turn == sd2){
					score2++;
				}else{
					score1++;
				}
			}else{
				guess[guessLength-1] = '\0';
				bool valid = search(dictRoot, guess) && !search(guessRoot, guess) && checkChars(guess, board);

				if(valid == 1){
					insert(guessRoot, guess);
					checkConnection(send(turn, &temp, sizeof(uint8_t), MSG_NOSIGNAL), sd2, sd3);
					checkConnection(send(notturn, &guessLength, sizeof(uint8_t), MSG_NOSIGNAL), sd2, sd3); // Sends inactive player infomation
					checkConnection(send(notturn, &guess, guessLength, MSG_NOSIGNAL), sd2, sd3); // If active player had a valid guess
				}else{
					mistake = 0;
					temp = 0;
					checkConnection(send(sd2, &temp, sizeof(uint8_t), MSG_NOSIGNAL), sd2, sd3);
					checkConnection(send(sd3, &temp, sizeof(uint8_t), MSG_NOSIGNAL), sd2, sd3);
					if(turn == sd2){
						score2++;
					}else{
						score1++;
					}
					around++;
				}
			}
			if(turn == sd2){
				turn = sd3;
				notturn = sd2;
			}else{
				turn = sd2;
				notturn = sd3;
			}
		}
	}
	close(sd2);
	close(sd3);
}

void createBoard(char* board, uint8_t length){
	for(int i = 0; i < length; i++){
		board[i] = "abcdefghijklmnopqrstuvwxyz"[random() % 26];
	}
	int flag = 0;
	for(int i = 0; i < length; i++){
		if(board[i] == 'a' || board[i] == 'e' || board[i] == 'i'
		|| board[i] == 'o' || board[i] == 'u'){
			flag = 1;
		}
	}
	if (flag == 0){
		board[length-1] = "aeiou"[random() % 5];
	}
}

void makeDictionary(){
	char line[40];
	FILE *fp = fopen(dictionary, "r");
	if(fp == NULL){
		fprintf(stderr,"./Dictionary file not found\n");
		exit(EXIT_FAILURE);
	}else{
		while (fgets(line, sizeof(line), fp)) {
			strtok(line, "\n");
			insert(dictRoot, line); //seg fault
		}
	}
}

bool checkChars(char* guess, char* board){
	char* copy[length];
	for(int i = 0; i < length; i++){
		copy[i] = board[i];
	}
	copy[length] = '\0';
	for(int i = 0; i < strlen(guess); i++){
		bool found = false;
		for(int j = 0; j < length; j++){
			if(copy[j] == guess[i]){
				found = true;
				j = length;
				copy[j] = '0';
			}
		}
		if(found == false){
			return false;
		}
	}
	return true;
}

void checkConnection(int status, int sd2, int sd3){
	if(status <= 0){
		close(sd2);
		close(sd3);
	}
}
