//Author: Garret Sweetwood
//CS 344
//Program 4
//Date 08/11/17


#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<netinet/in.h>

int main(int argc, char ** argv) {
	int i;
	int listening_port;
	int listen_socket;
	int establishedConnectionFD;
	int status;

						//if port not specified
	if (argc < 2) {
						//Print error
		fprintf(stderr, "USAGE: %s port\n");
		exit(1);
	}
	else {
						//get listening port from args
		listening_port = atoi(argv[1]);
	}

						//Create socket
	if ((listen_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {													
		error("ERROR opening socket");
		exit(1);
	}

						//Fill addr struct
	struct sockaddr_in server;

	server.sin_family = AF_INET;
	server.sin_port = htons(listening_port);
	server.sin_addr.s_addr = INADDR_ANY;

						//bind socket to port
	if (bind(listen_socket, (struct sockaddr *) &server, sizeof(server)) == -1) {
		error("ERROR on binding");
		exit(1);
	}

						//Listen for connections
	if (listen(listen_socket, 5) == -1) {
		error("ERROR on listen");
		exit(1);
	}

						//loop and accept
	while (1) { 
		establishedConnectionFD = accept(listen_socket, NULL, NULL);
		if (establishedConnectionFD == -1) {
			error("ERROR on accept");
			exit(1);
		}

						//fork
		int pid = fork();

		if (pid == -1) {
			fprintf(stderr, "fork error\n");
		}
		else if (pid == 0) {//THIS IS THE CHILD
			int toSend = htonl(0);

			if (send(establishedConnectionFD, &toSend, sizeof(toSend),
				0) == -1) {
				fprintf(stderr, "client send failed\n");
			}
						//get size of cipher text
			int cNum;
			if (recv(establishedConnectionFD, &cNum, sizeof(cNum), 0) == -1) {
				fprintf(stderr, "ERROFrecv cipher text size \n");
			}
			else if (cNum == 0) {
						//Plain text file size == 0
				fprintf(stderr, "ERROR recv cipher text size of 0\n");
			}

						//cLen == length of cipher text file
			int cLen = ntohl(cNum);//convert

						//get size of key text
			int kNum;
			if (recv(establishedConnectionFD, &kNum, sizeof(kNum), 0) == -1) {
				fprintf(stderr, "ERROR recv key text size\n");
			}
			else if (kNum == 0) {
				fprintf(stderr, "ERROR recv key text size of 0\n");
			}

							//kLen == length of key file
			int kLen = ntohl(kNum);//convert

								   //Allocate memory for cipher text
			char *cipherText = malloc(sizeof(char) * cLen);
			char buffer[1024];

							//Clear cipher text
			memset(cipherText, '\0', cLen);

							//Receive cipher text
			int len = 0;
			int r;
			while (len <= cLen) { 
				memset((char *)buffer, '\0', sizeof(buffer));
				r = recv(establishedConnectionFD, &buffer, 1024, 0);

				if (r == -1) {
					fprintf(stderr, "ERROR recv cipher text file\n");
					break;
				}
				else if (r == 0) {
					if (len < cLen) {
						break;
					}
				}
				else {
							//Concat string
					strncat(cipherText, buffer, (r - 1));
				}

				len += (r - 1);//add len received to total len 
			}

			cipherText[cLen - 1] = '\0';

								//Allocate memory for key text
			char *keyText = malloc(sizeof(char) * kLen);
			memset((char *)&buffer, '\0', sizeof(buffer));
			memset(keyText, '\0', kLen);

								//Receive key text
			len = 0;

			while (len <= kLen) {
								 
					memset((char *)buffer, '\0', sizeof(buffer));

				r = recv(establishedConnectionFD, &buffer, 1024, 0);//receive

				if (r == -1) {
					fprintf(stderr, "ERROR recv key text file\n");
					break;
				}
				else if (r == 0) {
					break;
				}
				else {
					strncat(keyText, buffer, (r - 1));
				}

				len += (r - 1);		//add len received to total len 
			}

			keyText[kLen - 1] = '\0';

			int cipherNum;
			int keyNum;
			int decNum;
								//Decrypt the cipher text file using key
			for (i = 0; i < cLen - 1; i++) {
				if (cipherText[i] == ' ') {
					cipherNum = 26;
				}
				else {
					cipherNum = cipherText[i] - 65;
				}

								//change key chars to ints 0-26
				if (keyText[i] == ' ') {//space
					keyNum = 26;
				}
				else {
					keyNum = keyText[i] - 65;
				}

								//Determine decrypted char
				decNum = cipherNum - keyNum;
				if (decNum < 0) {
					decNum += 27;
				}

								//replace cipher char with decrypted char
				if (decNum == 26) { //space
					cipherText[i] = ' ';
				}
				else {
					cipherText[i] = 'A' + (char)decNum;
				}
			}

							//send back decrypted file
			len = 0;

			while (len <= cLen) {
				char plainSend[1024];
				strncpy(plainSend, &cipherText[len], 1023);

				plainSend[1024] = '\0'; 
					if (send(establishedConnectionFD, &plainSend, 1024, 0) == -1) {
						fprintf(stderr, "decryption text send\n");
					}

				len += 1023; //add sent len to total leni
			}

								//Clean up
			free(cipherText);
								
		}
		else {//THIS IS THE PARENT
			close(establishedConnectionFD);

			//Wait for children
			do {
				waitpid(pid, &status, 0);
			} while (!WIFEXITED(status) && !WIFSIGNALED(status));
		}
	}

	//close socket
	close(listen_socket);

	return 0;
}