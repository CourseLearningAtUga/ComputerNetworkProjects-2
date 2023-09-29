all: http_downloader
	./http_downloader

http_downloader: main.o 
	cc -o http_downloader main.o

main.o: main.c
	cc -c main.c -o main.o 

clean:
	rm -rf http_downloader main.o

