#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "symtab.h"

extern FILE *yyin;
extern int yyparse(void);
extern ASTNode *programRoot;
extern SymTable *symtab;
extern int lexErrorCount;
extern int synErrorCount;

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <archivo.f1>\n", argv[0]);
        return 1;
    }

    yyin = fopen(argv[1], "r");
    if (!yyin) {
        perror("No se pudo abrir el archivo de entrada");
        return 1;
    }

    symtab = symtab_create();

    printf("=== F1Code Front-end ===\n");
    printf("Archivo: %s\n\n", argv[1]);

    int parseResult = yyparse();
    fclose(yyin);

    if (parseResult != 0 || synErrorCount > 0 || lexErrorCount > 0) {
        printf("\n>> Compilacion ABORTADA: %d error(es) lexico(s), %d error(es) sintactico(s).\n",
               lexErrorCount, synErrorCount);
        symtab_free(symtab);
        return 1;
    }

    printf(">> Analisis lexico y sintactico completados sin errores.\n\n");

    printf("--- Arbol de Sintaxis Abstracta (AST) ---\n");
    printAST(programRoot, 0);

    printf("\n--- Tabla de simbolos ---\n");
    symtab_print(symtab);

    freeAST(programRoot);
    symtab_free(symtab);
    return 0;
}
