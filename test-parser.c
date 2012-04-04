#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "wsparser.h"

static void xxdprint(const char *buff, uint64_t offset, uint64_t size) {
	uint64_t i = offset;
  uint64_t j;
	while(i < size)
	{
		uint8_t val = buff[i] & 0xff;
		if(i%8==0 && i > 0) {
			printf(" ");
		}
		if(i%16==0) {
			if(i>0) {
				printf(" ");
				for(j=i-16;j<i;j++) {
					int vv = buff[j];
					if(vv > 0x20 && vv < 0x7e) {
						printf("%c", vv);
					}
					else {
						printf(".");
					}
				}
				printf("\n");
			}
			printf("%.8lu: ", i);
		}
		printf("%.2x ", val);
		i++;
	}
	if(size%16!=0) {
		for(j=0; j<(16-(size%16)); j++) {
			printf("   ");
		}
		printf("  ");
		if(size%16<=8) printf(" ");
		for(j=i-(size%16);j<i;j++) {
			int vv = buff[j];
			if(vv > 0x20 && vv < 0x7e) {
				printf("%c", vv);
			}
			else {
				printf(".");
			}
		}
	}
	printf("\n");
}

static char login[] = {
  130,154,18,158,90,80,18,158,
  90,80,19,158,83,49,120,241,
  50,62,97,234,53,62,18,150,
  42,49,97,237,45,63,96,250
};

static char order[] = {
  130,144,219,34,181,28,218,34,
  181,28,217,34,181,28,218,34,
  180,29,127,37,101,28
};

static char logout[] = {
  136,128,209,189,157,197
};

void dump_header(ws_header* header) {
  fprintf(stderr, "fin: %i\n", header->fin);
  fprintf(stderr, "rsv0: %i\n", header->reserved[0]);
  fprintf(stderr, "rsv1: %i\n", header->reserved[1]);
  fprintf(stderr, "rsv2: %i\n", header->reserved[2]);
  fprintf(stderr, "opcode: %i\n", header->opcode);
  fprintf(stderr, "length: %i\n", header->length);
  fprintf(stderr, "masking: %i\n", header->mask);
  fprintf(stderr, "mask0: %i\n", header->maskkey[0]);
  fprintf(stderr, "mask1: %i\n", header->maskkey[1]);
  fprintf(stderr, "mask2: %i\n", header->maskkey[2]);
  fprintf(stderr, "mask3: %i\n", header->maskkey[3]);
}

int on_header(ws_parser* p) {
  fprintf(stderr, "on_header: %li\n", p->index);
  dump_header(&p->header);
  return 0;
}

int on_chunk(ws_parser* p, const char* at, size_t len) {
  fprintf(stderr, "on_chunk: %li\t%li\n", p->index, len);
  xxdprint(at, 0, len);
  return 0;
}

int on_complete(ws_parser* p) {
  fprintf(stderr, "on_complete: %li\n", p->index);
  return 0;
}

int main(int argc, char **argv) {
  ws_parser* parser = malloc(sizeof(ws_parser));
  ws_settings* settings = malloc(sizeof(ws_settings));
  settings->on_header = &on_header;
  settings->on_chunk = &on_chunk;
  settings->on_complete = &on_complete;
  ws_init(parser);
  fprintf(stderr, "login\n");
  ws_execute(parser, settings, login, 0, sizeof(login));
  fprintf(stderr, "order\n");
  ws_execute(parser, settings, order, 0, sizeof(order));
  fprintf(stderr, "logout\n");
  ws_execute(parser, settings, logout, 0, sizeof(logout));
  return 0;
}