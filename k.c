
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	enum {
		kIdent,
		kSOpen,
		kSClose,
		kNum,
		kFloat,
		kChar,
		kString,
		kRawString
	} type;
	const char *str;
	int col, line;
} Lexeme;

void pprintLexeme(Lexeme *l) {
	printf("%d:%d; type{%d} '%s'\n", l->line, l->col + 1, l->type, l->str);
}

typedef struct Tree {
	Lexeme *l;
	struct Tree **child;
	int nchild;
} Tree;

// state of the program
typedef struct {
	char *b, *s;
	int len;
	int line, lcol, col;
	int nestDepth;
	void *state;
	Tree *root;
} State;

// returns 0 on end of string
static char lnext(State *s) {
	s->col++;
	if (s->s == NULL) {
		s->s = s->b;
	}
	char c = s->s[s->len];
	s->len++;
	if (c == '\n') {
		s->lcol = s->col;
		s->col = 0;
		s->line++;
	}
	return c;
}

static void lback(State *s) {
	s->col--;
	s->len--;
	if (s->s[s->len] == '\n') {
		s->line--;
		s->col = s->lcol;
	}
}

static char lpeek(State *s) {
	char c = lnext(s);
	lback(s);
	return c;
}

static void ldump(State *s) {
	s->s = &s->s[s->len];
	s->len = 0;
}

static char *lemit(State *s) {
	char *str = calloc(sizeof(char), s->len + 1);
	strncpy(str, s->s, s->len);
	s->s = &s->s[s->len];
	s->len = 0;
	return str;
}

static int isws(char c) {
	return c == ' ' || c == '\t' || c == '\n';
}

static int isletter(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static int isdigit(char c) {
	return c >= '0' && c <= '9';
}

void *sexprState(State *s);
void *startState(State *s);

static int isidentc(char c) {
	return isletter(c) || isdigit(c) || c == '_';
}

void *identState(State *s) {
	char c;
	while ((c = lnext(s)) && isidentc(c)) {}
	lback(s);
	if (!c) return NULL; // shall return soon
	Lexeme l = {
		.type = kIdent,
		.str = lemit(s),
		.line = s->line + 1,
		.col = s->col - 1
	};
	pprintLexeme(&l);
	free((void *) l.str);
	return sexprState;
}

void *rawStringState(State *s) {
	char c;
	while ((c = lnext(s)) && c != '`') {}
	if (!c) return NULL; // shall return soon
	Lexeme l = {
		.type = kRawString,
		.str = lemit(s),
		.line = s->line + 1,
		.col = s->col - 1
	};
	pprintLexeme(&l);
	free((void *) l.str);
	return sexprState;
}

void *sexprState(State *s) {
	char c;
	while ((c = lnext(s))) {
		if (isws(c)) {
			// optional ws
			ldump(s);
		} else if (isletter(c) || c == '_') {
			// ident
			return identState;
		} else if (c == '`') {
			return rawStringState;
		} else if (c == '(') {
			// descend a level
			s->nestDepth++;
			Lexeme l = {
				.type = kSOpen,
				.str = lemit(s),
				.line = s->line + 1,
				.col = s->col - 1
			};
			pprintLexeme(&l);
			free((void *) l.str);
			return sexprState;
		} else if (c == ')') {
			// ascend a level
			s->nestDepth--;
			if (s->nestDepth < 0) {
				// too many end parens
				ldump(s);
				printf("%d:%d; Too many end parens\n", s->line + 1, s->col - 1);
			} else {
				Lexeme l = {
					.type = kSClose,
					.str = lemit(s),
					.line = s->line + 1,
					.col = s->col - 1
				};
				pprintLexeme(&l);
				free((void *) l.str);
				if (s->nestDepth == 0) {
					return startState;
				} else return sexprState;
			}
		} else {
			// error
			ldump(s);
			printf("%d:%d; Looking for atom or ')', found '%c'\n", s->line + 1, s->col - 1, c);
		}
	}
	return NULL;
}

void *startState(State *s) {
	char c;
	while ((c = lnext(s))) {
		if (c == '(') {
			// opens s-expr
			s->nestDepth++;
			Lexeme l = {
				.type = kSOpen,
				.str = lemit(s),
				.line = s->line + 1,
				.col = s->col - 1
			};
			pprintLexeme(&l);
			free((void *) l.str);
			// switch state
			return sexprState;
		} else if (isws(c)) {
			// whitespace, skip
			ldump(s);
		} else {
			// error
			ldump(s);
			printf("%d:%d; Looking for '(' to start s-expression, found '%c'\n", s->line + 1, s->col - 1, c);
		}
	}
	return NULL;
}

void lex(State *s) {
	if (s->state == NULL) {
		s->state = (void *) startState;
	}
	for (;;) {
		void *f = ((void *(*)(State*))s->state)(s);
		if (f == NULL) break;
		s->state = f;
	}
}

Tree *eval(State *s, char *src) {
	if (s->b == NULL) {
		s->b = calloc(sizeof(char), strlen(src) + 1);
		strcpy(s->b, src);
	} else {
		char *old = s->b;
		// allocate new space
		char *mem = malloc(((strlen(s->b)+1)- (s->s - s->b)) + (strlen(src)+1));
		strcpy(mem, s->s); // copy old unemitted string
		free(s->b); // free old string
		s->b = strcat(mem, src); // concatenate new string
		s->s = s->b;
		lback(s);
	}
	lex(s);
	return NULL;
}

void repl() {
	// constants
	enum { kLineLen = 0x100 };
	const char *const kPromt = ">>> ";
	const char *const kMorePromt = "  | ";

	// variables
	const char *const *prompt = &kPromt;
	char *line = calloc(sizeof(char), kLineLen);
	size_t len = kLineLen;
	State *s = calloc(sizeof(State), 1);

	// repl loop
	for (;;) {
		printf("%s", *prompt);
		size_t r = getline(&line, &len, stdin);
		if (r != EOF) {
			Tree *t = eval(s, line);
			if (s->state != startState) {
				prompt = &kMorePromt;
			} else {
				prompt = &kPromt;
				free(t);
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
