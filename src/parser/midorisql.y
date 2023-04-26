%{ 
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void yyerror(char *s, ...);
int yylex(void);
void flex_scan_string(const char* in);
void flex_delete_buffer(void);
%}

%union {
	int intval;
	char* strval;
}

%token CREATE
%token TABLE
%token INTEGER

%token <strval> NAME

%start stmt_list

%%

stmt_list: stmt ';'
		 | stmt_list stmt ';'
		 ;

stmt: create_table_stmt
    ;

create_table_stmt: CREATE TABLE NAME '(' create_col_list ')'
		 ;

create_col_list: create_definition
	       | create_col_list ',' create_definition
	       ;

create_definition: NAME data_type
		 ;

data_type: INTEGER
	 ;

%%

void yyerror(char *s, ...)
{
  extern int yylineno;

  va_list ap;
  va_start(ap, s);

  fprintf(stderr, "%d: error: ", yylineno);
  vfprintf(stderr, s, ap);
  fprintf(stderr, "\n");
}

int bison_parse_string(const char* in) {
  flex_scan_string(in);
  int rv = yyparse();
  flex_delete_buffer();
  return rv;
}


