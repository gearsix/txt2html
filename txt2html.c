#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h> // replace with utf8 support

#define ASTLIMIT 10000

// config
#define OPT_HB 0x01

// node tags
#define MAXNODE   0x41
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

void writebuf(struct node *n, int c);
struct node *txt2html(const char *txt);
struct node *newnode(struct node *prev, uint8_t tag, struct node *n);
struct node *closenode(struct node *n);

const uint8_t opts = OPT_HB;

int main(int argc, char **argv)
{
	const char *text = "aaaaaaaaa\n====\n\naaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\naaaaaaa\n\naaaaaa\n---";
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
	struct node *n = malloc(ASTLIMIT * sizeof(struct node));
	int c;
	unsigned int i = 0, j;
	const size_t len = strlen(txt);

	while (c != EOF) {
		c = (i++ < len) ? txt[i] : EOF;

		if (c == EOF) {
			n = closenode(n);
			continue;
		}

		switch (n->type) {
			case UL+OPEN+LI:
			case OL+OPEN+LI:
				break;
			case OPEN+P:
			case P:
				if (c == '\n' && txt[i+1] == '\n') {
					++i;
					n = newnode(n, CLOSE+P, n+1);
				} else if (c == '\n') {
					if (opts & OPT_HB) {
						n = newnode(n, OPEN+BR+CLOSE, n+1);
						n = newnode(n, P, n+1);
					} else writebuf(n, ' ');
				} else writebuf(n, c);
				break;
			default:
				if (isalnum(c) && txt[i+1] == '.' && txt[i+2] == ' ') {
					n = newnode(n, OPEN+OL, n+1);
					n = newnode(n, OL+OPEN+LI, n+1);
					i += 2;
				} else if ((c == '*' || c == '-') && txt[i+1] == ' ') {
					n = newnode(n, OPEN+UL, n+1);
					n = newnode(n, UL+OPEN+LI, n+1);
					i++;
				} else if (c == '\t') {
					if (isprint(txt[i+1])) {
						n = newnode(n, OPEN+PRE, n+1);
						n = newnode(n, PRE, n+1);
						++i;
					}
				} else if (isprint(c)) {
					j = i;
					while (txt[++j] != '\n' && j < len); ++j; // skip to next
					if (txt[j] == '=' && txt[j+1] == '=' && txt[j+2] == '=') {
						n = newnode(n, OPEN+H1, n+1);
						n = newnode(n, H1, n+1);
					} else if (txt[j] == '-' && txt[j+1] == '-' && txt[j+2] == '-') {
						n = newnode(n, OPEN+H2, n+1);
						n = newnode(n, H2, n+1);
					}
					if (n->type == H1 || n->type == H2) {
						while (i < j-1)
							writebuf(n, txt[i++]);
						while (txt[++i] != '\n'); // skip "==="/"---" line
						n = newnode(n, CLOSE+n->type, n+1);
					} else {
						n = newnode(n, OPEN+P, n+1);
						n = newnode(n, P, n+1);
						writebuf(n, c);
					}
				}
				break;
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
