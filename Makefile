all:
	gcc -Wall -Werror -o poc poc.c -lpthread 

payload:
	gcc -o payload_test payload.c