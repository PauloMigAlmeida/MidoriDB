%option noyywrap nodefault yylineno case-insensitive nounput noinput

%{
#include "midorisql.tab.h"
void yyerror(char *s, ...);
%}

%%

	/* keywords */
CREATE	{ return CREATE; }
TABLE	{ return TABLE; }
INTEGER	{ return INTEGER; }

[A-Za-z][A-Za-z0-9_]*   { 
				yylval.strval = strdup(yytext);
				return NAME;
			}
			
	/* operators */
[-+&~|^/%*(),.;!]   	{ return yytext[0]; }

	/* everything else */
[ \t\n]         	/* white space */
.               	{ yyerror("mystery character '%c'", *yytext); }
%%


/*
 in the docs, it explicitly says no to rely on functions that start with
 yy_ as their signature can change at any moment. So I'm creating this 
 wrapper to account for some of those issue (not all)
*/
void flex_scan_string(const char* in) {
  yy_scan_string(in);
}

void flex_delete_buffer(void) {
  yy_delete_buffer(YY_CURRENT_BUFFER);
}