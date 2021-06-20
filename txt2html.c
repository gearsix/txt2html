#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h> // replace with utf8 support

// config
#define OPT_HB 0x01

// node tags
#define OPEN      0x10
#define CLOSE     0x20
#define P         0x01
#define BR        0x02

struct node {
	struct node *prev, *next;
	uint8_t type;
	char *buf;
};

void writeP(struct node *n, int c);
struct node *txt2html(const char *txt);
struct node *next(struct node *prev, uint8_t tag, struct node *n);

const uint8_t opts = OPT_HB;

int main(int argc, char **argv)
{
	const char *text = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\naaaaaaa\n\naaaaaa";
	char *html = malloc(4062);

	struct node *n = txt2html(text);
	while(n != NULL) {
		if (n->buf != NULL)
			printf(n->buf);
		n = n->next;
	}
	puts("");

	return EXIT_SUCCESS;
}

struct node *txt2html(const char *txt)
{
	struct node *n = malloc(50 * sizeof(struct node));

	int c;
	unsigned int i = 0;
	const size_t len = strlen(txt);

	while (c != EOF) {
		c = (i < len) ? txt[i] : EOF;

		switch (n->type) {
			case P:
				if (c == EOF)
					n = next(n, CLOSE+P, n+1);
				else if (c == '\n' && txt[i+1] == '\n') {
					++i;
					n = next(n, CLOSE+P, n+1);
				} else if (c == '\n') {
					if (opts & OPT_HB) {
						n = next(n, OPEN+BR+CLOSE, n+1);
						n = next(n, P, n+1);
					} else writeP(n, ' ');
				} else writeP(n, c);
				break;
			case 0:
			default:
				if (isprint(c) || c == '\t') {
					if (n->prev == NULL || n->type == CLOSE+P) {
						n = next(n, OPEN+P, n+1);
						n = next(n, P, n+1);
					}
					writeP(n, c);
				}
				break;
		}

		++i;
	}

	while (n->prev != NULL)
		n = n->prev;
	return n;
}

struct node *next(struct node *prev, uint8_t tag, struct node *n)
{
	prev->next = n;
	n->prev = prev;
	n->type = tag;
	switch(tag) {
		case OPEN+P:
			n->buf = malloc(4);
			strncat(n->buf, "<p>", 4);
			break;
		case CLOSE+P:
			if (prev->type == P)
				writeP(prev, EOF);
			n->buf = malloc(5);
			strncat(n->buf, "</p>", 5);
			break;
		case OPEN+BR+CLOSE:
			if (prev->type == P)
				writeP(prev, EOF);
			n->buf = malloc(6);
			strncat(n->buf, "<br/>", 6);
			break;
		default:
			break;
	}
	return n;
}

void writeP(struct node *n, int c)
{
	static int pag = 0;
	static int len = 0;
	static char buf[BUFSIZ];

	if (len+1 == BUFSIZ || c == EOF) {
		n->buf = (pag == 0) ? malloc(len) : realloc(n->buf, strlen(n->buf) + len);
		memmove(n->buf, buf, len);
		++pag;
		len = 0;
		memset(buf, '\0', BUFSIZ); 
	}

	switch (c) {
		case EOF:
			pag = 0;
			return;
		case '\t':
			strncat(buf, "&emsp;", 7);
			len += 6;
			return;
		default:
			strncat(buf, (char *)&c, 2);
			len += 1;
			return;
	}
}
