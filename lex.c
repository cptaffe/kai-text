
typedef int (*CharacterPredicate)(char);

static char currentLexerState(LexerState *s) {
	char c = s->s[s->len];
	if (c == '\0') {
		return EOF;
	} else {
		return c;
	}
}

static void backLexerState(LexerState *s);

static char _actionUpToLexerState(LexerState *s, char (*func)(LexerState *), CharacterPredicate predicate) {
	char c;
	while ((c = func(s)) != EOF && predicate(c)) {}
	return c;
}

// returns EOF on end of string
static char nextLexerState(LexerState *s) {
	s->col++;
	if (s->s == NULL) {
		s->s = s->b;
	}
	char c = currentLexerState(s);
	if (c != EOF) s->len++;
	if (c == '\n') {
		s->lcol = s->col;
		s->col = 0;
		s->line++;
	}
	return c;
}

static char nextUpToLexerState(LexerState *s, CharacterPredicate predicate) {
	return _actionUpToLexerState(s, nextLexerState, predicate);
}

static void backLexerState(LexerState *s) {
	s->col--;
	if (s->len > 0) s->len--;
	if (s->s[s->len] == '\n') {
		s->line--;
		s->col = s->lcol;
	}
}

static char peekLexerState(LexerState *s) {
	char c = nextLexerState(s);
	backLexerState(s);
	return c;
}

static void _dumpLexerState(LexerState *s) {
	s->s = &s->s[s->len];
	s->len = 0;
}

static char ignoreLexerState(LexerState *s) {
	char c = nextLexerState(s);
	_dumpLexerState(s);
	return c;
}

static char ignoreUpToLexerState(LexerState *s, CharacterPredicate predicate) {
	return _actionUpToLexerState(s, nextLexerState, predicate);
}

static char *emitLexerState(LexerState *s) {
	char *str = calloc(sizeof(char), s->len + 1);
	strncpy(str, s->s, s->len);
	s->s = &s->s[s->len];
	s->len = 0;
	return str;
}

// str is from lemit,
// col, line from state,
// type is from argument
Lexeme *makeLexeme(LexerState *s, int type) {
	Lexeme *l = calloc(1, sizeof(Lexeme));
	*l = (Lexeme) {
		.type = type,
		.str = emitLexerState(s),
		.col = s->col + 1,
		.line = s->line
	};
	return l;
}

void pprintLexeme(Lexeme *l) {
	printf("%d:%d; type{%d} '%s'\n", l->line, l->col + 1, l->type, l->str);
}

SyntaxTree *makeSyntaxTree(Lexeme *l) {
	SyntaxTree *t = calloc(1, sizeof(SyntaxTree));
	*t = (SyntaxTree) {
		.l = l
	};
	return t;
}

void _pprintSyntaxTree(SyntaxTree *t) {
	if (t->l->type == kIdent) {
		printf("%s", t->l->str);
	} else if (t->l->type == kSexpr) {
		printf("(");
		for (int i = 0; i < t->nchild; i++) {
			if (i != 0) {
				printf(" ");
			}
			_pprintSyntaxTree(t->child[i]);
		}
		printf(")");
	} else if (t->l->type == kRawString) {
		printf("`%s`", t->l->str);
	} else if (t->l->type == kString) {
		printf("\"%s\"", t->l->str);
	} else if (t->l->type == kComment) {
		printf (";%s\n", t->l->str);
	}
}

void pprintSyntaxTree(SyntaxTree *t) {
	_pprintSyntaxTree(t);
	printf("\n");
}

static void appendSyntaxTree(SyntaxTree *t, SyntaxTree *c) {
	t->nchild++;
	t->child = realloc(t->child, t->nchild * sizeof(SyntaxTree*));
	t->child[t->nchild-1] = c;
}

static SyntaxTreeList *makeSyntaxTreeList(SyntaxTree *t) {
	SyntaxTreeList *tl = calloc(1, sizeof(SyntaxTreeList));
	*tl = (SyntaxTreeList) {
		.t = t
	};
	return tl;
}

static void pushSyntaxTreeLexerState(LexerState *s, SyntaxTree *t) {
	SyntaxTreeList *tl = makeSyntaxTreeList(t);
	(*tl).next = s->nestTrees;
	s->nestTrees = tl;
}

static SyntaxTree *popSyntaxTreeLexerState(LexerState *s) {
	SyntaxTreeList *tl = s->nestTrees;
	s->nestTrees = s->nestTrees->next;
	SyntaxTree *t = tl->t;
	free(tl);
	return t;
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

static int isidentc(char c) {
	return isletter(c) || isdigit(c) || c == '_';
}

static int identCharPredicate(char c) {
	return isidentc(c);
}

void *identState(LexerState *s) {
	char c = nextUpToLexerState(s, identCharPredicate);
	if (c == EOF) return NULL; // shall return soon
	backLexerState(s);
	appendSyntaxTree(s->nestTrees->t, makeSyntaxTree(makeLexeme(s, kIdent)));
	return sexprState;
}

void *metaStringState(LexerState *s, int type, CharacterPredicate predicate) {
	nextUpToLexerState(s, predicate);
	if (currentLexerState(s) == EOF) return NULL; // shall return soon
	backLexerState(s); // get off quote
	appendSyntaxTree(s->nestTrees->t, makeSyntaxTree(makeLexeme(s, type)));
	ignoreLexerState(s); // drop end quote
	return sexprState;
}

static int rawStringCharPredicate(char c) {
	return c != '`';
}

void *rawStringState(LexerState *s) {
	nextUpToLexerState(s, rawStringCharPredicate);
	if (currentLexerState(s) == EOF) return NULL; // shall return soon
	backLexerState(s); // get off quote
	appendSyntaxTree(s->nestTrees->t, makeSyntaxTree(makeLexeme(s, kRawString)));
	ignoreLexerState(s); // drop end quote
	return sexprState;
}

static int stringCharPredicate(char c) {
	return c != '"' && c != '\n';
}

void *stringState(LexerState *s) {
	char c = nextUpToLexerState(s, stringCharPredicate);
	if (c == EOF) return NULL;
	if (c == '\n') {
		// error
		_dumpLexerState(s);
		printf("%d:%d; Strings cannot contain newlines, use a Raw String\n", s->line + 1, s->col - 1);
		return sexprState;
	}
	backLexerState(s); // get off quote
	appendSyntaxTree(s->nestTrees->t, makeSyntaxTree(makeLexeme(s, kString)));
	ignoreLexerState(s); // drop end quote
	return sexprState;
}

static int lineCommentCharPredicate(char c) {
	return c != '\n';
}

void *lineCommentState(LexerState *s) {
	char c = nextUpToLexerState(s, lineCommentCharPredicate);
	if (c == EOF) return NULL;
	backLexerState(s); // back off newline
	appendSyntaxTree(s->nestTrees->t, makeSyntaxTree(makeLexeme(s, kComment)));
	ignoreLexerState(s); // drop newline
	return sexprState;
}

void *sexprState(LexerState *s) {
	char c;
	while ((c = peekLexerState(s)) != EOF) {
		if (isws(c)) {
			// optional ws
			ignoreLexerState(s);
		} else if (isletter(c) || c == '_') {
			// ident
			nextLexerState(s);
			return identState;
		} else if (c == '`') {
			ignoreLexerState(s);
			return rawStringState;
		} else if (c == '"') {
			ignoreLexerState(s);
			return stringState;
		} else if (c == ';') {
			ignoreLexerState(s);
			return lineCommentState;
		} else if (c == '(') {
			nextLexerState(s);
			// descend a level
			s->nestDepth++;
			SyntaxTree *t = makeSyntaxTree(makeLexeme(s, kSexpr));
			appendSyntaxTree(s->nestTrees->t, t);
			pushSyntaxTreeLexerState(s, t);
			return sexprState;
		} else if (c == ')') {
			ignoreLexerState(s);
			// ascend a level
			s->nestDepth--;
			if (s->nestDepth < 0) {
				// too many end parens
				printf("%d:%d; Too many end parens\n", s->line + 1, s->col - 1);
			} else {
				// No lexeme needed, only action.
				// Pop from tree list.
				popSyntaxTreeLexerState(s);
				if (s->nestDepth == 0) {
					return startState;
				} else return sexprState;
			}
		} else if (c == ']') {
			ignoreLexerState(s);
			for (; s->nestDepth > 0; s->nestDepth--) {
				popSyntaxTreeLexerState(s);
			}
			return startState;
		} else {
			// error
			ignoreLexerState(s);
			printf("%d:%d; Looking for atom or ')', found '%c'\n", s->line + 1, s->col - 1, c);
		}
	}
	return NULL;
}

void *startState(LexerState *s) {
	char c;
	while ((c = peekLexerState(s)) != EOF) {
		if (c == '(') {
			nextLexerState(s);
			// opens s-expr
			s->nestDepth++;
			s->root = makeSyntaxTree(makeLexeme(s, kSexpr));
			pushSyntaxTreeLexerState(s, s->root);
			return sexprState;
		} else if (isws(c)) {
			// whitespace, skip
			ignoreLexerState(s);
		} else {
			// error
			ignoreLexerState(s);
			printf("%d:%d; Looking for '(' to start s-expression, found '%c'\n", s->line + 1, s->col - 1, c);
		}
	}
	return NULL;
}

static void l(LexerState *s) {
	if (s->state == NULL) {
		s->state = (void *) startState;
	}
	for (;;) {
		void *f = ((void *(*)(LexerState*))s->state)(s);
		if (f == NULL) break;
		s->state = f;
	}
}

SyntaxTree *lex(LexerState *s, char *src) {
	if (s->b == NULL) {
		s->b = calloc(sizeof(char), strlen(src)+1);
		strcpy(s->b, src);
	} else {
		char *old = s->b;
		// allocate new space
		char *mem = malloc(((strlen(s->b)+1)- (s->s - s->b)) + (strlen(src)+1));
		strcpy(mem, s->s); // copy old unemitted string
		free(s->b); // free old string
		s->b = strcat(mem, src); // concatenate new string
		s->s = s->b;
		backLexerState(s);
	}
	l(s);
	if (s->state == startState) {
		SyntaxTree *t = s->root;
		s->root = NULL;
		return t;
	} else {
		return NULL;
	}
}
