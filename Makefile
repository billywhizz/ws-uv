OPT_FAST=-Wall -O3 -I./http-parser -I./libuv/include

CC?=gcc

websock: websock.c libuv/.libs/libuv.a http-parser/http_parser.o
	$(CC) $(OPT_FAST) -lrt -lm -lpthread -o websock wsparser.c websock.c sha1.c libuv/.libs/libuv.a http-parser/http_parser.o

httpd: httpd.c libuv/.libs/libuv.a http-parser/http_parser.o
	$(CC) $(OPT_FAST) -lrt -lm -lpthread -o httpd httpd.c libuv/.libs/libuv.a http-parser/http_parser.o

socket: app.c libuv/.libs/libuv.a
	$(CC) $(OPT_FAST) -lrt -lm -lpthread -o socket socket.c app.c libuv/.libs/libuv.a

libuv/.libs/libuv.a:
	$(MAKE) -C libuv

http-parser/http_parser.o:
	$(MAKE) -C http-parser http_parser.o

clean:
	rm -f websock
	rm -f socket
	rm -f httpd