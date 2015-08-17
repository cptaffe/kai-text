
typedef struct {
	enum {
		kIdent,
		kSexpr,
		kNum,
		kFloat,
		kChar,
		kString,
		kRawString,
		kComment
	} type;
	const char *str;
	int col, line;
} Lexeme;

typedef struct SyntaxTree {
	Lexeme *l;
	struct SyntaxTree **child;
	int nchild;
} SyntaxTree;

SyntaxTree *makeSyntaxTree(Lexeme *l);
void freeSyntaxTree(SyntaxTree *t);
void pprintSyntaxTree(SyntaxTree *t);

typedef struct SyntaxTreeList {
	SyntaxTree *t;
	struct SyntaxTreeList *next;
} SyntaxTreeList;

// state of the program
typedef struct {
	char *b, *s;
	int len;
	int line, lcol, col;
	int nestDepth;
	void *state;
	SyntaxTree *root;
	SyntaxTreeList *nestTrees;
} LexerState;

Lexeme *makeLexeme(LexerState *s, int type);
void freeLexeme(Lexeme *l);
void pprintLexeme(Lexeme *l);

void *startState(LexerState *s);
void *sexprState(LexerState *s);
void *rawStringState(LexerState *s);
void *identState(LexerState *s);

SyntaxTree *lex(LexerState *s, char *src);
