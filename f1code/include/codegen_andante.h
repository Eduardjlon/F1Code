#ifndef CODEGEN_ANDANTE_H
#define CODEGEN_ANDANTE_H

#include "ast.h"
#include "symtab.h"

/*
 * Genera el programa Andante (.and) equivalente a un AST de F1Code ya
 * validado por semanticCheck(). Devuelve una cadena nueva (liberar con
 * free). 'source' solo se usa en el comentario inicial.
 *
 * Es el unico modulo que conoce Andante: si cambia el destino, solo se
 * reemplaza este archivo.
 */
char *generateAndante(ASTNode *program, SymTable *t, const char *source);

#endif
