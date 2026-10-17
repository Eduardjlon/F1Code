#ifndef SEMANTICO_H
#define SEMANTICO_H

#include "ast.h"
#include "symtab.h"

/*
 * Recorre el AST, llena la tabla de simbolos, anota el tipo de cada
 * expresion (etype) y marca las sentencias que contienen un finish
 * (hasFinish). Devuelve la cantidad de errores semanticos.
 */
int semanticCheck(ASTNode *program, SymTable *t);

#endif
