%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "f1code.tab.h"

extern int lexErrorCount;
static int commentLine = 0;

/* Cada token guarda su linea en yylloc (la usan el AST y los errores) y se
   devuelve al parser. Espacios y comentarios no tocan yylloc: asi un ';'
   faltante al final se reporta en la linea del ultimo token. */
#define TOKEN(t) do { yylloc.first_line = yylloc.last_line = yylineno; return (t); } while (0)
%}

%option noyywrap
%option yylineno
%option nounput noinput
%x COMMENT

DIGIT   [0-9]
ID      [a-zA-Z_][a-zA-Z0-9_]*

%%

"//".*                  { /* comentario de una linea: se ignora */ }
"/*"                    { commentLine = yylineno; BEGIN(COMMENT); }
<COMMENT>"*/"           { BEGIN(INITIAL); }
<COMMENT>[^*\n]+        { /* contenido del comentario de bloque */ }
<COMMENT>"*"            { }
<COMMENT>\n             { }
<COMMENT><<EOF>>        {
                            fprintf(stderr, "Error lexico en linea %d: comentario de bloque sin cerrar\n", commentLine);
                            lexErrorCount++;
                            BEGIN(INITIAL);
                            yyterminate();
                        }

"garage"    { TOKEN(GARAGE); }
"engine"    { TOKEN(ENGINE); }
"fuel"      { TOKEN(FUEL); }
"pitstop"   { TOKEN(PITSTOP); }
"radio"     { TOKEN(RADIO); }
"race"      { TOKEN(RACE); }
"overtake"  { TOKEN(OVERTAKE); }
"lap"       { TOKEN(LAP); }
"boost"     { TOKEN(BOOST); }
"finish"    { TOKEN(FINISH); }

":="  { TOKEN(ASSIGN); }
"<="  { TOKEN(LE); }
">="  { TOKEN(GE); }
"=="  { TOKEN(EQ); }
"!="  { TOKEN(NE); }
"<"   { TOKEN(LT); }
">"   { TOKEN(GT); }
"+"   { TOKEN(PLUS); }
"-"   { TOKEN(MINUS); }
"*"   { TOKEN(TIMES); }
"/"   { TOKEN(DIVIDE); }
"("   { TOKEN(LPAREN); }
")"   { TOKEN(RPAREN); }
"{"   { TOKEN(LBRACE); }
"}"   { TOKEN(RBRACE); }
";"   { TOKEN(SEMI); }

{DIGIT}+"."{DIGIT}+   { yylval.fval = atof(yytext); TOKEN(REAL); }
{DIGIT}+{ID}          {
                          fprintf(stderr, "Error lexico en linea %d: el identificador '%s' no puede iniciar con un digito\n",
                                  yylineno, yytext);
                          lexErrorCount++;
                          yylval.sval = strdup(yytext);
                          TOKEN(IDENT);   /* se sigue analizando para reportar mas errores */
                      }
{DIGIT}+              { yylval.ival = atoi(yytext); TOKEN(ENTERO); }
{ID}                  { yylval.sval = strdup(yytext); TOKEN(IDENT); }

[ \t\r\n]+   { /* espacios en blanco: se ignoran */ }

.  {
      fprintf(stderr, "Error lexico en linea %d: caracter no reconocido '%s'\n", yylineno, yytext);
      lexErrorCount++;
   }

%%
