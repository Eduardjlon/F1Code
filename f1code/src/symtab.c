#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"

SymTable *symtab_create(void) {
    SymTable *t = (SymTable *)malloc(sizeof(SymTable));
    t->head = NULL;
    t->count = 0;
    return t;
}

int symtab_insert(SymTable *t, const char *name, VarType type, int line) {
    if (symtab_lookup(t, name) != NULL) {
        return 0; /* ya existe: redeclaracion */
    }
    SymEntry *e = (SymEntry *)malloc(sizeof(SymEntry));
    e->name = strdup(name);
    e->type = type;
    e->line = line;
    e->next = t->head;
    t->head = e;
    t->count++;
    return 1;
}

SymEntry *symtab_lookup(SymTable *t, const char *name) {
    for (SymEntry *e = t->head; e != NULL; e = e->next) {
        if (strcmp(e->name, name) == 0) return e;
    }
    return NULL;
}

void symtab_print(SymTable *t) {
    if (t->count == 0) {
        printf("(sin variables declaradas)\n");
        return;
    }
    printf("%-15s %-10s %s\n", "Nombre", "Tipo", "Linea");
    printf("%-15s %-10s %s\n", "------", "----", "-----");
    /* invertir orden de insercion para mostrar en orden de declaracion */
    SymEntry *arr[256];
    int n = 0;
    for (SymEntry *e = t->head; e != NULL && n < 256; e = e->next) arr[n++] = e;
    for (int i = n - 1; i >= 0; i--) {
        printf("%-15s %-10s %d\n", arr[i]->name, arr[i]->type == TYPE_INT ? "engine" : "fuel", arr[i]->line);
    }
}

void symtab_free(SymTable *t) {
    SymEntry *e = t->head;
    while (e) {
        SymEntry *nx = e->next;
        free(e->name);
        free(e);
        e = nx;
    }
    free(t);
}
