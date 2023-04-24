%{ 
#include <stdio.h>
void yyerror(char *s);
int yylex(void);
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
		 | error ';'
		 | stmt_list error ';'
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

void yyerror(char *s)
{
	fprintf(stderr, "error: %s\n",s);
}
