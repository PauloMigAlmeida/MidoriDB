/* generate much more meaningful errors rather than the
   uninformative string "syntax error" */
%define parse.error verbose

%{
#include <datastructure/vector.h>

void yyerror(struct vector *vec, const char *s, ...);
bool emit(struct vector *vec, char *s, ...);
int yylex(void);
void flex_scan_string(const char* in);
void flex_delete_buffer(void);
%}

%code requires { 
#include <compiler/common.h>
#include <datastructure/vector.h>
}

/* 
   Add another argument in yyparse() so that we
   can communicate the parsed result to the caller.
   We can't return the result directly, since the
   return value is already reserved as an int, with
   0=success, 1=error, 2=nomem
*/

%parse-param {struct vector *result}

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

stmt: create_table_stmt { emit(result, "STMT"); }
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

void yyerror(struct vector *vec, const char *s, ...)
{
  char buf[256];
  va_list ap;
  
  va_start(ap, s);
  
  /* if the error happened at the lexical phase then 
     we print it to stderr as there is no vector ref yet */
  if(!vec){
  	vfprintf(stderr, s, ap);  
  	fprintf(stderr, "\n");
  }else {
  	sprintf(buf, s, ap);
  	
  	vector_clear(vec);
  
  	/* although unlikely, if we fail to push content to the vector
	   so other program can read the error message, then we fail 
	   over to the stderr */
  	if(!vector_push(vec, buf, strlen(buf))){
  		vfprintf(stderr, s, ap);  
  		fprintf(stderr, "\n");
  	}
  }    
}

bool emit(struct vector *vec, char *s, ...)
{
  char buf[256];
  va_list ap;
  
  va_start(ap, s);
  sprintf(buf, s, ap);
  
  return vector_push(vec, buf, strlen(buf));  
}

int bison_parse_string(const char* in, struct vector *out) {
  flex_scan_string(in);
  int rv = yyparse(out);
  flex_delete_buffer();
  return rv;
}


