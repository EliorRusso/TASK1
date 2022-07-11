
server:
	gcc tg.c -o server
client:
	gcc clien.c -o client
clean: rm *.o Server Client