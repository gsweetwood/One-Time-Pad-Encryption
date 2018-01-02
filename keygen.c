//Author: Garret Sweetwood
//CS 344
//Program 4
//Date 08/11/17


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char* argv[]) {

	int i, randNum, randChar;

	srand(time(NULL));

	int keyLength = atoi(argv[1]);  //This takes the string argument, converts it to a usable int

	for (i = 0; i < keyLength; i++) {
		randNum = rand() % 27;

		if (randNum < 26) {
			randChar = randNum + 65;
			printf("%c", randChar);
		}
		else {
			printf(" ");
		}
	}
	printf("\n");

	return 0;
}