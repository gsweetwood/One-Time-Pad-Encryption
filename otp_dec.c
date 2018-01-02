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
#include<sys/stat.h>
#include<fcntl.h>
#include<netinet/in.h>
#include<netdb.h>

int main(int argc, char **argv) {
	int i;

					//Check for correct num of args
	if (argc < 4) {
		fprintf(stderr, "Must specifiy ciphertext, key, and port number\n");
		exit(1);
	}

						//Get port num from args
	int portNum = atoi(argv[3]);

						//open cipher text file and key for reading
	int fdCipher = open(argv[1], O_RDONLY);
	int fdKey = open(argv[2], O_RDONLY);

						//check that there was not an error opening
	if (fdCipher == -1 || fdKey == -1) {
		fprintf(stderr, "error opening files\n");
		exit(1);
	}

						//Get size of cipher text
	int cLen = lseek(fdCipher, 0, SEEK_END);

						//Get size of key text
	int kLen = lseek(fdKey, 0, SEEK_END);

						//Verify key file is larger than cipher text
	if (kLen < cLen) { //compare key to plain
		fprintf(stderr, "Key too short\n");
		exit(1);
	}

						//Create string to hold cipher text
	char *cipherText = malloc(sizeof(char) * cLen);

						//Set file point to begining of file
	lseek(fdCipher, 0, SEEK_SET);

						//Read cipher text into string
	if (read(fdCipher, cipherText, cLen) == -1) {
												 
		fprintf(stderr, "read cipher text dec\n");
		exit(1);
	}

						//Null terminate the string
	cipherText[cLen - 1] = '\0';

						//Check that chars in cipher text are valid
	for (i = 0; i < cLen - 1; i++) {
		if (isalpha(cipherText[i]) || isspace(cipherText[i])) {
		}
		else { 
			fprintf(stderr, "Cipher text invalid char\n");
			exit(1);
		}
	}

						//Create string to hold key text
	char *keyText = malloc(sizeof(char) * kLen);
						//Set file point to begining of file
	lseek(fdKey, 0, SEEK_SET);

						//Read key text into string
	if (read(fdKey, keyText, kLen) == -1) {
		fprintf(stderr, "read key text enc\n");
		exit(1);
	}

						//Null terminate the string
	keyText[kLen - 1] = '\0';

						//Check that chars in plain text are valid
	for (i = 0; i < kLen - 1; i++) {
		if (isalpha(keyText[i]) || isspace(keyText[i])) {
		}
		else {
			fprintf(stderr, "key text invalid char\n");
			exit(1);
		}
	}

						//create socket, client
	int socketfd;

	if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
															
		fprintf(stderr, "socket error\n");
		exit(1);
	}

						//Setting up address
	struct hostent * server_ip_address;
	server_ip_address = gethostbyname("localhost");

	if (server_ip_address == NULL) {
		fprintf(stderr, "could not resolve host name\n");
		exit(1);
	}

	struct sockaddr_in server;

						//clear socket structure
	memset((char *)&server, 0, sizeof(server));


	server.sin_family = AF_INET;
	server.sin_port = htons(portNum);
	memcpy(&server.sin_addr, server_ip_address->h_addr,
		server_ip_address->h_length);


						//Connect socket
	if (connect(socketfd, (struct sockaddr*) &server,
		sizeof(server)) == -1) {
		fprintf(stderr, "connect\n");
		exit(2);
	}

						//confirm connection
	int r;
	int conNum;
						//Receive confirmation number
	if ((r = recv(socketfd, &conNum, sizeof(conNum), 0)) == -1) {
		//If error receiving
		fprintf(stderr, "recv enc\n");
		exit(1);
	}
	else if (r == 0) {
		fprintf(stderr, "recv enc 0\n");
		exit(1);
	}

						//Check that confirmation number is correct
	int confirm = ntohl(conNum);

						//If number recieved is not correct
	if (confirm != 0) {
		fprintf(stderr, "could not contact otp_dec_d on port %d\n",
			portNum);
		exit(2);
	}

						//send cipher text file size
	int cLenSend = htonl(cLen); //convert

	if (send(socketfd, &cLenSend, sizeof(cLenSend), 0) == -1) {
		fprintf(stderr, "cipher text file send\n");
		exit(1);
	}

						//send key text file size
	int kLenSend = htonl(kLen); //convert

	if (send(socketfd, &kLenSend, sizeof(kLenSend), 0) == -1) {
		fprintf(stderr, "key text file send\n");
		exit(1);
	}

						//Send cipher text
	int len = 0;
	while (len <= cLen) {
		char cipherSend[1024];

						//subset of cipher  to send
		strncpy(cipherSend, &cipherText[len], 1023);

		cipherSend[1024] = '\0';

								 //send
		if (send(socketfd, cipherSend, 1024, 0) == -1) {
			printf("cipher text send\n");
			exit(1);
		}

		len += 1023; //Add len sent to len
	}

					//Send key text
	len = 0;
	while (len <= kLen) {
		char keySend[1024];

						  //subset of key to send
		strncpy(keySend, &keyText[len], 1023);
		keySend[1024] = '\0';

						//send
		if (send(socketfd, &keySend, 1024, 0) == -1) {
			fprintf(stderr, "key text send\n");
			exit(1);
		}

		len += 1023; //add len sent to len
	}

					//allocate memory for plain text
	char *plainText = malloc(sizeof(char) * cLen);
	char buffer[1024];

					//Clear plaintext 
	memset(plainText, '\0', cLen);

					//Receive plain text
	len = 0;
	r = 0;
	while (len < cLen) { //while the whole file has not 
		memset((char *)buffer, '\0', sizeof(buffer));

		r = recv(socketfd, &buffer, 1024, 0);

		if (r == -1) {
			fprintf(stderr, "recv plain text file dec\n");
			exit(1);
		}
		else if (r == 0) {
			if (len < cLen) {
				fprintf(stderr, "recv plain text file <\n");
				exit(1);
			}
		}
		else {
						//concat string
			strncat(plainText, buffer, (r - 1));
		}

		len += (r - 1); //add len received to len
	}

	plainText[cLen - 1] = '\0';
	printf("%s\n", plainText);

					//Free memory
	free(plainText);
	free(keyText);
	free(cipherText);

	return 0;
}