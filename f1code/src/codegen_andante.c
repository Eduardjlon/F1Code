/*
 * codegen_andante.c — Generador F1Code -> Andante (Grupo 8)
 *
 *   garage engine x := e;     ->  numero x = e;
 *   garage fuel x := e;       ->  decimal x = e;
 *   garage chassis x := e;    ->  numero o decimal, segun el tipo inferido de e
 *   garage pilot x := 'A';    ->  texto x = "A";  (los textos salen con comillas dobles)
 *   x := e;                   ->  avanzar x = e;
 *   x boost e;                ->  avanzar x = x + (e);
 *   pitstop x;                ->  leer x;
 *   radio e;                  ->  caminar e;
 *   race (c) {A} overtake {B} ->  virar (c) inicio A final sino inicio B final
 *   lap (c) {A}               ->  historia (c) inicio A final
 *   stint (I; c; P) {A}       ->  I  historia (c) inicio A P final
 *   finish;                   ->  ver emitList()
 *   podium e;                 ->  avanzar f1_podio = e;  y luego como finish
 *   // ... y / * ... * /       ->  se copian igual, en el mismo lugar del programa
 *
 * Ademas:
 *   - Una declaracion que quedaria dentro de un bloque de Andante se sube al
 *     inicio del programa (F1Code tiene tabla global; asi no dependemos del
 *     alcance de Andante). En su lugar queda un 'avanzar'.
 *   - Identificadores que chocan con palabras reservadas de Andante se
 *     renombran agregando "_f1".
 *   - Subexpresiones compuestas se generan entre parentesis.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "codegen_andante.h"
#include "comentarios.h"

/* ---------- Buffer de texto ---------- */

typedef struct {
    char *d;
    size_t len, cap;
} Buf;

static void bufPrintf(Buf *b, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n < 0) return;
    if (b->len + (size_t)n + 1 > b->cap) {
        size_t nc = b->cap ? b->cap : 256;
        while (nc < b->len + (size_t)n + 1) nc *= 2;
        char *nd = realloc(b->d, nc);
        if (!nd) { fprintf(stderr, "memoria insuficiente\n"); exit(2); }
        b->d = nd;
        b->cap = nc;
    }
    va_start(ap, fmt);
    vsnprintf(b->d + b->len, (size_t)n + 1, fmt, ap);
    va_end(ap);
    b->len += (size_t)n;
}

static Buf body;            /* instrucciones                    */
static int keepComments = 1;   /* 0 con --sin-comentarios */
static int nextComment = 0;    /* siguiente comentario por copiar */
static int lastLine = 0;       /* linea .f1 de la ultima instruccion escrita en una sola linea */
static Buf decls;           /* declaraciones subidas al inicio  */
static char *flag = NULL;   /* variable para finish anidado     */
static char *podio = NULL;  /* variable que recibe el valor de podium */
static SymTable *table = NULL;

/* ---------- Nombres ---------- */

static const char *ANDANTE_KEYWORDS[] = {
    "caminar", "leer", "avanzar", "virar", "sino", "historia", "inicio",
    "final", "detener", "numero", "decimal", "texto", "verdadero", "falso",
    NULL
};

static int isAndanteKeyword(const char *s) {
    for (int i = 0; ANDANTE_KEYWORDS[i]; i++)
        if (strcmp(ANDANTE_KEYWORDS[i], s) == 0) return 1;
    return 0;
}

static int nameTaken(const char *s) {
    if (isAndanteKeyword(s)) return 1;
    if (flag && strcmp(flag, s) == 0) return 1;
    if (podio && strcmp(podio, s) == 0) return 1;
    for (SymEntry *e = table->head; e; e = e->next) {
        if (strcmp(e->name, s) == 0) return 1;
        if (e->outName && strcmp(e->outName, s) == 0) return 1;
    }
    return 0;
}

/* Devuelve 'base' o base_f1, base_f1_f1... hasta encontrar uno libre */
static char *freeName(const char *base) {
    char *cand = strdup(base);
    while (nameTaken(cand)) {
        size_t n = strlen(cand);
        char *longer = malloc(n + 4);
        if (!longer) exit(2);
        memcpy(longer, cand, n);
        memcpy(longer + n, "_f1", 4);
        free(cand);
        cand = longer;
    }
    return cand;
}

static void assignNames(void) {
    /* 1) los nombres que no chocan se quedan igual */
    for (SymEntry *e = table->head; e; e = e->next) {
        free(e->outName);
        e->outName = isAndanteKeyword(e->name) ? NULL : strdup(e->name);
    }
    /* 2) los que chocan con Andante se renombran */
    for (SymEntry *e = table->head; e; e = e->next)
        if (!e->outName) e->outName = freeName(e->name);
}

static const char *outName(const char *f1Name) {
    SymEntry *e = symtab_lookup(table, f1Name);
    return (e && e->outName) ? e->outName : f1Name;
}

static const char *andanteType(VarType t) {
    return t == TYPE_FLOAT ? "decimal" : t == TYPE_TEXT ? "texto" : "numero";
}

/* Valor neutro de cada tipo (declaraciones subidas al inicio y f1_podio) */
static const char *neutralValue(VarType t) {
    return t == TYPE_FLOAT ? "0.0" : t == TYPE_TEXT ? "\"\"" : "0";
}

/* Literal de texto de Andante: entre comillas dobles. Las secuencias de escape del
   original (\n, \t, \") pasan tal cual; una " sin escapar (de un texto con comillas
   simples, como 'dijo "hola"') se escapa para no cerrar el texto antes de tiempo. */
static void emitText(Buf *b, const char *s) {
    bufPrintf(b, "\"");
    for (; *s; s++) {
        if (*s == '\\' && s[1]) {
            bufPrintf(b, "%c%c", s[0], s[1]);
            s++;
        } else if (*s == '"') {
            bufPrintf(b, "\\\"");
        } else {
            bufPrintf(b, "%c", *s);
        }
    }
    bufPrintf(b, "\"");
}

/* El real mas corto que vuelve exactamente al mismo valor: 50.5, 0.25, 3.0 */
static void formatReal(char *out, size_t size, double v) {
    for (int p = 1; p <= 17; p++) {
        snprintf(out, size, "%.*f", p, v);
        if (strtod(out, NULL) == v) return;
    }
}

/* ---------- finish ---------- */

static int isStop(ASTNode *s) { return s->type == ND_FINISH || s->type == ND_RETURN; }

/* 1 si hay un finish o podium alcanzable dentro de algun bloque (nivel > 0) */
static int nestedFinish(ASTNode *s, int level) {
    for (; s; s = s->next) {
        if (isStop(s)) return level > 0;   /* lo que sigue es inalcanzable */
        if (s->type == ND_IF &&
            (nestedFinish(s->thenBody, level + 1) || nestedFinish(s->elseBody, level + 1)))
            return 1;
        if ((s->type == ND_WHILE || s->type == ND_FOR) && nestedFinish(s->thenBody, level + 1))
            return 1;
    }
    return 0;
}

/* Busca los podium alcanzables: *found = 1 si hay alguno; *rtype = tipo de f1_podio
   (fuel si alguno devuelve fuel, pilot si devuelven texto; semantico.c impide mezclar texto y numero) */
static void scanReturns(ASTNode *s, int *found, VarType *rtype) {
    for (; s; s = s->next) {
        if (s->type == ND_RETURN) {
            *found = 1;
            if (s->expr->etype == TYPE_FLOAT || s->expr->etype == TYPE_TEXT) *rtype = s->expr->etype;
        }
        if (isStop(s)) return;
        if (s->type == ND_IF) {
            scanReturns(s->thenBody, found, rtype);
            scanReturns(s->elseBody, found, rtype);
        }
        if (s->type == ND_WHILE || s->type == ND_FOR)
            scanReturns(s->thenBody, found, rtype);
    }
}

/* ---------- Expresiones ---------- */

static int isNeg(ASTNode *e) { return e->type == ND_BINOP && strcmp(e->op, "neg") == 0; }

static void emitExpr(Buf *b, ASTNode *e, int asChild) {
    char num[64];
    switch (e->type) {
        case ND_INT:
            bufPrintf(b, "%d", e->ival);
            break;
        case ND_FLOAT:
            formatReal(num, sizeof num, e->fval);
            bufPrintf(b, "%s", num);
            break;
        case ND_TEXT:
            emitText(b, e->name);
            break;
        case ND_VAR:
            bufPrintf(b, "%s", outName(e->name));
            break;
        case ND_BINOP:
            if (asChild) bufPrintf(b, "(");
            if (isNeg(e)) {
                bufPrintf(b, "-");
                emitExpr(b, e->right, 1);
            } else {
                emitExpr(b, e->left, 1);
                bufPrintf(b, " %s ", e->op);
                emitExpr(b, e->right, 1);
            }
            if (asChild) bufPrintf(b, ")");
            break;
        case ND_RELOP:
            emitExpr(b, e->left, 0);
            bufPrintf(b, " %s ", e->op);
            emitExpr(b, e->right, 0);
            break;
        default:
            break;
    }
}

/* ---------- Sentencias ---------- */

static void indent(int level) {
    for (int i = 0; i < level; i++) bufPrintf(&body, "    ");
}

static void emitList(ASTNode *s, int level, int depth);
static void indent(int level);

void setKeepComments(int on) { keepComments = on; }

/* Copia, en orden, los comentarios que empiezan antes de 'beforeLine'. Un comentario
   que estaba en la misma linea que la instruccion anterior se deja a su lado. */
static void flushComments(int beforeLine, int level) {
    if (!keepComments) return;
    while (nextComment < comentariosTotal() && comentarioEn(nextComment)->line < beforeLine) {
        const Comentario *c = comentarioEn(nextComment);
        if (c->line == lastLine && body.len && body.d[body.len - 1] == '\n') {
            body.len--;                                   /* x = 1; // comentario */
            bufPrintf(&body, "   %s\n", c->text);
        } else {
            indent(level);
            bufPrintf(&body, "%s\n", c->text);
        }
        lastLine = 0;
        nextComment++;
    }
}

static void emitStmt(ASTNode *s, int level, int depth) {
    const char *name = s->name ? outName(s->name) : NULL;
    switch (s->type) {
        case ND_DECL:
            indent(level);
            if (level == 0) {
                bufPrintf(&body, "%s %s = ", andanteType(s->vtype), name);
            } else {
                /* se sube la declaracion y aqui queda la asignacion */
                bufPrintf(&decls, "%s %s = %s;\n", andanteType(s->vtype), name, neutralValue(s->vtype));
                bufPrintf(&body, "avanzar %s = ", name);
            }
            emitExpr(&body, s->expr, 0);
            bufPrintf(&body, ";\n");
            break;

        case ND_ASSIGN:
            indent(level);
            bufPrintf(&body, "avanzar %s = ", name);
            emitExpr(&body, s->expr, 0);
            bufPrintf(&body, ";\n");
            break;

        case ND_BOOST:
            indent(level);
            bufPrintf(&body, "avanzar %s = %s + ", name, name);
            emitExpr(&body, s->expr, 1);
            bufPrintf(&body, ";\n");
            break;

        case ND_INPUT:
            indent(level);
            bufPrintf(&body, "leer %s;\n", name);
            break;

        case ND_OUTPUT:
            indent(level);
            bufPrintf(&body, "caminar ");
            emitExpr(&body, s->expr, 0);
            bufPrintf(&body, ";\n");
            break;

        case ND_IF:
            indent(level);
            bufPrintf(&body, "virar (");
            emitExpr(&body, s->cond, 0);
            bufPrintf(&body, ") inicio\n");
            emitList(s->thenBody, level + 1, depth + 1);
            flushComments(s->endLine, level + 1);      /* comentarios al final del bloque */
            indent(level);
            bufPrintf(&body, "final\n");
            if (s->elseBody) {
                indent(level);
                bufPrintf(&body, "sino inicio\n");
                emitList(s->elseBody, level + 1, depth + 1);
                flushComments(s->elseEnd, level + 1);
                indent(level);
                bufPrintf(&body, "final\n");
            }
            break;

        case ND_WHILE:
            indent(level);
            bufPrintf(&body, "historia (");
            if (s->hasFinish) bufPrintf(&body, "%s == 0 && ", flag);
            emitExpr(&body, s->cond, 0);
            bufPrintf(&body, ") inicio\n");
            emitList(s->thenBody, level + 1, depth + 1);
            flushComments(s->endLine, level + 1);
            indent(level);
            bufPrintf(&body, "final\n");
            break;

        case ND_FOR: {
            /* stint (I; c; P) { A }  ==  I; lap (c) { A P; }
               El paso se agrega al final del cuerpo solo mientras se genera, asi
               las reglas de finish lo dejan dentro del virar (<flag> == 0). */
            emitStmt(s->init, level, depth);
            indent(level);
            bufPrintf(&body, "historia (");
            if (s->hasFinish) bufPrintf(&body, "%s == 0 && ", flag);
            emitExpr(&body, s->cond, 0);
            bufPrintf(&body, ") inicio\n");
            ASTNode *last = s->thenBody;
            while (last && last->next) last = last->next;
            if (last) last->next = s->step;
            emitList(last ? s->thenBody : s->step, level + 1, depth + 1);
            if (last) last->next = NULL;   /* se desengancha: el AST queda como estaba */
            flushComments(s->endLine, level + 1);
            indent(level);
            bufPrintf(&body, "final\n");
            break;
        }

        default:
            break;
    }
}

/*
 * level = sangria en Andante; depth = anidamiento en F1Code (race/lap).
 * Son distintos porque el virar (<flag> == 0) agrega sangria sin agregar
 * un bloque de F1Code.
 *
 * finish:
 *   - En el nivel principal de F1Code (depth 0): se deja de generar.
 *   - Dentro de un bloque: avanzar <flag> = 1; y el resto del bloque no se
 *     genera. Despues de una sentencia que contiene un finish, el resto de la
 *     lista queda dentro de virar (<flag> == 0), y los ciclos que lo contienen
 *     agregan "<flag> == 0 &&" a su condicion.
 */
/* finish / podium: podium guarda su valor; dentro de un bloque se levanta la bandera */
static void emitStop(ASTNode *s, int level, int depth) {
    if (s->type == ND_RETURN) {
        indent(level);
        bufPrintf(&body, "avanzar %s = ", podio);
        emitExpr(&body, s->expr, 0);
        bufPrintf(&body, ";\n");
    }
    if (depth > 0) {
        indent(level);
        bufPrintf(&body, "avanzar %s = 1;\n", flag);
    }
}

static void emitList(ASTNode *s, int level, int depth) {
    for (; s; s = s->next) {
        flushComments(s->line, level);   /* comentarios que van antes de esta sentencia */
        if (isStop(s)) {
            emitStop(s, level, depth);
            /* solo si escribio algo (un finish del nivel principal no genera nada) */
            lastLine = (s->type == ND_RETURN || depth > 0) ? s->line : 0;
            return;
        }
        emitStmt(s, level, depth);
        lastLine = (s->type == ND_IF || s->type == ND_WHILE || s->type == ND_FOR) ? 0 : s->line;
        if (s->hasFinish && s->next) {
            /* el siguiente es finish: no hace falta virar. (Un podium si lo necesita:
               no debe pisar el valor de un podium anidado que ya se ejecuto.) */
            if (s->next->type == ND_FINISH) {
                if (depth > 0) {
                    indent(level);
                    bufPrintf(&body, "avanzar %s = 1;\n", flag);
                }
                return;
            }
            indent(level);
            bufPrintf(&body, "virar (%s == 0) inicio\n", flag);
            emitList(s->next, level + 1, depth);
            indent(level);
            bufPrintf(&body, "final\n");
            return;
        }
    }
}

/* ---------- Punto de entrada ---------- */

static const char *baseName(const char *path) {
    const char *p = path;
    for (const char *c = path; *c; c++)
        if (*c == '/' || *c == '\\') p = c + 1;
    return p;
}

char *generateAndante(ASTNode *program, SymTable *t, const char *source) {
    Buf out = {0};
    body = (Buf){0};
    decls = (Buf){0};
    table = t;
    free(flag);
    flag = NULL;
    free(podio);
    podio = NULL;

    ASTNode *stmts = program ? program->thenBody : NULL;
    int hasReturn = 0;
    VarType returnType = TYPE_INT;
    assignNames();
    if (nestedFinish(stmts, 0)) flag = freeName("f1_fin");
    scanReturns(stmts, &hasReturn, &returnType);
    if (hasReturn) podio = freeName("f1_podio");

    nextComment = 0;
    lastLine = 0;
    emitList(stmts, 0, 0);
    flushComments(INT_MAX, 0);           /* comentarios del final del programa */

    bufPrintf(&out, "// Codigo Andante generado por f1code (Grupo 1) desde %s\n\n", baseName(source));
    if (decls.len) bufPrintf(&out, "%s", decls.d);
    if (flag) bufPrintf(&out, "numero %s = 0;\n", flag);
    if (podio) bufPrintf(&out, "%s %s = %s;\n", andanteType(returnType), podio, neutralValue(returnType));
    if (decls.len || flag || podio) bufPrintf(&out, "\n");
    if (body.len) bufPrintf(&out, "%s", body.d);

    free(body.d);
    free(decls.d);
    free(flag);
    flag = NULL;
    free(podio);
    podio = NULL;
    return out.d;
}
