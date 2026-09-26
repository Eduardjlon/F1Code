#ifndef AST_H
#define AST_H

typedef enum {
    TYPE_INT,
    TYPE_FLOAT
} VarType;

typedef enum {
    ND_PROGRAM,
    ND_DECL,
    ND_ASSIGN,
    ND_BOOST,
    ND_INPUT,
    ND_OUTPUT,
    ND_IF,
    ND_WHILE,
    ND_FINISH,
    ND_BINOP,
    ND_RELOP,
    ND_INT,
    ND_FLOAT,
    ND_VAR
} NodeType;

typedef struct ASTNode {
    NodeType type;
    int line;

    struct ASTNode *next;      /* siguiente sentencia en una lista */

    VarType vtype;             /* usado por ND_DECL */
    char *name;                /* usado por DECL, ASSIGN, BOOST, INPUT, VAR */
    struct ASTNode *expr;      /* expr de inicializacion / asignacion / boost / output */

    struct ASTNode *cond;      /* usado por IF, WHILE */
    struct ASTNode *thenBody;  /* usado por IF (true), WHILE (cuerpo) */
    struct ASTNode *elseBody;  /* usado por IF (overtake), puede ser NULL */

    char op[4];                /* "+","-","*","/","neg","<",">","<=",">=","==","!=" */
    struct ASTNode *left;
    struct ASTNode *right;

    int ival;
    double fval;
} ASTNode;

ASTNode *newProgram(ASTNode *stmts);
ASTNode *newDecl(VarType vtype, char *name, ASTNode *expr, int line);
ASTNode *newAssign(char *name, ASTNode *expr, int line);
ASTNode *newBoost(char *name, ASTNode *expr, int line);
ASTNode *newInput(char *name, int line);
ASTNode *newOutput(ASTNode *expr, int line);
ASTNode *newIf(ASTNode *cond, ASTNode *thenBody, ASTNode *elseBody, int line);
ASTNode *newWhile(ASTNode *cond, ASTNode *body, int line);
ASTNode *newFinish(int line);
ASTNode *newBinOp(const char *op, ASTNode *left, ASTNode *right, int line);
ASTNode *newRelOp(const char *op, ASTNode *left, ASTNode *right, int line);
ASTNode *newNeg(ASTNode *val, int line);
ASTNode *newIntLit(int v, int line);
ASTNode *newFloatLit(double v, int line);
ASTNode *newVar(char *name, int line);

void printAST(ASTNode *node, int indent);
void freeAST(ASTNode *node);

#endif
