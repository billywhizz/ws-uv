#include "wsparser.h"

void ws_init(ws_parser* parser) {
  parser->index = 0;
  parser->state = WS_HEADER;
  parser->maskpos = 0;
  parser->bodypos = 0;
  parser->payload16 = 1;
};

size_t ws_execute(ws_parser* parser, const ws_settings* settings, const char* data, size_t start, size_t end) {
  uint8_t* p = (uint8_t*)data;
  ws_header* current = &parser->header;
  while(start < end) {
    switch(parser->state) {
      case WS_HEADER:
        switch(parser->index) {
          case 0:
            parser->payload16 = 1;
            parser->maskpos = 0;
            current->fin = (p[start] >> 7) & 0x01;
            current->reserved[0] = (p[start] >> 6) & 0x01;
            current->reserved[1] = (p[start] >> 5) & 0x01;
            current->reserved[2] = (p[start] >> 4) & 0x01;
            current->opcode = p[start] & 0x0f;
            parser->payload16 = 0;
            parser->payload64 = 0;
            current->maskkey[0] = current->maskkey[1] = current->maskkey[2] = current->maskkey[3] = 0;
            break;
          case 1:
            current->mask = (p[start] >> 7) & 0x01;
            current->length = p[start] & 0x7f;
            if(current->length == 127) {
              parser->payload16 = 0;
              parser->payload64 = 1;
            }
            else if(current->length == 126) {
              parser->payload16 = 1;
              parser->payload64 = 0;
            }
            else if(current->mask == 0) {
              if(current->length > 0) {
                parser->state = WS_BODY;
                parser->bodypos = 0;
                settings->on_header(parser);
              }
              else {
                settings->on_header(parser);
                settings->on_complete(parser);
                parser->index = 0;
              }
            }
            break;
          case 2:
            if(parser->payload16 == 1) {
              current->length += p[start] << 8;
            }
            else if(parser->payload64 == 1) {
              current->length += (uint64_t)p[start] << 56;
            }
            else {
              current->maskkey[0] = p[start];
            }
            break;
          case 3:
            if(parser->payload16 == 1) {
              current->length += p[start];
              if(current->mask == 0) {
                if(current->length > 0) {
                  parser->state = WS_BODY;
                  parser->bodypos = 0;
                  settings->on_header(parser);
                }
                else {
                  settings->on_header(parser);
                  settings->on_complete(parser);
                  parser->index = 0;
                }
              }
            }
            else if(parser->payload64 == 1) {
              current->length += (uint64_t)p[start] << 48;
            }
            else {
              current->maskkey[1] = p[start];
            }
            break;
          case 4:
            if(parser->payload16 == 1) {
              current->maskkey[0] = p[start];
            }
            else if(parser->payload64 == 1) {
              current->length += (uint64_t)p[start] << 40;
            }
            else {
              current->maskkey[2] = p[start];
            }
            break;
          case 5:
            if(parser->payload16 == 1) {
              current->maskkey[1] = p[start];
            }
            else if(parser->payload64 == 1) {
              current->length += (uint64_t)p[start] << 32;
            }
            else {
              current->maskkey[3] = p[start];
              if(current->length > 0) {
                parser->state = WS_BODY;
                parser->bodypos = 0;
                settings->on_header(parser);
              }
              else {
                settings->on_header(parser);
                settings->on_complete(parser);
                parser->index = 0;
              }
            }
            break;
          case 6:
            if(parser->payload16 == 1) {
              current->maskkey[2] = p[start];
            }
            else if(parser->payload64 == 1) {
              current->length += (uint64_t)p[start] << 24;
            }
            break;
          case 7:
            if(parser->payload16 == 1) {
              current->maskkey[3] = p[start];
              if(current->length > 0) {
                parser->state = WS_BODY;
                parser->bodypos = 0;
                settings->on_header(parser);
              }
              else {
                settings->on_header(parser);
                settings->on_complete(parser);
                parser->index = 0;
              }
            }
            else if(parser->payload64 == 1) {
              current->length += (uint64_t)p[start] << 16;
            }
            break;
          case 8:
            if(parser->payload64 == 1) {
              current->length += (uint64_t)p[start] << 8;
            }
            break;
          case 9:
            if(parser->payload64 == 1) {
              current->length += p[start];
            }
            break;
          case 10:
            if(parser->payload64 == 1) {
              current->maskkey[0] = p[start];
            }
            break;
          case 11:
            if(parser->payload64 == 1) {
              current->maskkey[1] = p[start];
            }
            break;
          case 12:
            if(parser->payload64 == 1) {
              current->maskkey[2] = p[start];
            }
            break;
          case 13:
            if(parser->payload64 == 1) {
              current->maskkey[3] = p[start];
              if(current->length > 0) {
                parser->state = WS_BODY;
                parser->bodypos = 0;
                settings->on_header(parser);
              }
              else {
                settings->on_header(parser);
                settings->on_complete(parser);
                parser->index = 0;
              }
            }
            break;
        }
        parser->index++;
        start++;
        break;
      case WS_BODY:
        if(current->mask == 1) {
          uint64_t toread = current->length - parser->bodypos;
          uint64_t done = toread;
          uint64_t currpos = start;
          if(toread == 0) {
            parser->state = WS_HEADER;
          }
          else if(toread <= (end-start)) {
            while(done--) {
              p[currpos] = p[currpos] ^ current->maskkey[parser->maskpos++];
              if(parser->maskpos == 4) {
                parser->maskpos = 0;
              }
              currpos++;
            }
            settings->on_chunk(parser, data + start, toread);
            settings->on_complete(parser);
            parser->index = 0;
            parser->state = WS_HEADER;
            start = currpos;
          }
          else {
            toread = end - start;
            done = toread;
            while(done--) {
              p[currpos] = p[currpos] ^ current->maskkey[parser->maskpos++];
              if(parser->maskpos == 4) {
                parser->maskpos = 0;
              }
              currpos++;
            }
            settings->on_chunk(parser, data + start, toread);
            start = currpos;
          }
        }
        else {
          uint64_t toread = current->length - parser->bodypos;
          if(toread == 0) {
            parser->state = WS_HEADER;
          }
          else if(toread <= (end-start)) {
            settings->on_chunk(parser, data + start, toread);
            settings->on_complete(parser);
            parser->index = 0;
            parser->state = WS_HEADER;
            start += toread;
          }
          else {
            toread = end - start;
            settings->on_chunk(parser, data + start, toread);
            start += toread;
          }
        }
        break;
      default:
        break;
    }
  }
  return start;
}

void ws_reset(ws_parser* parser) {
  parser->index = 0;
}
