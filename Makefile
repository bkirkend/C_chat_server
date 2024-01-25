build:
	gcc server_new.c -o server
	gcc client.c -o client
thread:
	gcc server_thread.c -pthread -o server
	gcc client.c -pthread -o client
clean:
	rm -f server
	rm -f client
	rm -f *~
