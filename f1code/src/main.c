/*
 * f1code — Traductor F1Code (.f1) -> Andante (.and)
 * Grupo 1 · Proyecto de Compiladores · USPG
 *
 * Flujo:  .f1 -> Flex -> Bison -> AST -> semantico -> generador Andante -> .and
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "symtab.h"
#include "semantico.h"
#include "codegen_andante.h"

extern FILE *yyin;
extern int yyparse(void);
extern int yylex_destroy(void);
extern ASTNode *programRoot;
extern SymTable *symtab;
extern int lexErrorCount;
extern int synErrorCount;

static void usage(const char *prog) {
    fprintf(stderr,
            "Uso: %s <archivo.f1> [-o salida.and] [--ast] [--tabla]\n"
            "  -o archivo   nombre del archivo de salida (por defecto: el mismo nombre con extension .and)\n"
            "  --ast        imprime el arbol de sintaxis abstracta\n"
            "  --tabla      imprime la tabla de simbolos\n",
            prog);
}

static int endsWith(const char *s, const char *suffix) {
    size_t a = strlen(s), b = strlen(suffix);
    return a >= b && strcmp(s + a - b, suffix) == 0;
}

/* programa.f1 -> programa.and */
static char *defaultOutput(const char *input) {
    const char *dot = strrchr(input, '.');
    const char *sep = NULL;
    for (const char *c = input; *c; c++)
        if (*c == '/' || *c == '\\') sep = c;
    size_t base = (dot && (!sep || dot > sep)) ? (size_t)(dot - input) : strlen(input);
    char *r = malloc(base + 5);
    if (!r) exit(2);
    memcpy(r, input, base);
    strcpy(r + base, ".and");
    return r;
}

static void cleanup(char *output) {
    free(output);
    freeAST(programRoot);
    programRoot = NULL;
    symtab_free(symtab);
    yylex_destroy();
}

int main(int argc, char **argv) {
    const char *input = NULL;
    char *output = NULL;
    int showAst = 0, showTable = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            free(output);
            output = strdup(argv[++i]);
        } else if (strcmp(argv[i], "--ast") == 0) {
            showAst = 1;
        } else if (strcmp(argv[i], "--tabla") == 0) {
            showTable = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else if (argv[i][0] == '-' || input) {
            usage(argv[0]);
            return 2;
        } else {
            input = argv[i];
        }
    }
    if (!input) {
        usage(argv[0]);
        return 2;
    }
    if (!endsWith(input, ".f1"))
        fprintf(stderr, "Advertencia: '%s' no tiene extension .f1\n", input);
    if (!output) output = defaultOutput(input);

    yyin = fopen(input, "r");
    if (!yyin) {
        perror("No se pudo abrir el archivo de entrada");
        free(output);
        return 2;
    }

    symtab = symtab_create();

    /* 1) Analisis lexico y sintactico */
    int parseResult = yyparse();
    fclose(yyin);
    if (parseResult != 0 || synErrorCount > 0 || lexErrorCount > 0) {
        fprintf(stderr, "\n>> Compilacion ABORTADA: %d error(es) lexico(s), %d error(es) sintactico(s). No se genero %s\n",
                lexErrorCount, synErrorCount, output);
        cleanup(output);
        return 1;
    }

    /* 2) Analisis semantico (llena la tabla de simbolos) */
    int semErrors = semanticCheck(programRoot, symtab);
    if (showAst) {
        printf("--- Arbol de Sintaxis Abstracta (AST) ---\n");
        printAST(programRoot, 0);
        printf("\n");
    }
    if (showTable) {
        printf("--- Tabla de simbolos ---\n");
        symtab_print(symtab);
        printf("\n");
    }
    if (semErrors > 0) {
        fprintf(stderr, "\n>> Compilacion ABORTADA: %d error(es) semantico(s). No se genero %s\n",
                semErrors, output);
        cleanup(output);
        return 1;
    }

    /* 3) Generacion de Andante (solo si no hubo errores) */
    char *code = generateAndante(programRoot, symtab, input);
    FILE *f = fopen(output, "wb");
    if (!f) {
        perror("No se pudo crear el archivo de salida");
        free(code);
        cleanup(output);
        return 2;
    }
    fputs(code, f);
    fclose(f);
    printf(">> Traduccion completada: %s -> %s\n", input, output);

    free(code);
    cleanup(output);
    return 0;
}
