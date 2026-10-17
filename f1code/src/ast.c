#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

static ASTNode *allocNode(NodeType type, int line) {
    ASTNode *n = (ASTNode *)calloc(1, sizeof(ASTNode));
    n->type = type;
    n->line = line;
    return n;
}

ASTNode *newProgram(ASTNode *stmts) {
    ASTNode *n = allocNode(ND_PROGRAM, 0);
    n->thenBody = stmts; /* reutilizamos thenBody como "lista de sentencias" del programa */
    return n;
}

ASTNode *newDecl(VarType vtype, char *name, ASTNode *expr, int line) {
    ASTNode *n = allocNode(ND_DECL, line);
    n->vtype = vtype;
    n->name = name;
    n->expr = expr;
    return n;
}

ASTNode *newAssign(char *name, ASTNode *expr, int line) {
    ASTNode *n = allocNode(ND_ASSIGN, line);
    n->name = name;
    n->expr = expr;
    return n;
}

ASTNode *newBoost(char *name, ASTNode *expr, int line) {
    ASTNode *n = allocNode(ND_BOOST, line);
    n->name = name;
    n->expr = expr;
    return n;
}

ASTNode *newInput(char *name, int line) {
    ASTNode *n = allocNode(ND_INPUT, line);
    n->name = name;
    return n;
}

ASTNode *newOutput(ASTNode *expr, int line) {
    ASTNode *n = allocNode(ND_OUTPUT, line);
    n->expr = expr;
    return n;
}

ASTNode *newIf(ASTNode *cond, ASTNode *thenBody, ASTNode *elseBody, int line) {
    ASTNode *n = allocNode(ND_IF, line);
    n->cond = cond;
    n->thenBody = thenBody;
    n->elseBody = elseBody;
    return n;
}

ASTNode *newWhile(ASTNode *cond, ASTNode *body, int line) {
    ASTNode *n = allocNode(ND_WHILE, line);
    n->cond = cond;
    n->thenBody = body;
    return n;
}

ASTNode *newFinish(int line) {
    return allocNode(ND_FINISH, line);
}

ASTNode *newBinOp(const char *op, ASTNode *left, ASTNode *right, int line) {
    ASTNode *n = allocNode(ND_BINOP, line);
    snprintf(n->op, sizeof(n->op), "%s", op);
    n->left = left;
    n->right = right;
    return n;
}

ASTNode *newRelOp(const char *op, ASTNode *left, ASTNode *right, int line) {
    ASTNode *n = allocNode(ND_RELOP, line);
    snprintf(n->op, sizeof(n->op), "%s", op);
    n->left = left;
    n->right = right;
    return n;
}

ASTNode *newNeg(ASTNode *val, int line) {
    ASTNode *n = allocNode(ND_BINOP, line);
    snprintf(n->op, sizeof(n->op), "neg");
    n->right = val;
    return n;
}

ASTNode *newIntLit(int v, int line) {
    ASTNode *n = allocNode(ND_INT, line);
    n->ival = v;
    return n;
}

ASTNode *newFloatLit(double v, int line) {
    ASTNode *n = allocNode(ND_FLOAT, line);
    n->fval = v;
    return n;
}

ASTNode *newVar(char *name, int line) {
    ASTNode *n = allocNode(ND_VAR, line);
    n->name = name;
    return n;
}

static void indentPrint(int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
}

static void printExpr(ASTNode *n) {
    if (!n) { printf("?"); return; }
    switch (n->type) {
        case ND_INT:   printf("%d", n->ival); break;
        case ND_FLOAT: printf("%g", n->fval); break;
        case ND_VAR:   printf("%s", n->name); break;
        case ND_BINOP:
            if (strcmp(n->op, "neg") == 0) {
                printf("-(");
                printExpr(n->right);
                printf(")");
            } else {
                printf("(");
                printExpr(n->left);
                printf(" %s ", n->op);
                printExpr(n->right);
                printf(")");
            }
            break;
        case ND_RELOP:
            printf("(");
            printExpr(n->left);
            printf(" %s ", n->op);
            printExpr(n->right);
            printf(")");
            break;
        default: printf("<expr>");
    }
}

static void printList(ASTNode *list, int indent) {
    for (ASTNode *s = list; s != NULL; s = s->next) {
        printAST(s, indent);
    }
}

void printAST(ASTNode *node, int indent) {
    if (!node) return;
    indentPrint(indent);
    switch (node->type) {
        case ND_PROGRAM:
            printf("Program\n");
            printList(node->thenBody, indent + 1);
            break;
        case ND_DECL:
            printf("Decl (%s) %s := ", node->vtype == TYPE_INT ? "engine" : "fuel", node->name);
            printExpr(node->expr);
            printf("   [linea %d]\n", node->line);
            break;
        case ND_ASSIGN:
            printf("Assign %s := ", node->name);
            printExpr(node->expr);
            printf("   [linea %d]\n", node->line);
            break;
        case ND_BOOST:
            printf("Boost %s += ", node->name);
            printExpr(node->expr);
            printf("   [linea %d]\n", node->line);
            break;
        case ND_INPUT:
            printf("Input %s   [linea %d]\n", node->name, node->line);
            break;
        case ND_OUTPUT:
            printf("Output ");
            printExpr(node->expr);
            printf("   [linea %d]\n", node->line);
            break;
        case ND_IF:
            printf("If ");
            printExpr(node->cond);
            printf("   [linea %d]\n", node->line);
            indentPrint(indent + 1); printf("Then:\n");
            printList(node->thenBody, indent + 2);
            if (node->elseBody) {
                indentPrint(indent + 1); printf("Overtake:\n");
                printList(node->elseBody, indent + 2);
            }
            break;
        case ND_WHILE:
            printf("While ");
            printExpr(node->cond);
            printf("   [linea %d]\n", node->line);
            printList(node->thenBody, indent + 1);
            break;
        case ND_FINISH:
            printf("Finish   [linea %d]\n", node->line);
            break;
        default:
            printf("<nodo desconocido>\n");
    }
}

void freeAST(ASTNode *node) {
    if (!node) return;
    freeAST(node->next);
    freeAST(node->expr);
    freeAST(node->cond);
    freeAST(node->thenBody);
    freeAST(node->elseBody);
    freeAST(node->left);
    freeAST(node->right);
    free(node->name);
    free(node);
}
