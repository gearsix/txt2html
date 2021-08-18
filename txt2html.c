#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h> // replace with utf8 support
#include <assert.h>

#define ASTLIMIT 10000

#define OPT_V  0x10 // print verbose logs
#define OPT_BR 0x01 // newlines as <br/> nodes within <p> (not ' ')

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
struct node *txt2html(char *txt, struct node *n);
struct node *newnode(struct node *prev, struct node *next, uint8_t tag);
struct node *closenode(struct node *n);

uint8_t opts = 0;

void help()
{
	puts("usage: txt2html [OPTIONS] FILE...");
	puts("");
	puts("Convert content in txt files to html.");
	puts("");
	puts("FILE...   A list of filepaths that point to files to be converted to HTML");
	puts("");
	puts("OPTIONS");
	puts("-br           Treat newlines within paragraphs as line breaks.");
	puts("-v            Print verbose logs during runtime");
	puts("-h, --help    Print this message");
}

void verbose(const char *fmt, ...)
{
	if (opts & OPT_V) {
		printf("txt2html: ");
		va_list args;
		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
	}
}

int main(int argc, char **argv)
{
	int i = 0, a = argc-1;
	for (; a > 0; --a) {
		if (argv[a] == NULL)
			continue;
		if (argv[a][i] == '-') {
			if (strcmp(argv[a], "-h") == 0 ||
				strcmp(argv[a], "--help") == 0) {
				help();
				exit(0);
			} else if (strcmp(argv[a], "-br") == 0) {
				opts |= OPT_BR;
			} else if (strcmp(argv[a], "-v") == 0) {
				opts |= OPT_V;
			}
			
			argv[a][0] = '\0';
		}
	}
	
	verbose("printing verbose logs\n");

	char c;
	struct node *ast = calloc(ASTLIMIT, sizeof(struct node));
	for (a = 1, c = EOF; a < argc; ++a) {
		if (strlen(argv[a]) == 0)
			continue;

		verbose("opening %s\n", argv[a]);
		FILE *f = fopen(argv[a], "r");
		
		struct node *n = ast;
		do {
			char buf[BUFSIZ] = {'\0'};
			verbose("reading block...\r");
			fread(buf, BUFSIZ-1, sizeof(char), f);
			n = txt2html(buf, n);
			c = fgetc(f);
			if (c != EOF && ungetc(c, f) == EOF)
				perror("txt2html: ungetc() fail");
		} while (c != EOF);
		
		n = ast;
		int ncount = 0;
		while (n != NULL) {
			if (n->buf != NULL) {
				printf("%s", n->buf);
				if (n->buf[strlen(n->buf)+1] == '&')
					free(n->buf);
			}
			n = n->next;
			++ncount;
		}
		verbose("counted %d nodes\n", ncount);
		verbose("closing %s\n", argv[a]);
		fclose(f);
	}
	free(ast);

	return EXIT_SUCCESS;
}

struct node *txt2html(char *txt, struct node *n)
{
	assert(n != NULL);
	if (txt == NULL || txt[0] == EOF)
		goto EXIT;
	const size_t len = strlen(txt);

	unsigned int i = 0;
	while (i != EOF) {
		while (txt[i] == '\n') ++i;

		switch (n->type) {
			case UL+OPEN+LI:
				n = newnode(n, n+1, UL+LI);
			case UL+LI:
				while (i <= len && isprint(txt[i]))
					writebuf(n, txt[i++]);
				if (txt[i] == '\n' && (txt[i+1] == '\n' || txt[i+1] == '\0')) {
					++i;
					n = closenode(n);
					n = newnode(n, n+1, CLOSE+UL);
					if (txt[i+1] == '\0') goto EXIT;
				} else if (txt[i] == '\n' && (txt[i+1] == '-' || txt[i+1] == '*') && txt[i+2] == ' ') {
					i += 2;
					n = closenode(n);
					n = newnode(n, n+1, UL+OPEN+LI);
					n = newnode(n, n+1, UL+LI);
				} else if (txt[i] == '\n' && (opts & OPT_BR)) {
					n = newnode(n, n+1, OPEN+BR+CLOSE);
					n = newnode(n, n+1, UL+OPEN+LI);
				} else if (txt[i] == '\n') {
					writebuf(n, ' ');
				} else {
					writebuf(n, txt[i]);
				}
				++i;
				break;
			case OL+OPEN+LI:
				n = newnode(n, n+1, OL+LI);
			case OL+LI:
				while (i <= len && isprint(txt[i]))
					writebuf(n, txt[i++]);
				if (txt[i] == '\n' && (txt[i+1] == '\n' || txt[i+1] == '\0')) {
					++i;
					n = closenode(n);
					n = newnode(n, n+1, CLOSE+OL);
					if (txt[i+1] == '\0') goto EXIT;
				} else if (txt[i] == '\n' && (isalnum(txt[i+1]) && txt[i+2] == '.')) {
					i += 2;
					n = closenode(n);
					n = newnode(n, n+1, OL+OPEN+LI);
					n = newnode(n, n+1, OL+LI);
				} else if (txt[i] == '\n' && (opts & OPT_BR)) {
					n = newnode(n, n+1, OPEN+BR+CLOSE);
					n = newnode(n, n+1, OL+LI);
				} else if (txt[i] == '\n') {
					writebuf(n, ' ');
				} else {
					writebuf(n, txt[i]);
				}
				++i;
				break;
			case H1:
			case H2:
				while (txt[i] != '\n')
					writebuf(n, txt[i++]);
				do { ++i; } while (txt[i] == '-' || txt[i] == '=');
				n = newnode(n, n+1, CLOSE+n->type);
				break;
			case P:
				while (i <= len && isprint(txt[i]))
					writebuf(n, txt[i++]);
				if (txt[i] == '\n' && txt[i+1] == '\n') {
					++i;
					n = closenode(n);
				} else if (txt[i] == '\n' && (opts & OPT_BR)) {
					n = newnode(n, n+1, OPEN+BR+CLOSE);
					n = newnode(n, n+1, P);
				} else if (txt[i] == '\n') {
					writebuf(n, ' ');
				} else {
					writebuf(n, txt[i]);
				}
				++i;
				break;
			default:
				if (isalnum(txt[i]) && txt[i+1] == '.') {
					n = newnode(n, n+1, OPEN+OL);
					n = newnode(n, n+1, OL+OPEN+LI);
					i += 3;
				} else if ((txt[i] == '*' || txt[i] == '-') && txt[i+1] == ' ') {
					n = newnode(n, n+1, OPEN+UL);
					n = newnode(n, n+1, UL+OPEN+LI);
					i += 2;
				} else if (txt[i] == '\t' && isprint(txt[i+1])) {
					n = newnode(n, n+1, OPEN+PRE);
					n = newnode(n, n+1, PRE);
					++i;
				} else if (isprint(txt[i])) {
					switch (isheading(txt, i)) {
						case H1:
							n = newnode(n, n+1, OPEN+H1);
							n = newnode(n, n+1, H1);
							break;
						case H2:
							n = newnode(n, n+1, OPEN+H2);
							n = newnode(n, n+1, H2);
							break;
						default:
							n = newnode(n, n+1, OPEN+P);
							n = newnode(n, n+1, P);
							break;
					}
					writebuf(n, txt[i++]);
				}
				break;
		}

		if (i >= len || txt[i] == '\0') {
EXIT:
			i = EOF;
			n = closenode(n);
			continue;
		}
	}

	return n;
}

struct node *closenode(struct node *n)
{
	assert(n != NULL);
	switch (n->type) {
		case UL+OPEN+LI:
		case UL+LI:
			n = newnode(n, n+1, CLOSE+UL+LI);
			break;
		case OPEN+UL:
		case CLOSE+UL+LI:
			n = newnode(n, n+1, CLOSE+UL);
			break;
		case OL+OPEN+LI:
		case OL+LI:
			n = newnode(n, n+1, CLOSE+OL+LI);
			break;
		case OPEN+OL:
		case CLOSE+OL+LI:
			n = newnode(n, n+1, CLOSE+OL);
			break;
		case OPEN+P:
		case P:
			n = newnode(n, n+1, CLOSE+P);
			break;
		default:
			break;
	}
	return n;
}

// malloc node `n` and set it's values according to `tag`.
// a pointer to `n` is returned.
struct node *newnode(struct node *prev, struct node *next, uint8_t tag)
{
	assert(next != NULL && prev != NULL);
	prev->next = next;
	next->prev = prev;
	next->type = tag;
	switch(tag) {
		case OPEN+H1:
			next->buf = "<h1>\0";
			break;
		case OPEN+H2:
			next->buf = "<h2>\0";
			break;
		case OPEN+P:
			next->buf = "<p>\0";
			break;
		case OPEN+OL:
			next->buf = "<ol>\n\0";
			break;
		case OPEN+UL:
			next->buf = "<ul>\n\0";
			break;
		case OL+OPEN+LI:
		case UL+OPEN+LI:
			next->buf = "  <li>\0";
			break;
		case CLOSE+H1:
			if (prev != NULL && prev->type == H1)
				writebuf(prev, EOF);
			next->buf = "</h1>\n\0";
			break;
		case CLOSE+H2:
			if (prev != NULL && prev->type == H2)
				writebuf(prev, EOF);
			next->buf = "</h2>\n\0";
			break;
		case CLOSE+P:
			if (prev != NULL && prev->type == P)
				writebuf(prev, EOF);
			next->buf = "</p>\n\0";
			break;
		case CLOSE+OL:
			next->buf = "</ol>\n\0";
			break;
		case CLOSE+UL:
			next->buf = "</ul>\n\0";
			break;
		case UL+CLOSE+LI:
		case OL+CLOSE+LI:
			if (prev != NULL && (prev->type & OPEN+LI) != 0)
				writebuf(prev, EOF);
			next->buf = "</li>\n\0";
			break;
		case OPEN+BR+CLOSE:
			if (prev != NULL && prev->type == P)
				writebuf(prev, EOF);
			next->buf = "<br/>\n\0";
			break;
		default:
			break;
	}
	return next;
}

// writebuf has an internal static buffer (`buf`) that it writes `c` to.
// if `c=EOF` or `buf` reaches `BUFSIZ`, then `buf` it's written to n->buf.
// `n->buf` will only be allocated used memory.
void writebuf(struct node *n, int c)
{
	assert(n != NULL);
	static int pg = 0;
	static int len = 0;
	static char buf[BUFSIZ+1];

	if (len+2 == BUFSIZ || c == EOF) {
		if (c == EOF) {
			buf[len++] = '\0';
			buf[len++] = '&'; // signal malloc
		}
		n->buf = (pg == 0) ? malloc(len) : realloc(n->buf, strlen(n->buf) + len);
		memmove(n->buf, buf, len);
		++pg;
		len = 0;
		memset(buf, '\0', BUFSIZ); 
	}

	switch (c) {
		case EOF:
			pg = 0;
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

