%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "symtab.h"

extern int yylineno;
extern char *yytext;
int yylex(void);
void yyerror(const char *s);

ASTNode *programRoot = NULL;
SymTable *symtab = NULL;
int lexErrorCount = 0;
int synErrorCount = 0;
%}

%union {
    int ival;
    double fval;
    char *sval;
    struct ASTNode *node;
}

%token GARAGE ENGINE FUEL PITSTOP RADIO RACE OVERTAKE LAP BOOST FINISH
%token ASSIGN LE GE EQ NE LT GT PLUS MINUS TIMES DIVIDE
%token LPAREN RPAREN LBRACE RBRACE SEMI
%token <ival> ENTERO
%token <fval> REAL
%token <sval> IDENT

%type <node> programa lista_sent sentencia declaracion asignacion incremento
%type <node> entrada salida condicional ciclo fin_stmt opt_overtake
%type <node> cond expr term factor

%%

programa:
    lista_sent { programRoot = newProgram($1); }
    ;

lista_sent:
      %empty                { $$ = NULL; }
    | sentencia lista_sent { $1->next = $2; $$ = $1; }
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
    ;

declaracion:
      GARAGE ENGINE IDENT ASSIGN expr SEMI {
          $$ = newDecl(TYPE_INT, $3, $5, yylineno);
          if (!symtab_insert(symtab, $3, TYPE_INT, yylineno))
              fprintf(stderr, "Error semantico en linea %d: '%s' ya habia sido declarada\n", yylineno, $3);
      }
    | GARAGE FUEL IDENT ASSIGN expr SEMI {
          $$ = newDecl(TYPE_FLOAT, $3, $5, yylineno);
          if (!symtab_insert(symtab, $3, TYPE_FLOAT, yylineno))
              fprintf(stderr, "Error semantico en linea %d: '%s' ya habia sido declarada\n", yylineno, $3);
      }
    ;

asignacion:
    IDENT ASSIGN expr SEMI {
        $$ = newAssign($1, $3, yylineno);
        if (!symtab_lookup(symtab, $1))
            fprintf(stderr, "Advertencia en linea %d: '%s' se usa sin haber sido declarada\n", yylineno, $1);
    }
    ;

incremento:
    IDENT BOOST expr SEMI {
        $$ = newBoost($1, $3, yylineno);
        if (!symtab_lookup(symtab, $1))
            fprintf(stderr, "Advertencia en linea %d: '%s' se usa sin haber sido declarada\n", yylineno, $1);
    }
    ;

entrada:
    PITSTOP IDENT SEMI {
        $$ = newInput($2, yylineno);
        if (!symtab_lookup(symtab, $2))
            fprintf(stderr, "Advertencia en linea %d: '%s' se usa sin haber sido declarada\n", yylineno, $2);
    }
    ;

salida:
    RADIO expr SEMI { $$ = newOutput($2, yylineno); }
    ;

condicional:
    RACE LPAREN cond RPAREN LBRACE lista_sent RBRACE opt_overtake {
        $$ = newIf($3, $6, $8, yylineno);
    }
    ;

opt_overtake:
      %empty { $$ = NULL; }
    | OVERTAKE LBRACE lista_sent RBRACE { $$ = $3; }
    ;

ciclo:
    LAP LPAREN cond RPAREN LBRACE lista_sent RBRACE {
        $$ = newWhile($3, $6, yylineno);
    }
    ;

fin_stmt:
    FINISH SEMI { $$ = newFinish(yylineno); }
    ;

cond:
      expr LT expr { $$ = newRelOp("<",  $1, $3, yylineno); }
    | expr GT expr { $$ = newRelOp(">",  $1, $3, yylineno); }
    | expr LE expr { $$ = newRelOp("<=", $1, $3, yylineno); }
    | expr GE expr { $$ = newRelOp(">=", $1, $3, yylineno); }
    | expr EQ expr { $$ = newRelOp("==", $1, $3, yylineno); }
    | expr NE expr { $$ = newRelOp("!=", $1, $3, yylineno); }
    ;

expr:
      expr PLUS term  { $$ = newBinOp("+", $1, $3, yylineno); }
    | expr MINUS term { $$ = newBinOp("-", $1, $3, yylineno); }
    | term             { $$ = $1; }
    ;

term:
      term TIMES factor  { $$ = newBinOp("*", $1, $3, yylineno); }
    | term DIVIDE factor { $$ = newBinOp("/", $1, $3, yylineno); }
    | factor              { $$ = $1; }
    ;

factor:
      ENTERO               { $$ = newIntLit($1, yylineno); }
    | REAL                 { $$ = newFloatLit($1, yylineno); }
    | IDENT                { $$ = newVar($1, yylineno); }
    | LPAREN expr RPAREN   { $$ = $2; }
    | MINUS factor              { $$ = newNeg($2, yylineno); }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Error sintactico en linea %d: %s (cerca de '%s')\n", yylineno, s, yytext);
    synErrorCount++;
}
