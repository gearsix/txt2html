#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h> // replace with utf8 support
#include <assert.h>

#define MEMLIMIT 100000000

#define OPT_V  0x10 // print verbose logs
#define OPT_NM 0x20 // no memory limit
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

// rules for detecting tags
#define RULE_CLOSE_OL(str) (*str == '\n' && (*(str+1) == '\n' || *(str+1) == '\0'))
#define LEN_CLOSE_OL       2
#define RULE_OPEN_OLI(str) (isalnum(*str) && *(str+1) == '.' && *(str+2) == ' ')
#define LEN_OPEN_OLI       3
#define RULE_CLOSE_UL(str) (*str == '\n' && (*(str+1) == '\n' || *(str+1) == '\0'))
#define LEN_CLOSE_UL       2
#define RULE_OPEN_ULI(str) ((*str == '-' || *str == '*') && *(str+1) == ' ')
#define LEN_OPEN_ULI       2
#define RULE_OPEN_PRE(str) (*str == '\t' && isprint(*(str+1)))
#define LEN_OPEN_PRE       1

struct node {
	struct node *prev, *next;
	uint8_t type;
	char *buf;
};

struct node *parsef(FILE **f);
int readast(struct node *ast);
struct node *buf2ast(const char *buf, struct node *ast);
struct node *newnode(struct node *prev, const int type);
size_t nextnode(const char *str, struct node **n);
size_t parseh(const char *str, struct node **n);
size_t parseoli(const char *str, struct node **n);
size_t parseuli(const char *str, struct node **n);
size_t parsep(const char *str, struct node **n);
size_t parsetxt(const char *str, struct node **n);
void writebuf(struct node *n, int c);
int isheading(const char *txt);

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

void parseargs(int argc, char **argv)
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
			} else if (strcmp(argv[a], "-nm") == 0) {
				opts |= OPT_NM;
			}
			
			argv[a][0] = '\0';
		}
	}
}

int main(int argc, char **argv)
{
	parseargs(argc, argv);
	verbose("printing verbose logs\n");

	int a;
	FILE *f;
	struct node *ast;
	for (a = 1; a < argc; ++a) {
		if (strlen(argv[a]) == 0)
			continue;

		verbose("opening %s\n", argv[a]);
		if ((f = fopen(argv[a], "r")) == NULL) {
			perror("fopen failed, abort");
			continue;
		}

		ast = parsef(&f);
		verbose("counted %d nodes\n", readast(ast));
		verbose("closing %s\n", argv[a]);
		if (fclose(f) == EOF) perror("fclose failed");

		while (ast != NULL) {
			if (ast->buf && ast->buf[strlen(ast->buf)+1] == '$')
				free(ast->buf);
			if (ast->next) free(ast->next);
			if (ast->prev) {
				ast = ast->prev;
			} else {
				free(ast);
				break;
			}
		}
		newnode(NULL, 0); // reset node count
	}

	return EXIT_SUCCESS;
}

struct node *parsef(FILE **f)
{
	int n;
	struct node *ast = NULL;
	do {
		verbose("reading block...\r");
		char buf[BUFSIZ] = {'\0'};
		n = fread(buf, BUFSIZ-1, sizeof(char), *f);
		ast = buf2ast(buf, ast);
		verbose("                \r");
	} while (n > 0);
	buf2ast(NULL, ast);
	return ast;
}

int readast(struct node *ast)
{
	while (ast->prev != NULL) ast = ast->prev; // rewind
	int cnt = 0;
	while (ast != NULL) {
		if (ast->buf != NULL)
			printf("%s", ast->buf);
		ast = ast->next;
		++cnt;
	}
	return cnt;
}

struct node *buf2ast(const char *buf, struct node *ast)
{
	struct node *n = ast;
	size_t i = 0;
	size_t len = (buf != NULL) ? strlen(buf) : 0;

	if (buf == NULL && ast != NULL)
		n = newnode(n, CLOSE+n->type);

	while (i < len && buf != NULL) {
		while (buf[i] == '\n') ++i;
		if (n == NULL)
			i += nextnode(&buf[i], &n);
		switch (n->type) {
			case H1:
			case H2:    i += parseh(&buf[i], &n);   break;
			case P:     i += parsep(&buf[i], &n);   break;
			//case PRE:   i += parsetxt(&buf[i], &n); break; // TODO
			case OL+LI: i += parseoli(&buf[i], &n); break;
			case UL+LI: i += parseuli(&buf[i], &n); break;
			default:    i += nextnode(&buf[i], &n); break;
		}
	}
	return n;
}

struct node *newnode(struct node *prev, const int type)
{
	static size_t cnt;

	if (prev == NULL && type == 0) {
		cnt = 0;
		return NULL;
	}

	if (!(opts & OPT_NM) && (sizeof(struct node) * cnt > MEMLIMIT)) {
		printf("txt2html: reached memory limit\n");
		abort();
	}

	struct node *n = calloc(1, sizeof(struct node));
	n->type = type;

	if (prev != NULL) {
		n->prev = prev;
		prev->next = n;
		if (type & CLOSE) writebuf(prev, EOF);
	}

	switch(type) {
		case OPEN+H1:       n->buf = "<h1>\0";   break;
		case OPEN+H2:       n->buf = "<h2>\0";   break;
		case OPEN+P:        n->buf = "<p>\0";    break;
		case OPEN+OL:       n->buf = "<ol>\n\0"; break;
		case OPEN+UL:       n->buf = "<ul>\n\0"; break;
		case OL+OPEN+LI:
		case UL+OPEN+LI:    n->buf = "  <li>\0"; break;
		
		case CLOSE+H1:      n->buf = "</h1>\n\0"; break;
		case CLOSE+H2:      n->buf = "</h2>\n\0"; break;
		case CLOSE+P:       n->buf = "</p>\n\0"; break;
		case CLOSE+OL:      n->buf = "</ol>\n\0"; break;
		case CLOSE+UL:      n->buf = "</ul>\n\0"; break;
		case UL+CLOSE+LI:
		case OL+CLOSE+LI:   n->buf = "</li>\n\0"; break;
	
		case OPEN+BR+CLOSE: n->buf = "<br/>\n\0"; break;
		
		default:
			--cnt;
			break;
	}

	++cnt;
	return n;
}

size_t nextnode(const char *str, struct node **n)
{
	size_t ret = 0;
	if (RULE_OPEN_OLI(&str[ret])) {
		ret += LEN_OPEN_OLI;
		*n = newnode(*n, OPEN+OL);
		*n = newnode(*n, OPEN+OL+LI);
		*n = newnode(*n, OL+LI);
	} else if (RULE_OPEN_ULI(&str[ret])) {
		ret += LEN_OPEN_ULI;
		*n = newnode(*n, OPEN+UL);
		*n = newnode(*n, OPEN+UL+LI);
		*n = newnode(*n, UL+LI);
	} else if (RULE_OPEN_PRE(&str[ret])) {
		ret += LEN_OPEN_PRE;
		*n = newnode(*n, OPEN+PRE);
		*n = newnode(*n, PRE);
	} else if (isprint(str[ret])) {
		switch (isheading(&str[ret])) {
			case H1:
				*n = newnode(*n, OPEN+H1);
				*n = newnode(*n, H1);
				break;
			case H2:
				*n = newnode(*n, OPEN+H2);
				*n = newnode(*n, H2);
				break;
			default:
				*n = newnode(*n, OPEN+P);
				*n = newnode(*n, P);
				break;
		}
	}
	return ret;
}

size_t parseh(const char *str, struct node **n)
{
	size_t ret = 0;
	while(str[ret] != '\n' && str[ret] != '\0') writebuf(*n, str[ret++]);
	do { ++ret; } while (str[ret] == '-' || str[ret] == '=');
	*n = newnode(*n, CLOSE+(*n)->type);
	return ret;
}

size_t parsep(const char *str, struct node **n)
{
	size_t i = parsetxt(str, n);
	if (str[i] == '\n' && str[i+1] == '\n') *n = newnode(*n, CLOSE+P);
	return i;
}

size_t parseoli(const char *str, struct node **n)
{
	size_t ret = 0;
	size_t len = strlen(str);

	while (ret < len) {
		ret += parsetxt(&str[ret], n);
		*n = newnode(*n, CLOSE+OL+LI);

		if (str[ret] == '\0' || RULE_CLOSE_OL(&str[ret])) {
			ret += LEN_CLOSE_OL;
			*n = newnode(*n, CLOSE+OL);
			break;
		} else if (RULE_OPEN_OLI(&str[ret])) {
			ret += LEN_OPEN_OLI;
			*n = newnode(*n, OPEN+OL+LI);
			*n = newnode(*n, OL+LI);
		}
	}

	return ret;
}

size_t parseuli(const char *str, struct node **n)
{
	size_t ret = 0;
	size_t len = strlen(str);

	while (ret < len) {
		ret += parsetxt(&str[ret], n);
		*n = newnode(*n, CLOSE+UL+LI);

		if (str[ret] == '\0' || RULE_CLOSE_UL(&str[ret])) {
			ret += LEN_CLOSE_UL;
			*n = newnode(*n, CLOSE+UL);
			break;
		} else if (RULE_OPEN_ULI(&str[ret])) {
			ret += LEN_OPEN_ULI;
			*n = newnode(*n, OPEN+UL+LI);
			*n = newnode(*n, UL+LI);
		} else break;
	}

	return ret;
}

size_t parsetxt(const char *str, struct node **n)
{
	size_t ret = 0;

	while (str[ret] != '\0' && (isprint(str[ret]) || str[ret] == '\n')) {
		if (str[ret] == '\n' && str[ret+1] == '\n')
			break;
		else if (str[ret] == '\n') {
			if (((*n)->type & OL+LI && RULE_OPEN_OLI(&str[ret+1])) ||
				((*n)->type & UL+LI && RULE_OPEN_ULI(&str[ret+1]))) {
				++ret;
				break;
			}

			if ((*n)->type == PRE && str[ret+1] == '\t') {
				ret += 2;
			} else if (opts & OPT_BR) {
				*n = newnode(*n, OPEN+BR+CLOSE);
				*n = newnode(*n, (*n)->prev->type);
			} else {
				writebuf(*n, str[ret]);
			}
		} else {
			writebuf(*n, str[ret]);
		}
		++ret;
	}

	return ret;
}

// writebuf has an internal static buffer (`buf`) that it writes `c` to.
// if `c == EOF` or `buf` reaches `BUFSIZ`, then `buf` it's written to n->buf.
// `n->buf` will only be allocated required memory.
void writebuf(struct node *n, int c)
{
	assert(n != NULL);
	static int pg = 0;
	static int len = 0;
	static char buf[BUFSIZ+1];

	if (len+2 == BUFSIZ || c == EOF && len > 0) {
		if (c == EOF) {
			buf[len++] = '\0';
			buf[len++] = '$'; // signal malloc'd (not assigned)
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

int isheading(const char *txt)
{
	assert(txt != NULL);
	while (*txt++ != '\n' && *txt != '\0'); // skip to next line
	if (*txt == '\0' || strlen(txt) < 3)
		return 0;
	if (*txt == '=' && *(txt+1) == '=' && *(txt+2) == '=')
		return H1;
	if (*txt == '-' && *(txt+1) == '-' && *(txt+2) == '-')
		return H2;
	else
		return 0;
}
