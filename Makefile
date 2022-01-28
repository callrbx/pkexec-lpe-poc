all:
	gcc -o poc poc.c -lpthread 

payload:
	gcc -o payload_test payload.c