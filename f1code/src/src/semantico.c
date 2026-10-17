/*
 * semantico.c — Validaciones de F1Code
 *
 * Reglas (resuelven los "Pendientes" del contrato sin agregar sintaxis):
 *   1. Toda variable se declara antes de usarse y una sola vez.
 *   2. engine op engine -> engine (la division es entera).
 *      Si interviene un fuel, el resultado es fuel.
 *   3. engine -> fuel se permite; fuel -> engine es error
 *      (Andante no tiene conversion de decimal a numero).
 *   4. Division entre la constante cero es error.
 *   5. finish es valido en cualquier parte; lo que sigue en el mismo
 *      bloque es inalcanzable (advertencia).
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "semantico.h"

static SymTable *table = NULL;
static int errors = 0;

static void semError(int line, const char *fmt, ...) {
    va_list ap;
    fprintf(stderr, "Error semantico en linea %d: ", line);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    errors++;
}

static void semWarning(int line, const char *fmt, ...) {
    va_list ap;
    fprintf(stderr, "Advertencia en linea %d: ", line);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

static VarType combine(VarType a, VarType b) {
    if (a == TYPE_ERROR || b == TYPE_ERROR) return TYPE_ERROR;
    if (a == TYPE_FLOAT || b == TYPE_FLOAT) return TYPE_FLOAT;
    return TYPE_INT;
}

static int isZero(ASTNode *e) {
    return (e->type == ND_INT && e->ival == 0) || (e->type == ND_FLOAT && e->fval == 0.0);
}

static VarType exprType(ASTNode *e) {
    if (!e) return TYPE_ERROR;
    switch (e->type) {
        case ND_INT:
            e->etype = TYPE_INT;
            break;
        case ND_FLOAT:
            e->etype = TYPE_FLOAT;
            break;
        case ND_VAR: {
            SymEntry *s = symtab_lookup(table, e->name);
            if (!s) {
                semError(e->line, "la variable '%s' no ha sido declarada", e->name);
                e->etype = TYPE_ERROR;
            } else {
                e->etype = s->type;
            }
            break;
        }
        case ND_BINOP:
            if (strcmp(e->op, "neg") == 0) {
                e->etype = exprType(e->right);
            } else {
                VarType a = exprType(e->left);
                VarType b = exprType(e->right);
                if (strcmp(e->op, "/") == 0 && isZero(e->right))
                    semError(e->line, "division entre cero");
                e->etype = combine(a, b);
            }
            break;
        case ND_RELOP:
            exprType(e->left);
            exprType(e->right);
            e->etype = TYPE_INT;   /* resultado logico */
            break;
        default:
            e->etype = TYPE_ERROR;
            break;
    }
    return e->etype;
}

static void checkAssignable(VarType dest, VarType src, const char *var, int line) {
    if (dest == TYPE_INT && src == TYPE_FLOAT)
        semError(line, "tipos incompatibles: '%s' es engine y la expresion es fuel "
                       "(no hay conversion de fuel a engine)", var);
}

/* Devuelve 1 si la lista contiene un finish (en cualquier nivel) */
static int checkList(ASTNode *s) {
    int found = 0;
    for (; s; s = s->next) {
        switch (s->type) {
            case ND_DECL: {
                VarType t = exprType(s->expr);    /* antes de insertar: x := x es error */
                SymEntry *prev = symtab_lookup(table, s->name);
                if (prev) {
                    semError(s->line, "la variable '%s' ya fue declarada en la linea %d", s->name, prev->line);
                } else {
                    checkAssignable(s->vtype, t, s->name, s->line);
                    symtab_insert(table, s->name, s->vtype, s->line);
                }
                break;
            }
            case ND_ASSIGN: {
                VarType t = exprType(s->expr);
                SymEntry *v = symtab_lookup(table, s->name);
                if (!v) semError(s->line, "la variable '%s' no ha sido declarada", s->name);
                else    checkAssignable(v->type, t, s->name, s->line);
                break;
            }
            case ND_BOOST: {
                VarType t = exprType(s->expr);
                SymEntry *v = symtab_lookup(table, s->name);
                if (!v) semError(s->line, "la variable '%s' no ha sido declarada", s->name);
                else    checkAssignable(v->type, combine(v->type, t), s->name, s->line);
                break;
            }
            case ND_INPUT:
                if (!symtab_lookup(table, s->name))
                    semError(s->line, "la variable '%s' no ha sido declarada", s->name);
                break;
            case ND_OUTPUT:
                exprType(s->expr);
                break;
            case ND_IF: {
                exprType(s->cond);
                int inThen = checkList(s->thenBody);
                int inElse = checkList(s->elseBody);
                s->hasFinish = inThen || inElse;
                break;
            }
            case ND_WHILE:
                exprType(s->cond);
                s->hasFinish = checkList(s->thenBody);
                break;
            case ND_FINISH:
                s->hasFinish = 1;
                if (s->next)
                    semWarning(s->next->line,
                               "codigo inalcanzable despues de 'finish' (linea %d); no se traducira", s->line);
                break;
            default:
                break;
        }
        if (s->hasFinish) found = 1;
    }
    return found;
}

int semanticCheck(ASTNode *program, SymTable *t) {
    table = t;
    errors = 0;
    if (program) checkList(program->thenBody);
    return errors;
}
