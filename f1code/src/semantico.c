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
 *   6. garage chassis x := e; (declaracion generica) toma el tipo de e:
 *      engine si e es entera, fuel si es real. Desde ahi el tipo de x
 *      queda fijo y aplican las reglas 2 y 3 como a cualquier variable.
 *   7. stint (init; cond; paso) { cuerpo }: init, cond, cuerpo y paso se
 *      validan como sentencias normales; la variable de control va a la
 *      tabla global (sigue visible despues del ciclo).
 *   8. podium e; (retorno) termina el programa entregando e: para el
 *      control de flujo se comporta igual que finish (regla 5).
 *   9. Texto (garage pilot, literales 'Alex' o "Alex"): se asigna, se lee
 *      con pitstop, se imprime con radio y se compara solo con == y !=.
 *      No se opera con + - * / ni se mezcla con engine/fuel.
 *  10. Compatibilidad con el traductor del Grupo 2: una variable declarada
 *      engine/fuel/chassis con un valor literal por defecto (:= 0) que
 *      despues recibe un texto se toma como pilot (advertencia). Asi no se
 *      pierde ninguna instruccion del programa original.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantico.h"

static SymTable *table = NULL;
static int errors = 0;
static int podiumNum = 0, podiumText = 0;   /* tipos devueltos por podium */

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
        case ND_TEXT:
            e->etype = TYPE_TEXT;
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
                if (e->etype == TYPE_TEXT) {
                    semError(e->line, "no se puede aplicar '-' a un texto");
                    e->etype = TYPE_ERROR;
                }
            } else {
                VarType a = exprType(e->left);
                VarType b = exprType(e->right);
                if (a == TYPE_TEXT || b == TYPE_TEXT) {
                    semError(e->line, "la operacion '%s' no se puede hacer con texto (pilot)", e->op);
                    e->etype = TYPE_ERROR;
                    break;
                }
                if (strcmp(e->op, "/") == 0 && isZero(e->right))
                    semError(e->line, "division entre cero");
                e->etype = combine(a, b);
            }
            break;
        case ND_RELOP: {
            VarType a = exprType(e->left);
            VarType b = exprType(e->right);
            int ta = (a == TYPE_TEXT), tb = (b == TYPE_TEXT);
            if (a != TYPE_ERROR && b != TYPE_ERROR && ta != tb)
                semError(e->line, "no se puede comparar un texto con un numero");
            else if (ta && tb && strcmp(e->op, "==") != 0 && strcmp(e->op, "!=") != 0)
                semError(e->line, "un texto solo se compara con == o != (no con '%s')", e->op);
            e->etype = TYPE_INT;   /* resultado logico */
            break;
        }
        default:
            e->etype = TYPE_ERROR;
            break;
    }
    return e->etype;
}

static int isNumeric(VarType t) { return t == TYPE_INT || t == TYPE_FLOAT; }

static void checkAssignable(VarType dest, VarType src, const char *var, int line) {
    if (dest == TYPE_TEXT && isNumeric(src))
        semError(line, "tipos incompatibles: '%s' es pilot (texto) y la expresion es numerica", var);
    else if (isNumeric(dest) && src == TYPE_TEXT)
        semError(line, "tipos incompatibles: '%s' es %s y la expresion es texto", var, typeName(dest));
    else if (dest == TYPE_INT && src == TYPE_FLOAT)
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
                if (s->isGeneric)                 /* chassis: el tipo sale del valor inicial */
                    s->vtype = t;
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
                else if (v->type == TYPE_TEXT || t == TYPE_TEXT)
                    semError(s->line, "boost no se puede usar con texto (pilot)");
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
            case ND_FOR:
                checkList(s->init);               /* init y paso son una sola sentencia (next == NULL) */
                exprType(s->cond);
                s->hasFinish = checkList(s->thenBody);
                checkList(s->step);
                break;
            case ND_FINISH:
            case ND_RETURN:
                if (s->type == ND_RETURN) {
                    VarType t = exprType(s->expr);
                    if (t == TYPE_TEXT) podiumText = 1;
                    else if (isNumeric(t)) podiumNum = 1;
                    if (podiumText && podiumNum)
                        semError(s->line, "todos los podium deben devolver el mismo tipo "
                                          "(un texto o un numero, no ambos)");
                }
                s->hasFinish = 1;
                if (s->next)
                    semWarning(s->next->line,
                               "codigo inalcanzable despues de '%s' (linea %d); no se traducira",
                               s->type == ND_RETURN ? "podium" : "finish", s->line);
                break;
            default:
                break;
        }
        if (s->hasFinish) found = 1;
    }
    return found;
}

/* ---------- Regla 10: variables con valor por defecto que reciben texto ---------- */

typedef struct { ASTNode **d; int n, cap; } DeclList;

static void collectDecls(ASTNode *s, DeclList *L) {
    for (; s; s = s->next) {
        ASTNode *one[2] = { s, s->type == ND_FOR ? s->init : NULL };
        for (int k = 0; k < 2; k++) {
            ASTNode *d = one[k];
            if (!d || d->type != ND_DECL) continue;
            if (L->n == L->cap) {
                L->cap = L->cap ? L->cap * 2 : 16;
                L->d = realloc(L->d, (size_t)L->cap * sizeof *L->d);
                if (!L->d) exit(2);
            }
            L->d[L->n++] = d;
        }
        collectDecls(s->thenBody, L);
        collectDecls(s->elseBody, L);
    }
}

static ASTNode *findDecl(DeclList *L, const char *name) {
    for (int i = 0; i < L->n; i++)
        if (strcmp(L->d[i]->name, name) == 0) return L->d[i];   /* la primera declaracion */
    return NULL;
}

static int isLiteral(ASTNode *e) {
    if (e->type == ND_BINOP && strcmp(e->op, "neg") == 0) e = e->right;
    return e->type == ND_INT || e->type == ND_FLOAT;
}

static int isTextDecl(ASTNode *d) {
    return d->vtype == TYPE_TEXT || (d->isGeneric && d->expr && d->expr->type == ND_TEXT);
}

/* 1 si la expresion es un texto: un literal o una variable que ya es texto */
static int givesText(ASTNode *e, DeclList *L) {
    if (e->type == ND_TEXT) return 1;
    if (e->type == ND_VAR) {
        ASTNode *d = findDecl(L, e->name);
        return d && isTextDecl(d);
    }
    return 0;
}

static int promoteIn(ASTNode *s, DeclList *L) {
    int changed = 0;
    for (; s; s = s->next) {
        ASTNode *one[3] = { s, s->type == ND_FOR ? s->init : NULL, s->type == ND_FOR ? s->step : NULL };
        for (int k = 0; k < 3; k++) {
            ASTNode *a = one[k];
            if (!a || a->type != ND_ASSIGN || !givesText(a->expr, L)) continue;
            ASTNode *d = findDecl(L, a->name);
            if (!d || isTextDecl(d) || !isLiteral(d->expr)) continue;
            semWarning(d->line, "'%s' se declaro como %s con valor por defecto, pero en la linea %d "
                                "recibe un texto: se toma como pilot (texto)",
                       d->name, typeName(d->vtype), a->line);
            d->promoted = 1;
            d->promotedFrom = d->vtype;
            d->vtype = TYPE_TEXT;
            d->isGeneric = 0;
            freeAST(d->expr);
            d->expr = newTextLit(strdup(""), d->line);   /* valor por defecto de un texto */
            changed = 1;
        }
        changed |= promoteIn(s->thenBody, L);
        changed |= promoteIn(s->elseBody, L);
    }
    return changed;
}

static void promoteTextVars(ASTNode *stmts) {
    DeclList L = {0};
    collectDecls(stmts, &L);
    while (promoteIn(stmts, &L)) { }   /* hasta que no cambie (b := a, con a ya texto) */
    free(L.d);
}

int semanticCheck(ASTNode *program, SymTable *t) {
    table = t;
    errors = 0;
    podiumNum = podiumText = 0;
    if (program) {
        promoteTextVars(program->thenBody);
        checkList(program->thenBody);
    }
    return errors;
}
