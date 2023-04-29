/* generate much more meaningful errors rather than the
   uninformative string "syntax error" */
%define parse.error verbose

%define api.pure true

%{
#include <datastructure/vector.h>

void yyerror(struct vector *vec, const char *s, ...);
bool emit(struct vector *vec, char *s, ...);
int yylex(void*, void*);
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

/* add lexer parameter so it becomes reentrant (thread-safe) */
%param {void *scanner}

%union {
	int intval;
	double floatval;
	char *strval;
	int subtok;
}

/* names and literal values */

%token <strval> NAME
%token <strval> STRING
%token <intval> INTNUM
%token <intval> BOOL
%token <floatval> APPROXNUM

/* operators and precedence levels */

%right ASSIGN
%left OR
%left XOR
%left ANDOP
%nonassoc IN IS LIKE REGEXP
%left NOT '!'
%left BETWEEN
%left <subtok> COMPARISON /* = <> < > <= >= <=> */
%left '|'
%left '&'
%left <subtok> SHIFT /* << >> */
%left '+' '-'
%left '*' '/' '%' MOD
%left '^'
%nonassoc UMINUS

%token ALL
%token AND
%token AS
%token ASC
%token AUTO_INCREMENT
%token BETWEEN
%token BY
%token CASE
%token CHAR
%token COLUMN
%token CREATE
%token CROSS
%token CURRENT_DATE
%token CURRENT_TIME
%token CURRENT_TIMESTAMP
%token DATE
%token DATETIME
%token DEFAULT
%token DELETE
%token DESC
%token DISTINCT
%token DOUBLE
%token ELSE
%token END
%token <subtok> EXISTS
%token EXPLAIN
%token FLOAT
%token FROM
%token GROUP
%token HAVING
%token IF
%token IN
%token INDEX
%token INNER
%token INSERT
%token INT
%token INTEGER
%token INTO
%token JOIN
%token KEY
%token LEFT
%token LIKE
%token LIMIT
%token MOD
%token NOT
%token NULLX
%token ON
%token OR
%token ORDER
%token OUTER
%token PRIMARY
%token REGEXP
%token RIGHT
%token SELECT
%token SET
%token TABLE
%token THEN
%token TIME
%token TIMESTAMP
%token UNIQUE
%token UPDATE
%token VALUES
%token VARCHAR
%token WHEN
%token WHERE
%token XOR

/* functions with special syntax */
%token FCOUNT

%type <intval> select_opts select_expr_list
%type <intval> val_list opt_val_list case_list
%type <intval> groupby_list opt_asc_desc
%type <intval> table_references opt_inner_cross opt_outer
%type <intval> left_or_right column_list

%type <intval> insert_vals insert_vals_list
%type <intval> opt_if_not_exists update_asgn_list
%type <intval> opt_length
%type <intval> column_atts data_type create_col_list

%start stmt_list

%destructor { free($$); } <strval>

%%

stmt_list: stmt ';'
  | stmt_list stmt ';'
  ;

   /* statements: select statement */

stmt: select_stmt { emit(result, "STMT"); }
   ;

select_stmt: SELECT select_opts select_expr_list
                        { emit(result, "SELECTNODATA %d %d", $2, $3); } 
    | SELECT select_opts select_expr_list
     FROM table_references
     opt_where opt_groupby opt_having opt_orderby opt_limit
     opt_into_list { emit(result, "SELECT %d %d %d", $2, $3, $5); } ;
;

opt_where: /* nil */ 
   | WHERE expr { emit(result, "WHERE"); };

opt_groupby: /* nil */ 
   | GROUP BY groupby_list	{ emit(result, "GROUPBYLIST %d", $3); }
;

groupby_list: expr opt_asc_desc
                             { emit(result, "GROUPBY %d",  $2); $$ = 1; }
   | groupby_list ',' expr opt_asc_desc
                             { emit(result, "GROUPBY %d",  $4); $$ = $1 + 1; }
   ;

opt_asc_desc: /* nil */ { $$ = 0; }
   | ASC                { $$ = 0; }
   | DESC               { $$ = 1; }
   ;

opt_having: /* nil */ | HAVING expr { emit(result, "HAVING"); };

opt_orderby: /* nil */ | ORDER BY groupby_list { emit(result, "ORDERBY %d", $3); }
   ;

opt_limit: /* nil */ | LIMIT expr { emit(result, "LIMIT 1"); }
  | LIMIT expr ',' expr             { emit(result, "LIMIT 2"); }
  ; 

opt_into_list: /* nil */ 
   | INTO column_list { emit(result, "INTO %d", $2); }
   ;

column_list: NAME { emit(result, "COLUMN %s", $1); free($1); $$ = 1; }
  | column_list ',' NAME  { emit(result, "COLUMN %s", $3); free($3); $$ = $1 + 1; }
  ;

select_opts:                          { $$ = 0; }
| select_opts ALL                 { if($$ & 01) yyerror(result, "duplicate ALL option"); $$ = $1 | 01; }
| select_opts DISTINCT            { if($$ & 02) yyerror(result, "duplicate DISTINCT option"); $$ = $1 | 02; }
    ;

select_expr_list: select_expr { $$ = 1; }
    | select_expr_list ',' select_expr {$$ = $1 + 1; }
    | '*' { emit(result, "SELECTALL"); $$ = 1; }
    ;

select_expr: expr opt_as_alias ;

table_references:    table_reference { $$ = 1; }
    | table_references ',' table_reference { $$ = $1 + 1; }
    ;

table_reference:  table_factor
  | join_table
  ;

table_factor:
    NAME opt_as_alias { emit(result, "TABLE %s", $1); free($1); }
  | NAME '.' NAME opt_as_alias { emit(result, "TABLE %s.%s", $1, $3);
                               free($1); free($3); }
  | table_subquery opt_as NAME { emit(result, "SUBQUERYAS %s", $3); free($3); }
  | '(' table_references ')' { emit(result, "TABLEREFERENCES %d", $2); }
  ;

opt_as: AS 
  | /* nil */
  ;

opt_as_alias: AS NAME { emit (result, "ALIAS %s", $2); free($2); }
  | NAME              { emit (result, "ALIAS %s", $1); free($1); }
  | /* nil */
  ;

join_table:
    table_reference opt_inner_cross JOIN table_factor opt_join_condition
                  { emit(result, "JOIN %d", 0100+$2); }
  | table_reference left_or_right opt_outer JOIN table_factor join_condition
                  { emit(result, "JOIN %d", 0300+$2+$3); }
  ;

opt_inner_cross: /* nil */ { $$ = 0; }
   | INNER { $$ = 1; }
   | CROSS  { $$ = 2; }
;

opt_outer: /* nil */  { $$ = 0; }
   | OUTER {$$ = 4; }
   ;

left_or_right: LEFT { $$ = 1; }
    | RIGHT { $$ = 2; }
    ;

opt_join_condition: join_condition | /* nil */ ;

join_condition:
    ON expr { emit(result, "ONEXPR"); }
    ;

table_subquery: '(' select_stmt ')' { emit(result, "SUBQUERY"); }
   ;

   /* statements: delete statement */

stmt: delete_stmt { emit(result, "STMT"); }
   ;

delete_stmt: DELETE FROM NAME
    opt_where opt_orderby opt_limit
                  { emit(result, "DELETEONE %s", $3); free($3); }
;

   /* statements: insert statement */

stmt: insert_stmt { emit(result, "STMT"); }
   ;

insert_stmt: INSERT opt_into NAME
     opt_col_names
     VALUES insert_vals_list { emit(result, "INSERTVALS %d %s", $6, $3); free($3); }
   ;

opt_into: INTO | /* nil */
   ;

opt_col_names: /* nil */
   | '(' column_list ')' { emit(result, "INSERTCOLS %d", $2); }
   ;

insert_vals_list: '(' insert_vals ')' { emit(result, "VALUES %d", $2); $$ = 1; }
   | insert_vals_list ',' '(' insert_vals ')' { emit(result, "VALUES %d", $4); $$ = $1 + 1; }

insert_vals:
     expr { $$ = 1; }
   | DEFAULT { emit(result, "DEFAULT"); $$ = 1; }
   | insert_vals ',' expr { $$ = $1 + 1; }
   | insert_vals ',' DEFAULT { emit(result, "DEFAULT"); $$ = $1 + 1; }
   ;

insert_stmt: INSERT opt_into NAME opt_col_names
    select_stmt { emit(result, "INSERTSELECT %s", $3); free($3); }
  ;

/** update **/
stmt: update_stmt { emit(result, "STMT"); }
   ;

update_stmt: UPDATE table_references
    SET update_asgn_list
    opt_where
    opt_orderby
opt_limit { emit(result, "UPDATE %d %d", $2, $4); }
;

update_asgn_list:
     NAME COMPARISON expr 
       { if ($2 != 4) yyerror(result, "bad insert assignment to %s", $1);
	 emit(result, "ASSIGN %s", $1); free($1); $$ = 1; }
   | NAME '.' NAME COMPARISON expr 
       { if ($4 != 4) yyerror(result, "bad insert assignment to %s", $1);
	 emit(result, "ASSIGN %s.%s", $1, $3); free($1); free($3); $$ = 1; }
   | update_asgn_list ',' NAME COMPARISON expr
       { if ($4 != 4) yyerror(result, "bad insert assignment to %s", $3);
	 emit(result, "ASSIGN %s.%s", $3); free($3); $$ = $1 + 1; }
   | update_asgn_list ',' NAME '.' NAME COMPARISON expr
       { if ($6 != 4) yyerror(result, "bad insert assignment to %s.$s", $3, $5);
	 emit(result, "ASSIGN %s.%s", $3, $5); free($3); free($5); $$ = 1; }
   ;


   /** create table **/

opt_if_not_exists:  /* nil */ { $$ = 0; }
   | IF EXISTS           { if(!$2) yyerror(result, "IF EXISTS doesn't exist");
                        $$ = $2; /* NOT EXISTS hack */ }
   ;

stmt: create_table_stmt { emit(result, "STMT"); }
   ;

create_table_stmt: CREATE TABLE opt_if_not_exists NAME
   '(' create_col_list ')' { emit(result, "CREATE %d %d %s", $3, $6, $4); free($4); }
   ;

create_table_stmt: CREATE TABLE opt_if_not_exists NAME '.' NAME
   '(' create_col_list ')' { emit(result, "CREATE %d %d %s.%s", $3, $8, $4, $6);
                          free($4); free($6); }
   ;

create_col_list: create_definition { $$ = 1; }
    | create_col_list ',' create_definition { $$ = $1 + 1; }
    ;

create_definition: { emit(result, "STARTCOL"); } NAME data_type column_atts
                   { emit(result, "COLUMNDEF %d %s", $3, $2); free($2); }

    | PRIMARY KEY '(' column_list ')'    { emit(result, "PRIKEY %d", $4); }
    | KEY '(' column_list ')'            { emit(result, "KEY %d", $3); }
    | INDEX '(' column_list ')'          { emit(result, "KEY %d", $3); }
    ;

column_atts: /* nil */ { $$ = 0; }
    | column_atts NOT NULLX             { emit(result, "ATTR NOTNULL"); $$ = $1 + 1; }
    | column_atts NULLX
    | column_atts DEFAULT STRING        { emit(result, "ATTR DEFAULT STRING %s", $3); free($3); $$ = $1 + 1; }
    | column_atts DEFAULT INTNUM        { emit(result, "ATTR DEFAULT NUMBER %d", $3); $$ = $1 + 1; }
    | column_atts DEFAULT APPROXNUM     { emit(result, "ATTR DEFAULT FLOAT %g", $3); $$ = $1 + 1; }
    | column_atts DEFAULT BOOL          { emit(result, "ATTR DEFAULT BOOL %d", $3); $$ = $1 + 1; }
    | column_atts AUTO_INCREMENT        { emit(result, "ATTR AUTOINC"); $$ = $1 + 1; }
    | column_atts UNIQUE '(' column_list ')' { emit(result, "ATTR UNIQUEKEY %d", $4); $$ = $1 + 1; }
    | column_atts UNIQUE KEY { emit(result, "ATTR UNIQUEKEY"); $$ = $1 + 1; }
    | column_atts PRIMARY KEY { emit(result, "ATTR PRIKEY"); $$ = $1 + 1; }
    | column_atts KEY { emit(result, "ATTR PRIKEY"); $$ = $1 + 1; }
    ;

opt_length: /* nil */ { $$ = 0; }
   | '(' INTNUM ')' { $$ = $2; }
   | '(' INTNUM ',' INTNUM ')' { $$ = $2 + 1000*$4; }
   ;

data_type:
     INT opt_length { $$ = 40000 + $2; }
   | INTEGER opt_length { $$ = 50000 + $2; }
   | DOUBLE opt_length { $$ = 80000 + $2; }
   | FLOAT opt_length { $$ = 90000 + $2; }
   | DATE { $$ = 100001; }
   | TIME { $$ = 100002; }
   | TIMESTAMP { $$ = 100003; }
   | DATETIME { $$ = 100004; }
   | CHAR opt_length { $$ = 120000 + $2; }
   | VARCHAR '(' INTNUM ')' { $$ = 130000 + $3; }
   ;

   /**** expressions ****/

expr: NAME          { emit(result, "NAME %s", $1); free($1); }
   | NAME '.' NAME { emit(result, "FIELDNAME %s.%s", $1, $3); free($1); free($3); }
   | STRING        { emit(result, "STRING %s", $1); free($1); }
   | INTNUM        { emit(result, "NUMBER %d", $1); }
   | APPROXNUM     { emit(result, "FLOAT %g", $1); }
   | BOOL          { emit(result, "BOOL %d", $1); }
   ;

expr: expr '+' expr { emit(result, "ADD"); }
   | expr '-' expr { emit(result, "SUB"); }
   | expr '*' expr { emit(result, "MUL"); }
   | expr '/' expr { emit(result, "DIV"); }
   | expr '%' expr { emit(result, "MOD"); }
   | expr MOD expr { emit(result, "MOD"); }
   | '-' expr %prec UMINUS { emit(result, "NEG"); }
   | expr ANDOP expr { emit(result, "AND"); }
   | expr OR expr { emit(result, "OR"); }
   | expr XOR expr { emit(result, "XOR"); }
   | expr COMPARISON expr { emit(result, "CMP %d", $2); }
   | expr COMPARISON '(' select_stmt ')' { emit(result, "CMPSELECT %d", $2); }
   | expr '|' expr { emit(result, "BITOR"); }
   | expr '&' expr { emit(result, "BITAND"); }
   | expr '^' expr { emit(result, "BITXOR"); }
   | expr SHIFT expr { emit(result, "SHIFT %s", $2==1?"left":"right"); }
   | NOT expr { emit(result, "NOT"); }
   | '!' expr { emit(result, "NOT"); }
   ;    

expr:  expr IS NULLX     { emit(result, "ISNULL"); }
   |   expr IS NOT NULLX { emit(result, "ISNULL"); emit(result, "NOT"); }
   |   expr IS BOOL      { emit(result, "ISBOOL %d", $3); }
   |   expr IS NOT BOOL  { emit(result, "ISBOOL %d", $4); emit(result, "NOT"); }
   ;

expr: expr BETWEEN expr AND expr %prec BETWEEN { emit(result, "BETWEEN"); }
   ;


val_list: expr { $$ = 1; }
   | expr ',' val_list { $$ = 1 + $3; }
   ;

opt_val_list: /* nil */ { $$ = 0; }
   | val_list
   ;

expr: expr IN '(' val_list ')'       { emit(result, "ISIN %d", $4); }
   | expr NOT IN '(' val_list ')'    { emit(result, "ISIN %d", $5); emit(result, "NOT"); }
   | expr IN '(' select_stmt ')'     { emit(result, "INSELECT"); }
   | expr NOT IN '(' select_stmt ')' { emit(result, "INSELECT"); emit(result, "NOT"); }
   | EXISTS '(' select_stmt ')'      { emit(result, "EXISTS"); if($1) emit(result, "NOT"); }
   ;

expr: NAME '(' opt_val_list ')' {  emit(result, "CALL %d %s", $3, $1); free($1); }
   ;

  /* functions with special syntax */
expr: FCOUNT '(' '*' ')' { emit(result, "COUNTALL"); }
   | FCOUNT '(' expr ')' { emit(result, " CALL 1 COUNT"); } 

expr: CASE expr case_list END           { emit(result, "CASEVAL %d 0", $3); }
   |  CASE expr case_list ELSE expr END { emit(result, "CASEVAL %d 1", $3); }
   |  CASE case_list END                { emit(result, "CASE %d 0", $2); }
   |  CASE case_list ELSE expr END      { emit(result, "CASE %d 1", $2); }
   ;

case_list: WHEN expr THEN expr     { $$ = 1; }
         | case_list WHEN expr THEN expr { $$ = $1+1; } 
   ;

expr: expr LIKE expr { emit(result, "LIKE"); }
   | expr NOT LIKE expr { emit(result, "LIKE"); emit(result, "NOT"); }
   ;

expr: expr REGEXP expr { emit(result, "REGEXP"); }
   | expr NOT REGEXP expr { emit(result, "REGEXP"); emit(result, "NOT"); }
   ;

expr: CURRENT_TIMESTAMP { emit(result, "NOW"); };
   | CURRENT_DATE	{ emit(result, "NOW"); };
   | CURRENT_TIME	{ emit(result, "NOW"); };
   ;

%%

void yyerror(struct vector *vec, const char *s, ...)
{
	char buf[256];
	va_list ap;
	
	memzero(buf, sizeof(buf));
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
	
	va_end(ap);
}

bool emit(struct vector *vec, char *s, ...)
{
	char buf[256];
	char space = ' ';
	va_list ap;
	bool ret;
	
	memzero(buf, sizeof(buf));
	va_start(ap, s);
	vsnprintf(buf,sizeof(buf), s, ap);
	va_end(ap);
	
	ret = vector_push(vec, buf, strlen(buf));
	ret = ret && vector_push(vec, &space, sizeof(space));
	return ret;
}
