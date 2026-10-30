%option noyywrap
%{
#include "parser.tab.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
%}

DIGIT    [0-9]
ID       [a-zA-Z_][a-zA-Z0-9_]*

%%
"int"                   { return INT; }
"if"                    { return IF; }
"else"                  { return ELSE; }
"while"                 { return WHILE; }
"<="                    { return LE; }
">="                    { return GE; }
"=="                    { return EQ; }
"!="                    { return NE; }
"&&"                    { return AND; }
"||"                    { return OR; }
{DIGIT}+                { yylval.integer = std::atoi(yytext); return INTEGER; }
{ID}                    { yylval.text = strdup(yytext); return IDENTIFIER; }
[ \t\r\n]+              { /* skip whitespace */ }
"//"[^\n]*              { /* skip C++-style line comments */ }
"/*"([^*]|\*+[^*/])*\*+"/" { /* skip C-style block comments */ }
[+*/%(){};=<>!-]        { return yytext[0]; }
.                       { std::fprintf(stderr, "Unexpected character: %s\n", yytext); return yytext[0]; }
%%
