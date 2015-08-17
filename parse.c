
typedef struct SymbolEntry {
	const char *key;
	SyntaxTree *l;
	struct SymbolEntry *next, *prev;
} SymbolEntry;

typedef struct {
	SymbolEntry **values;
	size_t size;
} SymbolTable;

typedef struct {
	SymbolTable *symbols;
} ParseState;

// Keywords
typedef enum {
	kDefine, // defines a variable
	kList,
	kQuote,
	kMaxKeyword
} Keyword;

const char *kKeywords[] = {
	[kDefine] = "def",
	[kList] = "list",
	[kQuote] = "quote"
};

SymbolTable *makeSymbolTable(size_t size) {
	SymbolTable *t = calloc(1, sizeof(SymbolTable));
	*t = (SymbolTable) {
		.values = calloc(size, sizeof(SymbolEntry*)),
		.size = size
	};
	return t;
}

void freeSymbolTable(SymbolTable *t) {
	free(t->values);
	free(t);
}

static int hash(const char *str) {
	int h = 0;
	for (int i = 0; str[i]; i++) {
		h *= 31;
		h += str[i];
	}
	return h;
}

SymbolEntry *makeSymbolEntry(const char *key, SyntaxTree *l) {
	SymbolEntry *e = calloc(1, sizeof(SymbolEntry));
	*e = (SymbolEntry) {
		.key = key,
		.l = l
	};
	return e;
}

void insertSymbolTable(SymbolTable *t, const char *key, SyntaxTree *l) {
	SymbolEntry *e = makeSymbolEntry(key, l);
	int index = hash(key) % t->size;
	e->next = t->values[index];
	if (t->values[index]) t->values[index]->prev = e;
	t->values[index] = e;
}

static SymbolEntry *getSymbolEntry(SymbolEntry *e, const char *key) {
	if (e == NULL) return NULL;
	if (strcmp(e->key, key) == 0) {
		return e;
	} else return getSymbolEntry(e->next, key);
}

SyntaxTree *getSymbolTable(SymbolTable *t, const char *key) {
	return getSymbolEntry(t->values[hash(key) % t->size], key)->l;
}

SyntaxTree *removeSymbolTable(SymbolTable *t, const char *key) {
	SymbolEntry *e = getSymbolEntry(t->values[hash(key) % t->size], key);
	// remove from list
	if (e->prev) e->prev->next = e->next;
	if (e->next) e->next->prev = e->prev;
	// free container
	SyntaxTree *l = e->l;
	free(e);
	return l;
}

void pprintSymbolEntry(SymbolEntry *e) {
	if (!e) return;
	printf("%s: ", e->key);
	pprintSyntaxTree(e->l);
	pprintSymbolEntry(e->next);
}

void pprintSymbolTable(SymbolTable *t) {
	for (size_t i = 0; i < t->size; i++) {
		if (t->values[i]) pprintSymbolEntry(t->values[i]);
	}
}

ParseState *makeParseState() {
	enum {
		kTableSize = 0x100
	};
	ParseState *s = calloc(1, sizeof(ParseState));
	*s = (ParseState) {
		.symbols = makeSymbolTable(kTableSize)
	};
	return s;
}

void freeParseState(ParseState *s) {
	freeSymbolTable(s->symbols);
	free(s);
}

// Evaluate builtins
SyntaxTree *doKeyword(ParseState *s, SyntaxTree *t, Keyword keyword) {
	if (keyword == kDefine) {
		// Define
		// Define(Identifier, Generic)
		if (t->nchild != 3) {
			printf("%d:%d; Define takes 2 args not %d\n", t->l->line, t->l->col, t->nchild - 1);
		} else {
			if (t->child[1]->l->type != kIdent) {
				printf("%d:%d; Define, arg %d not Identifier\n", t->l->line, t->l->col, 1);
			} else {
				insertSymbolTable(s->symbols, t->child[1]->l->str, t->child[2]);
				freeSyntaxTree(t->child[0]);
				freeSyntaxTreeNonRecursive(t);
				return NULL;
			}
		}
	} else if (keyword == kList) {
		if (t->nchild < 2) {
			printf("%d:%d; List takes 1+ args not %d\n", t->l->line, t->l->col, t->nchild - 1);
		} else {
			freeSyntaxTree(t->child[0]);
			size_t size =  (t->nchild-1) * sizeof(SyntaxTree*);
			memmove(&t->child[0], &t->child[1], size);
			t->child = realloc(t->child, size);
			t->nchild--;
			return t;
		}
	} else if (keyword == kQuote) {
		if (t->nchild != 2) {
			printf("%d:%d; Quote takes 1 args not %d\n", t->l->line, t->l->col, t->nchild - 1);
		} else {
			freeSyntaxTree(t->child[0]);
			SyntaxTree *tr = t->child[1];
			freeSyntaxTreeNonRecursive(t);
			return tr;
		}
	}
	freeSyntaxTree(t);
	return NULL;
}

SyntaxTree *parse(ParseState *s, SyntaxTree *t) {
	if (t->l->type == kSexpr) {
		// in s-expression
		for (int j = 0; j < kMaxKeyword; j++) {
			if (strcmp(t->child[0]->l->str, kKeywords[j]) == 0) {
				return doKeyword(s, t, j);
			}
		}
		printf("%d:%d; %s is undefined!\n", t->l->line, t->l->col, t->child[0]->l->str);
		freeSyntaxTree(t);
		return NULL;
	}
	return t;
}
