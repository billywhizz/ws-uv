OPT_FAST=-Wall -O3 -I./http-parser -I./libuv/include

CC?=gcc

websock: websock.c libuv/uv.a http-parser/http_parser.o
	$(CC) $(OPT_FAST) -lrt -lm -lpthread -o websock wsparser.c websock.c sha1.c libuv/uv.a http-parser/http_parser.o

libuv/uv.a:
	$(MAKE) -C libuv

http-parser/http_parser.o:
	$(MAKE) -C http-parser http_parser.o

clean:
	rm -f libuv/uv.a
	rm -f http-parser/http_parser.o
	rm -f websock