
typedef struct SymbolEntry {
	const char *key;
	SyntaxTree *l;
	struct SymbolEntry *next, *prev;
} SymbolEntry;

typedef struct {
	SymbolEntry **values;
	size_t size;
} SymbolTable;

// Keywords
typedef enum {
	kDefine, // defines a variable
	kList,
	kQuote,
	kMaxKeyword
} Keyword;

const char *keywords[] = {
	[kDefine] = "let", // (let ((a 2) (x 3)) ...)
};

SyntaxTree *evalLet(SyntaxTree *t);

// Evaluator functions for each keyword
const void *evals[] = {
	[kDefine] = evalLet
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

SyntaxTree *evalLet(SyntaxTree *t) {
	return NULL;
}

SyntaxTree *_eval(SymbolTable *table, SyntaxTree *t) {
	if (t->l->type == kSexpr) {
		// (
		if (t->nchild > 0) {
			// first param should be ident
			if (t->child->l->type == kIdent) {
				for (int i = 0; i < kMaxKeyword; i++) {
					if (strcmp(t->child->l->str, keywords[i]) == 0) {
						return ((SyntaxTree *(*)(SyntaxTree *)) evals[i])(t);
					}
				}
			}
		}
	}
	freeSyntaxTree(t);
	return makeSyntaxTree(makeErrorLexeme(t->l->line, t->l->col, "Unevaluatable atom"));
}

SyntaxTree *eval(SyntaxTree *t) {
	SymbolTable *table = makeSymbolTable(0x100);
	t = _eval(table, t);
	freeSymbolTable(table);
	return t;
}
