CC=gcc 
all: server client 
server: server.o link_list_impl.o 
	$(CC) -pthread server.o link_list_impl.o -o server
client: client.o 
clean: 	
	rm -f *.o server client 
	
link_list_impl.o: link_list_impl.c link_list_impl.h
	$(CC) -c link_list_impl.c

server.o: server.c link_list_impl.c message.h link_list_impl.h
	$(CC) -c server.c link_list_impl.c -lpthread 

client.o: client.c message.h
	$(CC) -c client.c


