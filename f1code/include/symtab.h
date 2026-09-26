#ifndef SYMTAB_H
#define SYMTAB_H

#include "ast.h"

typedef struct SymEntry {
    char *name;
    VarType type;
    int line;
    struct SymEntry *next;
} SymEntry;

typedef struct SymTable {
    SymEntry *head;
    int count;
} SymTable;

SymTable *symtab_create(void);
int symtab_insert(SymTable *t, const char *name, VarType type, int line); /* 0 si ya existia */
SymEntry *symtab_lookup(SymTable *t, const char *name);
void symtab_print(SymTable *t);
void symtab_free(SymTable *t);

#endif
