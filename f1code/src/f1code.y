%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "symtab.h"

int yylex(void);
void yyerror(const char *s);

ASTNode *programRoot = NULL;
SymTable *symtab = NULL;
int lexErrorCount = 0;
int synErrorCount = 0;
%}

/* Mensajes de error con lo encontrado y lo esperado; @n da la linea de cada simbolo */
%define parse.error verbose
%locations

%union {
    int ival;
    double fval;
    char *sval;
    struct ASTNode *node;
}

%token GARAGE "garage" ENGINE "engine" FUEL "fuel" PITSTOP "pitstop" RADIO "radio"
%token RACE "race" OVERTAKE "overtake" LAP "lap" BOOST "boost" FINISH "finish"
%token ASSIGN ":=" LE "<=" GE ">=" EQ "==" NE "!=" LT "<" GT ">"
%token PLUS "+" MINUS "-" TIMES "*" DIVIDE "/"
%token LPAREN "(" RPAREN ")" LBRACE "{" RBRACE "}" SEMI ";"
%token <ival> ENTERO "numero entero"
%token <fval> REAL   "numero real"
%token <sval> IDENT  "identificador"

%type <node> programa lista_sent sentencia declaracion asignacion incremento
%type <node> entrada salida condicional ciclo fin_stmt opt_overtake
%type <node> cond expr term factor

/* Libera lo que Bison descarta al recuperarse de un error */
%destructor { free($$); } <sval>
%destructor { freeAST($$); } <node>

%%

programa:
    lista_sent {
        /* Tras recuperarse de un error en el nivel principal, Bison puede
           reducir 'programa' otra vez: se libera el anterior. */
        freeAST(programRoot);
        programRoot = newProgram($1);
        $$ = NULL;   /* programRoot es dueno del AST; NULL evita que el %destructor lo libere */
    }
    ;

lista_sent:
      %empty                { $$ = NULL; }
    | sentencia lista_sent  { if ($1) { $1->next = $2; $$ = $1; } else { $$ = $2; } }
    ;

sentencia:
      declaracion
    | asignacion
    | incremento
    | entrada
    | salida
    | condicional
    | ciclo
    | fin_stmt
    | error SEMI            { $$ = NULL; yyerrok; }   /* recuperacion: saltar hasta ';' y seguir */
    ;

/* La tabla de simbolos y las validaciones se hacen despues, en semantico.c */
declaracion:
      GARAGE ENGINE IDENT ASSIGN expr SEMI { $$ = newDecl(TYPE_INT,   $3, $5, @1.first_line); }
    | GARAGE FUEL   IDENT ASSIGN expr SEMI { $$ = newDecl(TYPE_FLOAT, $3, $5, @1.first_line); }
    ;

asignacion:
    IDENT ASSIGN expr SEMI  { $$ = newAssign($1, $3, @1.first_line); }
    ;

incremento:
    IDENT BOOST expr SEMI   { $$ = newBoost($1, $3, @1.first_line); }
    ;

entrada:
    PITSTOP IDENT SEMI      { $$ = newInput($2, @1.first_line); }
    ;

salida:
    RADIO expr SEMI         { $$ = newOutput($2, @1.first_line); }
    ;

condicional:
    RACE LPAREN cond RPAREN LBRACE lista_sent RBRACE opt_overtake {
        $$ = newIf($3, $6, $8, @1.first_line);
    }
    ;

opt_overtake:
      %empty                              { $$ = NULL; }
    | OVERTAKE LBRACE lista_sent RBRACE   { $$ = $3; }
    ;

ciclo:
    LAP LPAREN cond RPAREN LBRACE lista_sent RBRACE {
        $$ = newWhile($3, $6, @1.first_line);
    }
    ;

fin_stmt:
    FINISH SEMI             { $$ = newFinish(@1.first_line); }
    ;

cond:
      expr LT expr { $$ = newRelOp("<",  $1, $3, @2.first_line); }
    | expr GT expr { $$ = newRelOp(">",  $1, $3, @2.first_line); }
    | expr LE expr { $$ = newRelOp("<=", $1, $3, @2.first_line); }
    | expr GE expr { $$ = newRelOp(">=", $1, $3, @2.first_line); }
    | expr EQ expr { $$ = newRelOp("==", $1, $3, @2.first_line); }
    | expr NE expr { $$ = newRelOp("!=", $1, $3, @2.first_line); }
    ;

expr:
      expr PLUS term   { $$ = newBinOp("+", $1, $3, @2.first_line); }
    | expr MINUS term  { $$ = newBinOp("-", $1, $3, @2.first_line); }
    | term             { $$ = $1; }
    ;

term:
      term TIMES factor   { $$ = newBinOp("*", $1, $3, @2.first_line); }
    | term DIVIDE factor  { $$ = newBinOp("/", $1, $3, @2.first_line); }
    | factor              { $$ = $1; }
    ;

factor:
      ENTERO               { $$ = newIntLit($1, @1.first_line); }
    | REAL                 { $$ = newFloatLit($1, @1.first_line); }
    | IDENT                { $$ = newVar($1, @1.first_line); }
    | LPAREN expr RPAREN   { $$ = $2; }
    | MINUS factor         { $$ = newNeg($2, @1.first_line); }   /* lo usa el contrato: boost -10 */
    ;

%%

/* Traduce al espanol el mensaje que genera Bison */
static void replace(char *dst, size_t size, const char *src, const char *from, const char *to) {
    size_t n = 0, lf = strlen(from), lt = strlen(to);
    while (*src && n + 1 < size) {
        if (strncmp(src, from, lf) == 0 && n + lt < size) {
            memcpy(dst + n, to, lt);
            n += lt;
            src += lf;
        } else {
            dst[n++] = *src++;
        }
    }
    dst[n] = '\0';
}

void yyerror(const char *s) {
    char a[512], b[512];
    replace(a, sizeof a, s, "syntax error", "error de sintaxis");
    replace(b, sizeof b, a, ", unexpected ", ": se encontro ");
    replace(a, sizeof a, b, ", expecting ", ", se esperaba ");
    replace(b, sizeof b, a, " or ", " o ");
    replace(a, sizeof a, b, "end of file", "fin de archivo");
    fprintf(stderr, "Error sintactico en linea %d: %s\n", yylloc.first_line, a);
    synErrorCount++;
}
