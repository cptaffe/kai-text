
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lex.h"

void repl() {
	// constants
	enum { kLineLen = 0x100 };
	const char *kPromt = ">>> ";
	const char *kInSexprPromt = "  | ";
	const char *kInRawStringPrompt = "  | ";
	const char *kIdkWherePrompt = "??? ";

	// variables
	const char *prompt = kPromt;
	char *line = calloc(sizeof(char), kLineLen);
	size_t len = kLineLen;
	LexerState *s = calloc(sizeof(LexerState), 1);

	// repl loop
	for (;;) {
		printf("%s", prompt);
		size_t r = getline(&line, &len, stdin);
		if (r != EOF) {
			SyntaxTree *t = lex(s, line);
			if (s->state == sexprState) {
				prompt = kInSexprPromt;
			} else if (s->state == rawStringState) {
				prompt = kInRawStringPrompt;
			} else if (s->state == startState){
				if (t != NULL) {
					printf(": ");
					pprintSyntaxTree(t);
					printf("\n");
				} else {
					printf("null syntax tree (!)\n");
				}
				prompt = kPromt;
			} else {
				prompt = kIdkWherePrompt;
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
