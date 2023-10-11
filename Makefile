all: main.c
        cc -o http_downloader main.c -lssl -lcrypto

clean:
        rm -rf http_downloader
        rm part_*
        rm *.jpg *.gif *.pdf
