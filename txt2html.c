#include "txt2html.h"

struct node *parsef(FILE *f);
int readq(struct node *queue);

uint8_t opts;

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
	struct node *queue;
	for (a = 1; a < argc; ++a) {
		if (strlen(argv[a]) == 0)
			continue;

		verbose("opening %s\n", argv[a]);
		if ((f = fopen(argv[a], "r")) == NULL) {
			perror("fopen failed, abort");
			continue;
		}

		queue = parsef(f);
		verbose("counted %d nodes\n", readq(queue));
		verbose("closing %s\n", argv[a]);
		if (fclose(f) == EOF) perror("fclose failed");

		while (!queue) {
			if (queue->buf && queue->buf[strlen(queue->buf)+1] == '$')
				free(queue->buf);
			if (queue->next) free(queue->next);
			if (queue->prev) {
				queue = queue->prev;
			} else {
				free(queue);
				break;
			}
		}
		node_create(NULL, 0); // reset node count
	}

	return EXIT_SUCCESS;
}

struct node *parsef(FILE *f)
{
	int n;
	struct node *queue = 0;
	do {
		verbose("reading block...\r");
		char buf[BUFSIZ] = {'\0'};
		n = fread(buf, BUFSIZ-1, sizeof(char), f);
		queue = parse_buf(buf, &queue, opts);
		verbose("                \r");
	} while (n > 0);
	parse_buf(NULL, &queue, opts);
	return queue;
}

int readq(struct node *q)
{
	while (q->prev)
		q = q->prev; // rewind
	int cnt = 0;
	while (q) {
		if (q->buf != NULL)
			printf("%s", q->buf);
		q = q->next;
		++cnt;
	}
	return cnt;
}
