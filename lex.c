
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

// str is from lemit,
// col, line from state,
// type is from argument
Lexeme *makeLexeme(State *s, int type) {
	Lexeme *l = calloc(1, sizeof(Lexeme));
	*l = (Lexeme) {
		.type = type,
		.str = lemit(s),
		.col = s->col + 1,
		.line = s->line
	};
	return l;
}

void pprintLexeme(Lexeme *l) {
	printf("%d:%d; type{%d} '%s'\n", l->line, l->col + 1, l->type, l->str);
}

Tree *makeTree(Lexeme *l) {
	Tree *t = calloc(1, sizeof(Tree));
	*t = (Tree) {
		.l = l
	};
	pprintLexeme(l); // DEBUG
	return t;
}

void pprintTree(Tree *t) {
	if (t->l->type == kIdent) {
		printf("id: '%s' ", t->l->str);
	} else if (t->l->type == kSexpr) {
		printf("(");
		for (int i = 0; i < t->nchild; i++) {
			pprintTree(t->child[i]);
		}
		printf(")");
	} else if (t->l->type == kRawString) {
		printf("rs: %s ", t->l->str);
	}
}

static void appendTree(Tree *t, Tree *c) {
	t->nchild++;
	t->child = realloc(t->child, t->nchild * sizeof(Tree*));
	t->child[t->nchild-1] = c;
}

static TList *makeTList(Tree *t) {
	TList *tl = calloc(1, sizeof(TList));
	*tl = (TList) {
		.t = t
	};
	return tl;
}

static void pushTreeState(State *s, Tree *t) {
	TList *tl = makeTList(t);
	(*tl).next = s->nestTrees;
	s->nestTrees = tl;
}

static Tree *popTreeState(State *s) {
	TList *tl = s->nestTrees;
	s->nestTrees = s->nestTrees->next;
	Tree *t = tl->t;
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

static void *sexprState(State *s);
static void *startState(State *s);

static int isidentc(char c) {
	return isletter(c) || isdigit(c) || c == '_';
}

static void *identState(State *s) {
	char c;
	while ((c = lnext(s)) && isidentc(c)) {}
	lback(s);
	if (!c) return NULL; // shall return soon
	appendTree(s->nestTrees->t, makeTree(makeLexeme(s, kIdent)));
	return sexprState;
}

static void *rawStringState(State *s) {
	char c;
	while ((c = lnext(s)) && c != '`') {}
	if (!c) return NULL; // shall return soon
	appendTree(s->nestTrees->t, makeTree(makeLexeme(s, kRawString)));
	return sexprState;
}

static void *sexprState(State *s) {
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
			Tree *t = makeTree(makeLexeme(s, kSexpr));
			pushTreeState(s, t);
			appendTree(s->nestTrees->t, t);
			return sexprState;
		} else if (c == ')') {
			// ascend a level
			s->nestDepth--;
			if (s->nestDepth < 0) {
				// too many end parens
				ldump(s);
				printf("%d:%d; Too many end parens\n", s->line + 1, s->col - 1);
			} else {
				ldump(s);
				// No lexeme needed, only action.
				// Pop from tree list.
				Tree *t = popTreeState(s);
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

static void *startState(State *s) {
	char c;
	while ((c = lnext(s))) {
		if (c == '(') {
			// opens s-expr
			s->nestDepth++;
			s->root = makeTree(makeLexeme(s, kSexpr));
			pushTreeState(s, s->root);
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

static void l(State *s) {
	if (s->state == NULL) {
		s->state = (void *) startState;
	}
	for (;;) {
		void *f = ((void *(*)(State*))s->state)(s);
		if (f == NULL) break;
		s->state = f;
	}
}

Tree *lex(State *s, char *src) {
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
	l(s);
	if (s->state == startState) {
		return s->root;
	} else {
		return NULL;
	}
}
