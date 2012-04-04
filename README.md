libuv WebSocket
===============

this will eventually be a websocket server library and set of tools for 
building fast and small websocket servers in c using the wonderful libuv. the
goal is to be very small and very fast and leave most of the decisions to
the user of the library

Proposed API
-----

  * sha1.h - function to calculate SHA1 hash of a buffer
  * wsparser.h - functions for parsing websocket frames
  * websock.h - functions for running websocket servers  

Building
-----

    git clone git@github.com:billywhizz/ws-uv.git
    cd ws-uv
    git submodule init
    git submodule update
    make -j 4

Running
-----

the websock server will listen on port 80. see main in websocket.c for setup

    ./websock

Features
-----

  * supports parsing of all websocket control and data frames from a stream
  * evented io handled by libuv
  * does not support HTTP beyond what is required for performing a websocket
    handshake
  * should be pretty straightforward to build on windows/mac. if anyone
    feels like contributing patches for this, please feel free
  * no plans to add SSL support. if you want to do ssl then put something in
    front of this
  * zero dependencies. libuv and http-parser are statically linked into the 
    websocket library
  * at startup only consumes 600KB of RAM on linux
  * very small overhead per connection (need to measure)
  * handshake performed using joyent http-parser which is very low overhead
    and fast
  * only supports http://tools.ietf.org/html/rfc6455. is not backwards 
    compatible
  * only low level support - hands off messages to user. does not react to
    control messages or perform any logic around fragmented messages
  * does not do any utf8 decoding for text frames. chunks of frame payloads
    are returned as pointers to char buffers whether they are binary or text
  
Future
-----

  * support pausing of parser
  * !!!!handle overflows on headers and header sizes!!!!
  * allow user definable limits on headers and header sizes
  * allow configuration of port, tcp backlog, nodelay, keepalive, connection
    limits, bandwidth throttling, 
  * daemonization
  * cluster support by receiving socket on stdin/pipe
  * support quitting parser execution with return values from callbacks
  * no plans to add SSL support. if you want to do ssl then put something in
    front of this
  * websocket extension support
  * higher level api to deal with control frames and fragmented messages
  * possibly add support for static file serving and http 
    user modules
  * SPDY support? do we really want to do ssl?
  * possibly add fastcgi and memcached protocol support
  * thread safety
  * node.js scripts for benchmarking
  * tests
  * benchmarks
  * improve the make file to build and run tests
  * windows/mac support
  * clean and small websocket api for servers and clients
  * easy to hook into server for running websocket applications
  * user supplied c scripts (llvm)
  * defined interface for libraries that can be loaded at runtime
  * user scripts are written in c and can be compiled and run on the fly
    by the server (llvm)
  

Usage
-----

Parser:

see test-parser.c for usage of the parser

    // websocket message struct
    struct ws_header {
      uint8_t fin;
      uint8_t reserved[3];
      uint8_t opcode;
      uint32_t length;
      uint8_t mask;
      uint8_t maskkey[4];
    };

    // callbacks for parser
    
    // this is called when a websocket frame header has been parsed
    int on_header(ws_parser* p);
    // this is called for each chunk of the body of a frame received. it
    // is up to the callee to manage assembly of message bodies
    int on_chunk(ws_parser* p, const char* at, size_t len);
    // called when a frame is complete
    int on_complete(ws_parser* p);

    // allocate a new parser
    ws_parser* parser = malloc(sizeof(ws_parser));
    // allocate a settings structure
    ws_settings* settings = malloc(sizeof(ws_settings));
    // set callbacks for parser
    settings->on_header = &on_header;
    settings->on_chunk = &on_chunk;
    settings->on_complete = &on_complete;
    // initialise the parser. you should call this if you are reusing a parser
    // across multiple connections
    ws_init(parser);
    // execute the parser with 
    ws_execute(parser, settings, buffer, 0, sizeof(login));