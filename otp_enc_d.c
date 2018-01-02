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
																
	if (argc < 2) {												//must specify a port
		fprintf(stderr, "USAGE: %s port\n", argv[0]);
		exit(1);
	}
	else {
		listening_port = atoi(argv[1]);
	}

	
	//printf("TRACE socket create\n");								//Create main socket
	if ((listen_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		error("ERROR opening socket");
		exit(1);
	}

																//Fill addr struct
	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(listening_port);
	serverAddress.sin_addr.s_addr = INADDR_ANY;

																//bind socket to port
	if (bind(listen_socket, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) == -1) {
		error("ERROR on binding");
		exit(1);
	}
	//printf("TRACE bind call\n");
																//Listen for connections
	if (listen(listen_socket, 5) == -1) {
		error("ERROR on listen");
		exit(1);
	}
	//printf("TRACE listen call\n");
																//loop and accept
	while (1) { 
		establishedConnectionFD = accept(listen_socket, NULL, NULL);
		if (establishedConnectionFD == -1) {	
			error("ERROR on accept");
			exit(1);
		}

																//fork
		int pid = fork();
		//printf("TRACE fork called\n");
		if (pid == -1) {
			fprintf(stderr, "fork error\n");
		}
		else if (pid == 0) {									//THIS IS THE CHILD
							
			int toSend = htonl(1);
			//printf("TRACE check send \n");
			if (send(establishedConnectionFD, &toSend, sizeof(toSend),
				0) == -1) {
				error("ERROR writing to socket");
				exit(1);
			}
																//get size of plain text
			int textNum;
			//printf("TRACE recv plaintext \n");
			if (recv(establishedConnectionFD, &textNum, sizeof(textNum), 0) == -1) {
				fprintf(stderr, "Error, recv size\n");
			}
			else if (textNum == 0) {
				fprintf(stderr, "Error, size is 0\n");
			}

			int textLength = ntohl(textNum);

																	//get size of key text
			int keySize;
			//printf("TRACE recv key \n");
			if (recv(establishedConnectionFD, &keySize, sizeof(keySize), 0) == -1) {
				fprintf(stderr, "ERROR recv key not correct\n");
			}
			else if (keySize == 0) {
				fprintf(stderr, "ERROR key is size 0\n");
			}

			int keyLength = ntohl(keySize);

																 //Allocate memory for plain text
			char *plainText = malloc(sizeof(char) * textLength);
			char buffer[1024];

																//Clear plain text
			memset(plainText, '\0', textLength);

																
			int len = 0;
			int r;
			//printf("TRACE start while loop recv \n");
			while (len <= textLength) {							
				memset((char *)buffer, '\0', sizeof(buffer));	//clear buffer each use

				r = recv(establishedConnectionFD, &buffer, 1024, 0);

				if (r == -1) {
					fprintf(stderr, "recv plain text file -1\n");
					break;
				}
				else if (r == 0) {
					if (len < textLength) {
						break;
					}
				}
				else {
																//Concat string each iteration
					strncat(plainText, buffer, (r - 1));
				}

				len += (r - 1);									//add len received to total len received
			}

			plainText[textLength - 1] = '\0';					//Add null terminator to end

																//Allocate memory for key text
			char *keyText = malloc(sizeof(char) * keyLength);
																//clear buffer and key
			memset((char *)buffer, '\0', sizeof(buffer));
			memset(keyText, '\0', keyLength);

			//Receive key text
			len = 0;

			while (len <= keyLength) {							//while whole string not received
				memset((char *)buffer, '\0', sizeof(buffer));
				r = recv(establishedConnectionFD, &buffer, 1024, 0);

				if (r == -1) {
					fprintf(stderr, "ERROR recv status -1 \n");
					break;
				}
				else if (r == 0) {
					break;
				}
				else {
																//Concat the string 
					strncat(keyText, buffer, (r - 1));
				}
				len += (r - 1);
			}

			keyText[keyLength - 1] = '\0';

			int plainNum;
			int keyNum;
			int encryptNum;
																//ENCRYPTION
			for (i = 0; i < textLength - 1; i++) {								
				if (plainText[i] == ' ') {
					plainNum = 26;
				}
				else {
					plainNum = plainText[i] - 65;
				}
				if (keyText[i] == ' ') {
					keySize = 26;
				}
				else {
					keySize = keyText[i] - 65;
				}

															//Determine encrypted the character
				encryptNum = plainNum + keySize;
				if (encryptNum >= 27) {							//If too large, subtract 27
					encryptNum -= 27;
				}
															
				if (encryptNum == 26) { 
					plainText[i] = ' ';
				}
				else {
					plainText[i] = 'A' + (char)encryptNum;
				}
			}

			len = 0;
			while (len <= textLength) {						//Continue until entire file is finished
				char cipherSend[1024];												
				strncpy(cipherSend, &plainText[len], 1023);
				cipherSend[1024] = '\0';					//add a null terminator to the end

				if (send(establishedConnectionFD, &cipherSend, 1024, 0) == -1) {
					fprintf(stderr, "encryption text send\n");
				}
				len += 1023;								//Add sent  lenght to len
			}													
			free(plainText);								//clean up
			free(keyText);
		}
		else{												//THIS IS THE PARENT
															//clean up connection
			close(establishedConnectionFD);

															
			do {											//Wait for children
				waitpid(pid, &status, 0);
			} while (!WIFEXITED(status) && !WIFSIGNALED(status));
		}
	}

															//Finish by closing the main socket
	close(listen_socket);

	return 0;
}