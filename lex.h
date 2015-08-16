
typedef struct {
	enum {
		kIdent,
		kSexpr,
		kNum,
		kFloat,
		kChar,
		kString,
		kRawString
	} type;
	const char *str;
	int col, line;
} Lexeme;

typedef struct Tree {
	Lexeme *l;
	struct Tree **child;
	int nchild;
} Tree;

Tree *makeTree(Lexeme *l);
void pprintTree(Tree *t);

typedef struct TList {
	Tree *t;
	struct TList *next;
} TList;

// state of the program
typedef struct {
	char *b, *s;
	int len;
	int line, lcol, col;
	int nestDepth;
	void *state;
	Tree *root;
	TList *nestTrees;
} State;

Lexeme *makeLexeme(State *s, int type);
void pprintLexeme(Lexeme *l);

Tree *lex(State *s, char *src);
