%{
#include "midorisql.tab.h"
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
%%

