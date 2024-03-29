%option noyywrap nodefault yylineno case-insensitive reentrant bison-bridge

%{
#include "midorisql.tab.h"
void yyerror(void*, void*, char *s, ...);
int oldstate;

#define YY_FATAL_ERROR(msg) yyerror(NULL, yyscanner, msg)
%}

%x COMMENT
%s BTWMODE

%%
	/* keywords */

  /* Hack for BETWEEN ... AND ... 
   * return special AND token if BETWEEN seen
   */
<BTWMODE>AND	{ BEGIN INITIAL; return AND; }
AND	{ return ANDOP; }
AS	{ return AS; }
ASC	{ return ASC; }
AUTO_INCREMENT	{ return AUTO_INCREMENT; }
BETWEEN	{ BEGIN BTWMODE; return BETWEEN; }
BY	{ return BY; }
CASE	{ return CASE; }
CHAR(ACTER)?	{ return CHAR; }
COLUMN	{ return COLUMN; }
COMMENT	{ return COMMENT; }
CREATE	{ return CREATE; }
CROSS	{ return CROSS; }
CURRENT_DATE	{ return CURRENT_DATE; }
DATE	{ return DATE; }
DATETIME	{ return DATETIME; }
DELETE	{ return DELETE; }
DESC	{ return DESC; }
DISTINCT	{ return DISTINCT; }
DOUBLE	{ return DOUBLE; }
ELSE	{ return ELSE; }
END	{ return END; }
EXISTS	{ yylval->subtok = 0; return EXISTS; }
NOT[ \t\n]+EXISTS	{ yylval->subtok = 1; return EXISTS; }
EXPLAIN	{ return EXPLAIN; }
FROM	{ return FROM; }
GROUP	{ return GROUP; }
HAVING	{ return HAVING; }
IF	{ return IF; }
IN	{ return IN; }
INDEX	{ return INDEX; }
INNER	{ return INNER; }
INSERT	{ return INSERT; }
INT4?|INTEGER	{ return INTEGER; }
INTO	{ return INTO; }
IS	{ return IS; }
JOIN	{ return JOIN; }
KEY	{ return KEY; }
LEFT	{ return LEFT; }
LIKE	{ return LIKE; }
LIMIT	{ return LIMIT; }
MOD	{ return MOD; }
NOT	{ return NOT; }
NULL	{ return NULLX; }
ON	{ return ON; }
OR	{ return OR; }
ORDER	{ return ORDER; }
OUTER	{ return OUTER; }
PRIMARY	{ return PRIMARY; }
RIGHT	{ return RIGHT; }
SELECT	{ return SELECT; }
SET	{ return SET; }
TABLE	{ return TABLE; }
TINYINT { return TINYINT; }
THEN	{ return THEN; }
UNIQUE	{ return UNIQUE; }
UPDATE	{ return UPDATE; }
VALUES?	{ return VALUES; }
VARCHAR(ACTER)?	{ return VARCHAR; }
WHEN	{ return WHEN; }
WHERE	{ return WHERE; }
XOR	{ return XOR; }

   /* numbers */

-?[0-9]+	        { yylval->intval = atoi(yytext); return INTNUM; } 

-?[0-9]+"."[0-9]* |
-?"."[0-9]+	|
-?[0-9]+E[-+]?[0-9]+	|
-?[0-9]+"."[0-9]*E[-+]?[0-9]+ |
-?"."[0-9]*E[-+]?[0-9]+	{ yylval->floatval = atof(yytext) ;
                                  return APPROXNUM; }
    /* booleans */
TRUE	{ yylval->intval = 1; return BOOL; }
UNKNOWN	{ yylval->intval = -1; return BOOL; }
FALSE	{ yylval->intval = 0; return BOOL; }

   /* strings */

'(\\.|''|[^'\n])*'	|
\"(\\.|\"\"|[^"\n])*\"  { yylval->strval = strdup(yytext); return STRING; }

'(\\.|[^'\n])*$	        { yyerror(NULL, yyscanner, "Unterminated string %s", yytext); }
\"(\\.|[^"\n])*$	{ yyerror(NULL, yyscanner, "Unterminated string %s", yytext); }

   /* hex strings */
X'[0-9A-F]+' |  
0X[0-9A-F]+  { yylval->strval = strdup(yytext); return STRING; }

   /* bit strings */

0B[01]+      |
B'[01]+'     { yylval->strval = strdup(yytext); return STRING; }


   /* operators */
[-+&~|^/%*(),.;!]   { return yytext[0]; }

"&&"		{ return ANDOP; }
"||"            { return OR; }

"="	{ yylval->subtok = 4; return COMPARISON; }
">="	{ yylval->subtok = 6; return COMPARISON; }
">"	{ yylval->subtok = 2; return COMPARISON; }
"<="	{ yylval->subtok = 5; return COMPARISON; }
"<"	{ yylval->subtok = 1; return COMPARISON; }
"!="	|
"<>"	{ yylval->subtok = 3; return COMPARISON; }

"<<"	{ yylval->subtok = 1; return SHIFT; }
">>"	{ yylval->subtok = 2; return SHIFT; }

        /* functions */


	/* 
         * peek ahead and return function if name(
         */
COUNT    { int c = input(yyscanner); unput(c);
           if(c == '(') {return FCOUNT;}
           yylval->strval = strdup(yytext);
           return NAME; }

[A-Za-z][A-Za-z0-9_]*	{ yylval->strval = strdup(yytext);
                          return NAME; }
`[^`/\\.\n]+`           { yylval->strval = strdup(yytext+1);
                          yylval->strval[yyleng-2] = 0;
                          return NAME; }

`[^`\n]*$               { yyerror(NULL, yyscanner, "unterminated quoted name %s", yytext); }


	/* comments */   
#.*		;
"--"[ \t].*	;

"/*"            { oldstate = YY_START; BEGIN COMMENT; }
<COMMENT>"*/"   { BEGIN oldstate; }
<COMMENT>.|\n   ;
<COMMENT><<EOF>> { yyerror(NULL, yyscanner, "unclosed comment"); }

	/* everything else */
[ \t\n]         /* white space */
.               { yyerror(NULL, yyscanner, "mystery character '%c'", *yytext); }

%%
