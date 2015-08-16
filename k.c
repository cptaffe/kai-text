
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lex.h"

void repl() {
	// constants
	enum { kLineLen = 0x100 };
	const char *const kPromt = ">>> ";
	const char *const kMorePromt = "  | ";

	// variables
	const char *const *prompt = &kPromt;
	char *line = calloc(sizeof(char), kLineLen);
	size_t len = kLineLen;
	LexerState *s = calloc(sizeof(LexerState), 1);

	// repl loop
	for (;;) {
		printf("%s", *prompt);
		size_t r = getline(&line, &len, stdin);
		if (r != EOF) {
			SyntaxTree *t = lex(s, line);
			if (t == NULL) {
				prompt = &kMorePromt;
			} else {
				pprintSyntaxTree(t);
				printf("\n");
				prompt = &kPromt;
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
