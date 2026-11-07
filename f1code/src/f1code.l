%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "comentarios.h"
#include "f1code.tab.h"

/* La lista de comentarios se compila junto con el lexer, que es quien la llena
   (asi no hace falta agregar un objeto nuevo al Makefile). */
#include "comentarios.c"

extern int lexErrorCount;
static int commentLine = 0;

/* Texto del comentario de bloque que se esta leyendo (se guarda completo) */
static char *bloque = NULL;
static size_t bloqueLen = 0, bloqueCap = 0;
static void bloqueAgregar(const char *s, size_t n) {
    if (bloqueLen + n + 1 > bloqueCap) {
        bloqueCap = (bloqueLen + n + 1) * 2;
        bloque = realloc(bloque, bloqueCap);
        if (!bloque) exit(2);
    }
    memcpy(bloque + bloqueLen, s, n);
    bloqueLen += n;
    bloque[bloqueLen] = '\0';
}

/* Cada token guarda su linea en yylloc (la usan el AST y los errores) y se
   devuelve al parser. Espacios y comentarios no tocan yylloc: asi un ';'
   faltante al final se reporta en la linea del ultimo token. */
#define TOKEN(t) do { yylloc.first_line = yylloc.last_line = yylineno; return (t); } while (0)

/* Copia el contenido de un literal de texto sin las comillas */
static char *sinComillas(const char *s, int len) {
    char *r = malloc((size_t)len - 1);
    if (!r) exit(2);
    memcpy(r, s + 1, (size_t)len - 2);
    r[len - 2] = '\0';
    return r;
}
%}

%option noyywrap
%option yylineno
%option nounput noinput
%x COMMENT

DIGIT   [0-9]
ID      [a-zA-Z_][a-zA-Z0-9_]*

%%

  /* Los comentarios no son tokens, pero se guardan para copiarlos al .and */
"//".*                  { comentarioAgregar(yylineno, yytext, (size_t)yyleng); }
"/*"                    { commentLine = yylineno; bloqueLen = 0; bloqueAgregar("/*", 2); BEGIN(COMMENT); }
<COMMENT>"*/"           {
                            bloqueAgregar("*/", 2);
                            comentarioAgregar(commentLine, bloque, bloqueLen);
                            BEGIN(INITIAL);
                        }
<COMMENT>[^*\n]+        { bloqueAgregar(yytext, (size_t)yyleng); }
<COMMENT>"*"            { bloqueAgregar("*", 1); }
<COMMENT>\n             { bloqueAgregar("\n", 1); }
<COMMENT><<EOF>>        {
                            fprintf(stderr, "Error lexico en linea %d: comentario de bloque sin cerrar\n", commentLine);
                            lexErrorCount++;
                            BEGIN(INITIAL);
                            yyterminate();
                        }

"garage"    { TOKEN(GARAGE); }
"engine"    { TOKEN(ENGINE); }
"fuel"      { TOKEN(FUEL); }
"chassis"   { TOKEN(CHASSIS); }   /* declaracion generica: el tipo se infiere */
"pilot"     { TOKEN(PILOT); }     /* tipo texto */
"pitstop"   { TOKEN(PITSTOP); }
"radio"     { TOKEN(RADIO); }
"race"      { TOKEN(RACE); }
"overtake"  { TOKEN(OVERTAKE); }
"lap"       { TOKEN(LAP); }
"stint"     { TOKEN(STINT); }     /* ciclo para (for) */
"boost"     { TOKEN(BOOST); }
"finish"    { TOKEN(FINISH); }
"podium"    { TOKEN(PODIUM); }    /* retorno: termina el programa entregando un valor */

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

  /* Texto entre comillas simples o dobles. Las secuencias de escape (\n, \t, \", \') se
     conservan tal cual para que lleguen igual al C11 del final de la cadena. */
\'([^'\\\n]|\\.)*\'    { yylval.sval = sinComillas(yytext, yyleng); TOKEN(TEXTO); }
\"([^"\\\n]|\\.)*\"    { yylval.sval = sinComillas(yytext, yyleng); TOKEN(TEXTO); }
\'([^'\\\n]|\\.)*|\"([^"\\\n]|\\.)*  {
                          fprintf(stderr, "Error lexico en linea %d: texto sin cerrar %s\n", yylineno, yytext);
                          lexErrorCount++;
                      }

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
