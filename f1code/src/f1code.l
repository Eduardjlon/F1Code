%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "f1code.tab.h"

extern int lexErrorCount;
%}

%option noyywrap
%option yylineno

DIGIT   [0-9]
ID      [a-zA-Z_][a-zA-Z0-9_]*

%%

"//".*                              { /* comentario de una linea: se ignora */ }
"/*"([^*]|\*+[^*/])*\*+"/"          { /* comentario de bloque: se ignora */ }

"garage"    { return GARAGE; }
"engine"    { return ENGINE; }
"fuel"      { return FUEL; }
"pitstop"   { return PITSTOP; }
"radio"     { return RADIO; }
"race"      { return RACE; }
"overtake"  { return OVERTAKE; }
"lap"       { return LAP; }
"boost"     { return BOOST; }
"finish"    { return FINISH; }

":="  { return ASSIGN; }
"<="  { return LE; }
">="  { return GE; }
"=="  { return EQ; }
"!="  { return NE; }
"<"   { return LT; }
">"   { return GT; }
"+"   { return PLUS; }
"-"   { return MINUS; }
"*"   { return TIMES; }
"/"   { return DIVIDE; }
"("   { return LPAREN; }
")"   { return RPAREN; }
"{"   { return LBRACE; }
"}"   { return RBRACE; }
";"   { return SEMI; }

{DIGIT}+"."{DIGIT}+   { yylval.fval = atof(yytext); return REAL; }
{DIGIT}+              { yylval.ival = atoi(yytext); return ENTERO; }
{ID}                  { yylval.sval = strdup(yytext); return IDENT; }

[ \t\r\n]+   { /* espacios en blanco: se ignoran */ }

.  {
      fprintf(stderr, "Error lexico en linea %d: caracter no reconocido '%s'\n", yylineno, yytext);
      lexErrorCount++;
   }

%%
