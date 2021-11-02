all:
	${CC} -o txt2html txt2html.c node.c parse.c rules.c
debug:
	${CC} -g -o txt2html-debug txt2html.c node.c parse.c rules.c
