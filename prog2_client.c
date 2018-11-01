/* demo_client.c - code for example client program that uses TCP */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/*------------------------------------------------------------------------
* Program: demo_client
*
* Purpose: allocate a socket, connect to a server, and print all output
*
* Syntax: ./demo_client server_address server_port
*
* server_address - name of a computer on which server is executing
* server_port    - protocol port number server is using
*
*------------------------------------------------------------------------
*/
char buf[1000]; /* buffer to store guesses from cilents */

void checkConnection(int status, int sd);

int main( int argc, char **argv) {
	struct hostent *ptrh; /* pointer to a host table entry */
	struct protoent *ptrp; /* pointer to a protocol table entry */
	struct sockaddr_in sad; /* structure to hold an IP address */
	int sd; /* socket descriptor */
	int port; /* protocol port number */
	char *host; /* pointer to host name */
	int n; /* number of characters read */

	memset((char *)&sad,0,sizeof(sad)); /* clear sockaddr structure */
	sad.sin_family = AF_INET; /* set family to Internet */

	if( argc != 3 ) {
		fprintf(stderr,"Error: Wrong number of arguments\n");
		fprintf(stderr,"usage:\n");
		fprintf(stderr,"./client server_address server_port\n");
		exit(EXIT_FAILURE);
	}

	port = atoi(argv[2]); /* convert to binary */
	if (port > 0) /* test for legal value */
	sad.sin_port = htons((u_short)port);
	else {
		fprintf(stderr,"Error: bad port number %s\n",argv[2]);
		exit(EXIT_FAILURE);
	}

	host = argv[1]; /* if host argument specified */

	/* Convert host name to equivalent IP address and copy to sad. */
	ptrh = gethostbyname(host);
	if ( ptrh == NULL ) {
		fprintf(stderr,"Error: Invalid host: %s\n", host);
		exit(EXIT_FAILURE);
	}

	memcpy(&sad.sin_addr, ptrh->h_addr, ptrh->h_length);

	/* Map TCP transport protocol name to protocol number. */
	if ( ((long int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	/* Create a socket. */
	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sd < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/* Connect the socket to the specified server. */
	if (connect(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
		fprintf(stderr,"connect failed\n");
		exit(EXIT_FAILURE);
	}

	char player;
	uint8_t length;
	uint8_t period;
	uint8_t p1 = 0;
	uint8_t p2 = 0;
	uint8_t around;
	char turn;
	recv(sd, &player, 1, MSG_WAITALL);
	recv(sd, &length, sizeof(uint8_t), 0);
	recv(sd, &period, sizeof(uint8_t), 0);
	printf("You are Player %c\n", player);
	if(player == '1'){
		printf("You are the only player, game will begin whenever another player joins\n");
	}
	printf("Board size: %d\n", length);
	printf("Seconds per turn: %d\n", period);
	char board[length]; /* buffer for data from the server */
	char guess[length];
	int connected = 1;

	while(p1 != 3 && p2 != 3 && connected != 0){
		//connected = read(sd,buf,sizeof(buf)); //Come back to this
		//See if server is still connected

		recv(sd, &p1, sizeof(uint8_t), 0);
		recv(sd, &p2, sizeof(uint8_t), 0);

		if(p1 == 3 || p2 == 3){
			break;
		}

		recv(sd, &around, sizeof(uint8_t), 0);
		printf("Round %d...\nScore is %d-%d\n",around,p1,p2);
		recv(sd, &board, length, 0);
		board[length] = '\0';
		printf("Board: %s\n",board);

		uint8_t valid = 1;
		while(valid != 0){
			recv(sd, &turn, 1, 0);
			if (turn == 'Y'){
				printf("It is your turn! Enter a word: ");
				//playTurn
				fgets(guess, sizeof(guess), stdin); // need to not still ask for input if timed out
				uint8_t guessLength = strlen(guess);

				checkConnection(send(sd, &guessLength, sizeof(uint8_t), MSG_NOSIGNAL), sd); // Sends fguess and size
				send(sd, &guess, guessLength, 0);	// To server

				recv(sd, &valid, sizeof(uint8_t), 0); // Finds out if guess is valid
				if(valid == 0){
					printf("Invalid word\n");
				}else{
					printf("Valid word\n");
				}
			}else{
				printf("It is the other players turn. Please wait\n");
				uint8_t enemy;
				checkConnection(recv(sd, &enemy, sizeof(uint8_t), MSG_NOSIGNAL), sd);
				if(enemy == 0){
					valid = 0;
				}else{
					recv(sd, &guess, enemy, 0);
					guess[enemy] = '\0';
					printf("Your enemy guessed: %s\n", guess);
				}
			}
		}
		printf("\n");
	}

	if(player == '1'){
		printf("You scored: %d\n", p1);
		if(p1 == 3){
			printf("YOU WON!\n");
		}else{
			printf("YOU LOST!\n");
		}
	}else{
		printf("You scored: %d\n", p2);
		if(p2 == 3){
			printf("YOU WON!\n");
		}else{
			printf("YOU LOST!\n");
		}
	}

	close(sd);

	exit(EXIT_SUCCESS);
}

void checkConnection(int status, int sd){
	if(status <= 0){
		close(sd);
		exit(1);
	}
}
