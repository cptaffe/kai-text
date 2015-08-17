
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lex.h"

void repl() {
	// constants
	enum { kLineLen = 0x100 };

	char *line = calloc(sizeof(char), kLineLen);
	size_t len = kLineLen;
	LexerState *s = calloc(sizeof(LexerState), 1);

	// repl loop
	int depth = 0;
	for (;;) {
		if (depth > 0) {
			printf("%-2d> ", depth);
		} else {
			printf (">>> ");
		}
		size_t r = getline(&line, &len, stdin);
		if (r != EOF) {
			SyntaxTree *t = lex(s, line);
			if (s->state == startState){
				if (t != NULL) pprintSyntaxTree(t);
				depth = 0;
			} else {
				depth = s->nestDepth;
			}
		} else break; // EOF
	}
	printf("\n");

	// cleanup
	free(s->b);
	free(line);
	free(s);
}

int main() {
	repl();
}

#include "lex.c"
