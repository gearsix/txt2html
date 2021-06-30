#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h> // replace with utf8 support

#define ASTLIMIT 10000

// options within <p>
#define OPT_HB 0x01 // newlines as <br/> nodes

// node tags
#define OPEN      0x10
#define CLOSE     0x20
#define H1        0x01
#define H2        0x02
#define P         0x03
#define PRE       0x04
#define LI        0x05
#define BR        0x06
#define OL        0x04
#define UL        0x08

struct node {
	struct node *prev, *next;
	uint8_t type;
	char *buf;
};

int readp(struct node *n, char *txt, int txti);
int isheading(char *txt, int txti);
void writebuf(struct node *n, int c);
struct node *txt2html(char *txt);
struct node *newnode(struct node *prev, uint8_t tag, struct node *n);
struct node *closenode(struct node *n);

const uint8_t opts = OPT_HB;

int main(int argc, char **argv)
{
	char *text = "aaaaaaaaa\n====\n\naaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\naaaaaaa\n\naaaaaa\n---";
	char *html = malloc(4062);

	struct node *n = txt2html(text);
	while(n != NULL) {
		if (n->buf != NULL)
			printf(n->buf);
		n = n->next;
	}

	return EXIT_SUCCESS;
}

struct node *txt2html(char *txt)
{
	struct node *n = malloc(ASTLIMIT * sizeof(struct node));
	const size_t len = strlen(txt);

	unsigned int i = 0;
	while (i != EOF) {
		while (txt[i] == '\n') ++i;

		switch (n->type) {
			case UL+OPEN+LI:
			case OL+OPEN+LI:
				break;
			case H1:
			case H2:
				while (txt[i] != '\n')
					writebuf(n, txt[i++]);
				do { ++i; } while (txt[i] == '-' || txt[i] == '=');
				n = newnode(n, CLOSE+n->type, n+1);
				break;
			case P:
				while (i <= len && isprint(txt[i]))
					writebuf(n, txt[i++]);
				if (txt[i] == '\n' && txt[i+1] == '\n') {
					++i;
					n = closenode(n);
				} else if (txt[i] == '\n' && (opts & OPT_HB)) {
					n = newnode(n, OPEN+BR+CLOSE, n+1);
					n = newnode(n, P, n+1);
				} else if (txt[i] == '\n') {
					writebuf(n, ' ');
				} else {
					writebuf(n, txt[i]);
				}
				++i;
//				i = readp(n, txt, i);
				break;
			default:
				if (isalnum(txt[i]) && txt[i+1] == '.' && txt[i+2] == ' ') {
					n = newnode(n, OPEN+OL, n+1);
					n = newnode(n, OL+OPEN+LI, n+1);
					i += 2;
				} else if ((txt[i] == '*' || txt[i] == '-') && txt[i+1] == ' ') {
					n = newnode(n, OPEN+UL, n+1);
					n = newnode(n, UL+OPEN+LI, n+1);
					i++;
				} else if (txt[i] == '\t' && isprint(txt[i+1])) {
					n = newnode(n, OPEN+PRE, n+1);
					n = newnode(n, PRE, n+1);
					++i;
				} else if (isprint(txt[i])) {
					switch (isheading(txt, i)) {
						case H1:
							n = newnode(n, OPEN+H1, n+1);
							n = newnode(n, H1, n+1);
							break;
						case H2:
							n = newnode(n, OPEN+H2, n+1);
							n = newnode(n, H2, n+1);
							break;
						default:
							n = newnode(n, OPEN+P, n+1);
							n = newnode(n, P, n+1);
							break;
					}
					writebuf(n, txt[i++]);
				}
				break;
		}

		if (i >= len) {
			i = EOF;
			n = closenode(n);
			continue;
		}
	}

	while (n->prev != NULL)
		n = n->prev;
	return n;
}

struct node *closenode(struct node *n)
{
	switch (n->type) {
		case UL+OPEN+LI:
			n = newnode(n, CLOSE+UL+LI, n+1);
			n = newnode(n, CLOSE+UL, n+1);
			break;
		case UL:
			n = newnode(n, CLOSE+UL, n+1);
			break;
		case OL+OPEN+LI:
			n = newnode(n, CLOSE+OL+LI, n+1);
			n = newnode(n, CLOSE+OL, n+1);
			break;
		case OL:
			n = newnode(n, CLOSE+OL, n+1);
			break;
		case P:
		case OPEN+P:
			n = newnode(n, CLOSE+P, n+1);
			break;
		default:
			break;
	}
	return n;
}

// malloc node `n` and set it's values according to `tag`.
// a pointer to `n` is returned.
struct node *newnode(struct node *prev, uint8_t tag, struct node *n)
{
	if (n == NULL)
		perror("newnode, n cannot be NULL");
	if (prev != NULL)
		prev->next = n;
	n->prev = prev;
	n->type = tag;
	switch(tag) {
		case OPEN+H1:
			n->buf = malloc(5);
			strncat(n->buf, "<h1>", 5);
			break;
		case OPEN+H2:
			n->buf = malloc(5);
			strncat(n->buf, "<h2>", 5);
			break;
		case OPEN+P:
			n->buf = malloc(4);
			strncat(n->buf, "<p>", 4);
			break;
		case CLOSE+H1:
			if (prev != NULL && prev->type == H1)
				writebuf(prev, EOF);
			n->buf = malloc(7);
			strncat(n->buf, "</h1>\n", 7);
			break;
		case CLOSE+H2:
			if (prev != NULL && prev->type == H2)
				writebuf(prev, EOF);
			n->buf = malloc(7);
			strncat(n->buf, "</h2>\n", 7);
			break;
		case CLOSE+P:
			if (prev != NULL && prev->type == P)
				writebuf(prev, EOF);
			n->buf = malloc(6);
			strncat(n->buf, "</p>\n", 6);
			break;
		case OPEN+BR+CLOSE:
			if (prev != NULL && prev->type == P)
				writebuf(prev, EOF);
			n->buf = malloc(7);
			strncat(n->buf, "<br/>\n", 7);
			break;
		default:
			break;
	}
	return n;
}

// writebuf has an internal static buffer (`buf`) that it writes `c` to.
// if `c=EOF` or `buf` reaches `BUFSIZ`, then `buf` it's written to n->buf.
// `n->buf` will only be allocated used memory.
void writebuf(struct node *n, int c)
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
			break;
		case '\t':
			strncat(buf, "&emsp;", 7);
			len += 6;
			break;
		default:
			strncat(buf, (char *)&c, 2);
			len += 1;
			break;
	}
}

int isheading(char *txt, int i)
{
	const int len = strlen(txt);
	while (txt[i++] != '\n' && i < len); // skip to next line
	if (txt[i] == '=' && txt[i+1] == '=' && txt[i+2] == '=')
		return H1;
	if (txt[i] == '-' && txt[i+1] == '-' && txt[i+2] == '-')
		return H2;
	else
		return 0;
}

//
int readp(struct node *n, char *txt, int i)
{
	if (n == NULL || txt == NULL)
		return 0;

	const int len = strlen(txt);
	if (i > len) {
		n = closenode(n);
		return EOF;
	}

	if (txt[i] == '\n') {
		if (i+1 <= len && txt[i+1] == '\n') {
			n = closenode(n);
			++i;
		} else if (opts & OPT_HB) {
			n = newnode(n, OPEN+BR+CLOSE, n+1);
			n = newnode(n, P, n+1);
		} else {
			writebuf(n, ' ');
		}
		++i;
	} else while (i < len && txt[i] != '\n')
		writebuf(n, txt[i++]);

	if (i == len) {
		n = closenode(n);
		i = EOF;
	}

	return i;
}

