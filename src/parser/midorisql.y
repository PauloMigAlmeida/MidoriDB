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

       /* user @abc names */

%token <strval> USERVAR

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

%token ADD
%token ALL
%token ALTER
%token ANALYZE
%token AND
%token ANY
%token AS
%token ASC
%token AUTO_INCREMENT
%token BEFORE
%token BETWEEN
%token BIGINT
%token BINARY
%token BIT
%token BLOB
%token BOTH
%token BY
%token CALL
%token CASCADE
%token CASE
%token CHANGE
%token CHAR
%token CHECK
%token COLLATE
%token COLUMN
%token COMMENT
%token CONDITION
%token CONSTRAINT
%token CONTINUE
%token CONVERT
%token CREATE
%token CROSS
%token CURRENT_DATE
%token CURRENT_TIME
%token CURRENT_TIMESTAMP
%token CURRENT_USER
%token CURSOR
%token DATABASE
%token DATABASES
%token DATE
%token DATETIME
%token DAY_HOUR
%token DAY_MICROSECOND
%token DAY_MINUTE
%token DAY_SECOND
%token DECIMAL
%token DECLARE
%token DEFAULT
%token DELAYED
%token DELETE
%token DESC
%token DESCRIBE
%token DETERMINISTIC
%token DISTINCT
%token DISTINCTROW
%token DIV
%token DOUBLE
%token DROP
%token DUAL
%token EACH
%token ELSE
%token ELSEIF
%token ENCLOSED
%token END
%token ENUM
%token ESCAPED
%token <subtok> EXISTS
%token EXIT
%token EXPLAIN
%token FETCH
%token FLOAT
%token FOR
%token FORCE
%token FOREIGN
%token FROM
%token FULLTEXT
%token GRANT
%token GROUP
%token HAVING
%token HIGH_PRIORITY
%token HOUR_MICROSECOND
%token HOUR_MINUTE
%token HOUR_SECOND
%token IF
%token IGNORE
%token IN
%token INDEX
%token INFILE
%token INNER
%token INOUT
%token INSENSITIVE
%token INSERT
%token INT
%token INTEGER
%token INTERVAL
%token INTO
%token ITERATE
%token JOIN
%token KEY
%token KEYS
%token KILL
%token LEADING
%token LEAVE
%token LEFT
%token LIKE
%token LIMIT
%token LINES
%token LOAD
%token LOCALTIME
%token LOCALTIMESTAMP
%token LOCK
%token LONG
%token LONGBLOB
%token LONGTEXT
%token LOOP
%token LOW_PRIORITY
%token MATCH
%token MEDIUMBLOB
%token MEDIUMINT
%token MEDIUMTEXT
%token MINUTE_MICROSECOND
%token MINUTE_SECOND
%token MOD
%token MODIFIES
%token NATURAL
%token NOT
%token NO_WRITE_TO_BINLOG
%token NULLX
%token NUMBER
%token ON
%token ONDUPLICATE
%token OPTIMIZE
%token OPTION
%token OPTIONALLY
%token OR
%token ORDER
%token OUT
%token OUTER
%token OUTFILE
%token PRECISION
%token PRIMARY
%token PROCEDURE
%token PURGE
%token QUICK
%token READ
%token READS
%token REAL
%token REFERENCES
%token REGEXP
%token RELEASE
%token RENAME
%token REPEAT
%token REPLACE
%token REQUIRE
%token RESTRICT
%token RETURN
%token REVOKE
%token RIGHT
%token SCHEMA
%token SCHEMAS
%token SECOND_MICROSECOND
%token SELECT
%token SENSITIVE
%token SEPARATOR
%token SET
%token SHOW
%token SMALLINT
%token SOME
%token SONAME
%token SPATIAL
%token SPECIFIC
%token SQL
%token SQLEXCEPTION
%token SQLSTATE
%token SQLWARNING
%token SQL_BIG_RESULT
%token SQL_CALC_FOUND_ROWS
%token SQL_SMALL_RESULT
%token SSL
%token STARTING
%token STRAIGHT_JOIN
%token TABLE
%token TEMPORARY
%token TEXT
%token TERMINATED
%token THEN
%token TIME
%token TIMESTAMP
%token TINYBLOB
%token TINYINT
%token TINYTEXT
%token TO
%token TRAILING
%token TRIGGER
%token UNDO
%token UNION
%token UNIQUE
%token UNLOCK
%token UNSIGNED
%token UPDATE
%token USAGE
%token USE
%token USING
%token UTC_DATE
%token UTC_TIME
%token UTC_TIMESTAMP
%token VALUES
%token VARBINARY
%token VARCHAR
%token VARYING
%token WHEN
%token WHERE
%token WHILE
%token WRITE
%token XOR
%token YEAR
%token YEAR_MONTH
%token ZEROFILL

 /* functions with special syntax */
%token FSUBSTRING
%token FTRIM
%token FDATE_ADD FDATE_SUB
%token FCOUNT

%type <intval> select_opts select_expr_list
%type <intval> val_list opt_val_list case_list
%type <intval> groupby_list opt_asc_desc
%type <intval> table_references opt_inner_cross opt_outer
%type <intval> left_or_right opt_left_or_right_outer column_list
%type <intval> index_list opt_for_join

%type <intval> delete_opts delete_list
%type <intval> insert_opts insert_vals insert_vals_list
%type <intval> insert_asgn_list opt_if_not_exists update_opts update_asgn_list
%type <intval> opt_temporary opt_length opt_binary opt_uz enum_list
%type <intval> column_atts data_type opt_ignore_replace create_col_list

%start stmt_list

%%

stmt_list: stmt ';'
  | stmt_list stmt ';'
  ;

   /* statements: select statement */

stmt: select_stmt { emit(result, "STMT"); }
   ;

select_stmt: SELECT select_opts select_expr_list
                        { emit(result, "SELECTNODATA %d %d", $2, $3); } ;
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
| select_opts DISTINCTROW         { if($$ & 04) yyerror(result, "duplicate DISTINCTROW option"); $$ = $1 | 04; }
| select_opts HIGH_PRIORITY       { if($$ & 010) yyerror(result, "duplicate HIGH_PRIORITY option"); $$ = $1 | 010; }
| select_opts STRAIGHT_JOIN       { if($$ & 020) yyerror(result, "duplicate STRAIGHT_JOIN option"); $$ = $1 | 020; }
| select_opts SQL_SMALL_RESULT    { if($$ & 040) yyerror(result, "duplicate SQL_SMALL_RESULT option"); $$ = $1 | 040; }
| select_opts SQL_BIG_RESULT      { if($$ & 0100) yyerror(result, "duplicate SQL_BIG_RESULT option"); $$ = $1 | 0100; }
| select_opts SQL_CALC_FOUND_ROWS { if($$ & 0200) yyerror(result, "duplicate SQL_CALC_FOUND_ROWS option"); $$ = $1 | 0200; }
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
    NAME opt_as_alias index_hint { emit(result, "TABLE %s", $1); free($1); }
  | NAME '.' NAME opt_as_alias index_hint { emit(result, "TABLE %s.%s", $1, $3);
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
  | table_reference STRAIGHT_JOIN table_factor
                  { emit(result, "JOIN %d", 0200); }
  | table_reference STRAIGHT_JOIN table_factor ON expr
                  { emit(result, "JOIN %d", 0200); }
  | table_reference left_or_right opt_outer JOIN table_factor join_condition
                  { emit(result, "JOIN %d", 0300+$2+$3); }
  | table_reference NATURAL opt_left_or_right_outer JOIN table_factor
                  { emit(result, "JOIN %d", 0400+$3); }
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

opt_left_or_right_outer: LEFT opt_outer { $$ = 1 + $2; }
   | RIGHT opt_outer  { $$ = 2 + $2; }
   | /* nil */ { $$ = 0; }
   ;

opt_join_condition: join_condition | /* nil */ ;

join_condition:
    ON expr { emit(result, "ONEXPR"); }
    | USING '(' column_list ')' { emit(result, "USING %d", $3); }
    ;

index_hint:
     USE KEY opt_for_join '(' index_list ')'
                  { emit(result, "INDEXHINT %d %d", $5, 010+$3); }
   | IGNORE KEY opt_for_join '(' index_list ')'
                  { emit(result, "INDEXHINT %d %d", $5, 020+$3); }
   | FORCE KEY opt_for_join '(' index_list ')'
                  { emit(result, "INDEXHINT %d %d", $5, 030+$3); }
   | /* nil */
   ;

opt_for_join: FOR JOIN { $$ = 1; }
   | /* nil */ { $$ = 0; }
   ;

index_list: NAME  { emit(result, "INDEX %s", $1); free($1); $$ = 1; }
   | index_list ',' NAME { emit(result, "INDEX %s", $3); free($3); $$ = $1 + 1; }
   ;

table_subquery: '(' select_stmt ')' { emit(result, "SUBQUERY"); }
   ;

   /* statements: delete statement */

stmt: delete_stmt { emit(result, "STMT"); }
   ;

delete_stmt: DELETE delete_opts FROM NAME
    opt_where opt_orderby opt_limit
                  { emit(result, "DELETEONE %d %s", $2, $4); free($4); }
;

delete_opts: delete_opts LOW_PRIORITY { $$ = $1 + 01; }
   | delete_opts QUICK { $$ = $1 + 02; }
   | delete_opts IGNORE { $$ = $1 + 04; }
   | /* nil */ { $$ = 0; }
   ;

delete_stmt: DELETE delete_opts
    delete_list
    FROM table_references opt_where
            { emit(result, "DELETEMULTI %d %d %d", $2, $3, $5); }

delete_list: NAME opt_dot_star { emit(result, "TABLE %s", $1); free($1); $$ = 1; }
   | delete_list ',' NAME opt_dot_star
            { emit(result, "TABLE %s", $3); free($3); $$ = $1 + 1; }
   ;

opt_dot_star: /* nil */ | '.' '*' ;

delete_stmt: DELETE delete_opts
    FROM delete_list
    USING table_references opt_where
            { emit(result, "DELETEMULTI %d %d %d", $2, $4, $6); }
;

   /* statements: insert statement */

stmt: insert_stmt { emit(result, "STMT"); }
   ;

insert_stmt: INSERT insert_opts opt_into NAME
     opt_col_names
     VALUES insert_vals_list
     opt_ondupupdate { emit(result, "INSERTVALS %d %d %s", $2, $7, $4); free($4); }
   ;

opt_ondupupdate: /* nil */
   | ONDUPLICATE KEY UPDATE insert_asgn_list { emit(result, "DUPUPDATE %d", $4); }
   ;

insert_opts: /* nil */ { $$ = 0; }
   | insert_opts LOW_PRIORITY { $$ = $1 | 01 ; }
   | insert_opts DELAYED { $$ = $1 | 02 ; }
   | insert_opts HIGH_PRIORITY { $$ = $1 | 04 ; }
   | insert_opts IGNORE { $$ = $1 | 010 ; }
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

insert_stmt: INSERT insert_opts opt_into NAME
    SET insert_asgn_list
    opt_ondupupdate
     { emit(result, "INSERTASGN %d %d %s", $2, $6, $4); free($4); }
   ;

insert_stmt: INSERT insert_opts opt_into NAME opt_col_names
    select_stmt
    opt_ondupupdate { emit(result, "INSERTSELECT %d %s", $2, $4); free($4); }
  ;

insert_asgn_list:
     NAME COMPARISON expr 
     { if ($2 != 4) yyerror(result, "bad insert assignment to %s", $1);
       emit(result, "ASSIGN %s", $1); free($1); $$ = 1; }
   | NAME COMPARISON DEFAULT
               { if ($2 != 4) yyerror(result, "bad insert assignment to %s", $1);
                 emit(result, "DEFAULT"); emit(result, "ASSIGN %s", $1); free($1); $$ = 1; }
   | insert_asgn_list ',' NAME COMPARISON expr
               { if ($4 != 4) yyerror(result, "bad insert assignment to %s", $1);
                 emit(result, "ASSIGN %s", $3); free($3); $$ = $1 + 1; }
   | insert_asgn_list ',' NAME COMPARISON DEFAULT
               { if ($4 != 4) yyerror(result, "bad insert assignment to %s", $1);
                 emit(result, "DEFAULT"); emit(result, "ASSIGN %s", $3); free($3); $$ = $1 + 1; }
   ;

   /** replace just like insert **/
stmt: replace_stmt { emit(result, "STMT"); }
   ;

replace_stmt: REPLACE insert_opts opt_into NAME
     opt_col_names
     VALUES insert_vals_list
     opt_ondupupdate { emit(result, "REPLACEVALS %d %d %s", $2, $7, $4); free($4); }
   ;

replace_stmt: REPLACE insert_opts opt_into NAME
    SET insert_asgn_list
    opt_ondupupdate
     { emit(result, "REPLACEASGN %d %d %s", $2, $6, $4); free($4); }
   ;

replace_stmt: REPLACE insert_opts opt_into NAME opt_col_names
    select_stmt
    opt_ondupupdate { emit(result, "REPLACESELECT %d %s", $2, $4); free($4); }
  ;

/** update **/
stmt: update_stmt { emit(result, "STMT"); }
   ;

update_stmt: UPDATE update_opts table_references
    SET update_asgn_list
    opt_where
    opt_orderby
opt_limit { emit(result, "UPDATE %d %d %d", $2, $3, $5); }
;

update_opts: /* nil */ { $$ = 0; }
   | insert_opts LOW_PRIORITY { $$ = $1 | 01 ; }
   | insert_opts IGNORE { $$ = $1 | 010 ; }
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


   /** create database **/

stmt: create_database_stmt { emit(result, "STMT"); }
   ;

create_database_stmt: 
     CREATE DATABASE opt_if_not_exists NAME { emit(result, "CREATEDATABASE %d %s", $3, $4); free($4); }
   | CREATE SCHEMA opt_if_not_exists NAME { emit(result, "CREATEDATABASE %d %s", $3, $4); free($4); }
   ;

opt_if_not_exists:  /* nil */ { $$ = 0; }
   | IF EXISTS           { if(!$2) yyerror(result, "IF EXISTS doesn't exist");
                        $$ = $2; /* NOT EXISTS hack */ }
   ;


   /** create table **/
stmt: create_table_stmt { emit(result, "STMT"); }
   ;

create_table_stmt: CREATE opt_temporary TABLE opt_if_not_exists NAME
   '(' create_col_list ')' { emit(result, "CREATE %d %d %d %s", $2, $4, $7, $5); free($5); }
   ;

create_table_stmt: CREATE opt_temporary TABLE opt_if_not_exists NAME '.' NAME
   '(' create_col_list ')' { emit(result, "CREATE %d %d %d %s.%s", $2, $4, $9, $5, $7);
                          free($5); free($7); }
   ;

create_table_stmt: CREATE opt_temporary TABLE opt_if_not_exists NAME
   '(' create_col_list ')'
create_select_statement { emit(result, "CREATESELECT %d %d %d %s", $2, $4, $7, $5); free($5); }
    ;

create_table_stmt: CREATE opt_temporary TABLE opt_if_not_exists NAME
   create_select_statement { emit(result, "CREATESELECT %d %d 0 %s", $2, $4, $5); free($5); }
    ;

create_table_stmt: CREATE opt_temporary TABLE opt_if_not_exists NAME '.' NAME
   '(' create_col_list ')'
   create_select_statement  { emit(result, "CREATESELECT %d %d 0 %s.%s", $2, $4, $5, $7);
                              free($5); free($7); }
    ;

create_table_stmt: CREATE opt_temporary TABLE opt_if_not_exists NAME '.' NAME
   create_select_statement { emit(result, "CREATESELECT %d %d 0 %s.%s", $2, $4, $5, $7);
                          free($5); free($7); }
    ;

create_col_list: create_definition { $$ = 1; }
    | create_col_list ',' create_definition { $$ = $1 + 1; }
    ;

create_definition: { emit(result, "STARTCOL"); } NAME data_type column_atts
                   { emit(result, "COLUMNDEF %d %s", $3, $2); free($2); }

    | PRIMARY KEY '(' column_list ')'    { emit(result, "PRIKEY %d", $4); }
    | KEY '(' column_list ')'            { emit(result, "KEY %d", $3); }
    | INDEX '(' column_list ')'          { emit(result, "KEY %d", $3); }
    | FULLTEXT INDEX '(' column_list ')' { emit(result, "TEXTINDEX %d", $4); }
    | FULLTEXT KEY '(' column_list ')'   { emit(result, "TEXTINDEX %d", $4); }
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
    | column_atts COMMENT STRING { emit(result, "ATTR COMMENT %s", $3); free($3); $$ = $1 + 1; }
    ;

opt_length: /* nil */ { $$ = 0; }
   | '(' INTNUM ')' { $$ = $2; }
   | '(' INTNUM ',' INTNUM ')' { $$ = $2 + 1000*$4; }
   ;

opt_binary: /* nil */ { $$ = 0; }
   | BINARY { $$ = 4000; }
   ;

opt_uz: /* nil */ { $$ = 0; }
   | opt_uz UNSIGNED { $$ = $1 | 1000; }
   | opt_uz ZEROFILL { $$ = $1 | 2000; }
   ;

opt_csc: /* nil */
   | opt_csc CHAR SET STRING { emit(result, "COLCHARSET %s", $4); free($4); }
   | opt_csc COLLATE STRING { emit(result, "COLCOLLATE %s", $3); free($3); }
   ;

data_type:
     BIT opt_length { $$ = 10000 + $2; }
   | TINYINT opt_length opt_uz { $$ = 10000 + $2; }
   | SMALLINT opt_length opt_uz { $$ = 20000 + $2 + $3; }
   | MEDIUMINT opt_length opt_uz { $$ = 30000 + $2 + $3; }
   | INT opt_length opt_uz { $$ = 40000 + $2 + $3; }
   | INTEGER opt_length opt_uz { $$ = 50000 + $2 + $3; }
   | BIGINT opt_length opt_uz { $$ = 60000 + $2 + $3; }
   | REAL opt_length opt_uz { $$ = 70000 + $2 + $3; }
   | DOUBLE opt_length opt_uz { $$ = 80000 + $2 + $3; }
   | FLOAT opt_length opt_uz { $$ = 90000 + $2 + $3; }
   | DECIMAL opt_length opt_uz { $$ = 110000 + $2 + $3; }
   | DATE { $$ = 100001; }
   | TIME { $$ = 100002; }
   | TIMESTAMP { $$ = 100003; }
   | DATETIME { $$ = 100004; }
   | YEAR { $$ = 100005; }
   | CHAR opt_length opt_csc { $$ = 120000 + $2; }
   | VARCHAR '(' INTNUM ')' opt_csc { $$ = 130000 + $3; }
   | BINARY opt_length { $$ = 140000 + $2; }
   | VARBINARY '(' INTNUM ')' { $$ = 150000 + $3; }
   | TINYBLOB { $$ = 160001; }
   | BLOB { $$ = 160002; }
   | MEDIUMBLOB { $$ = 160003; }
   | LONGBLOB { $$ = 160004; }
   | TINYTEXT opt_binary opt_csc { $$ = 170000 + $2; }
   | TEXT opt_binary opt_csc { $$ = 171000 + $2; }
   | MEDIUMTEXT opt_binary opt_csc { $$ = 172000 + $2; }
   | LONGTEXT opt_binary opt_csc { $$ = 173000 + $2; }
   | ENUM '(' enum_list ')' opt_csc { $$ = 200000 + $3; }
   | SET '(' enum_list ')' opt_csc { $$ = 210000 + $3; }
   ;

enum_list: STRING { emit(result, "ENUMVAL %s", $1); free($1); $$ = 1; }
   | enum_list ',' STRING { emit(result, "ENUMVAL %s", $3); free($3); $$ = $1 + 1; }
   ;

create_select_statement: opt_ignore_replace opt_as select_stmt { emit(result, "CREATESELECT %d", $1); }
   ;

opt_ignore_replace: /* nil */ { $$ = 0; }
   | IGNORE { $$ = 1; }
   | REPLACE { $$ = 2; }
   ;

opt_temporary:   /* nil */ { $$ = 0; }
   | TEMPORARY { $$ = 1;}
   ;

   /**** set user variables ****/

stmt: set_stmt { emit(result, "STMT"); }
   ;

set_stmt: SET set_list ;

set_list: set_expr | set_list ',' set_expr ;

set_expr:
      USERVAR COMPARISON expr { if ($2 != 4) yyerror(result, "bad set to @%s", $1);
		 emit(result, "SET %s", $1); free($1); }
    | USERVAR ASSIGN expr { emit(result, "SET %s", $1); free($1); }
    ;

   /**** expressions ****/

expr: NAME          { emit(result, "NAME %s", $1); free($1); }
   | USERVAR         { emit(result, "USERVAR %s", $1); free($1); }
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
   | expr COMPARISON ANY '(' select_stmt ')' { emit(result, "CMPANYSELECT %d", $2); }
   | expr COMPARISON SOME '(' select_stmt ')' { emit(result, "CMPANYSELECT %d", $2); }
   | expr COMPARISON ALL '(' select_stmt ')' { emit(result, "CMPALLSELECT %d", $2); }
   | expr '|' expr { emit(result, "BITOR"); }
   | expr '&' expr { emit(result, "BITAND"); }
   | expr '^' expr { emit(result, "BITXOR"); }
   | expr SHIFT expr { emit(result, "SHIFT %s", $2==1?"left":"right"); }
   | NOT expr { emit(result, "NOT"); }
   | '!' expr { emit(result, "NOT"); }
   | USERVAR ASSIGN expr { emit(result, "ASSIGN @%s", $1); free($1); }
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

expr: FSUBSTRING '(' val_list ')' {  emit(result, "CALL %d SUBSTR", $3);}
   | FSUBSTRING '(' expr FROM expr ')' {  emit(result, "CALL 2 SUBSTR"); }
   | FSUBSTRING '(' expr FROM expr FOR expr ')' {  emit(result, "CALL 3 SUBSTR"); }
| FTRIM '(' val_list ')' { emit(result, "CALL %d TRIM", $3); }
   | FTRIM '(' trim_ltb expr FROM val_list ')' { emit(result, "CALL 3 TRIM"); }
   ;

trim_ltb: LEADING { emit(result, "INT 1"); }
   | TRAILING { emit(result, "INT 2"); }
   | BOTH { emit(result, "INT 3"); }
   ;

expr: FDATE_ADD '(' expr ',' interval_exp ')' { emit(result, "CALL 3 DATE_ADD"); }
   |  FDATE_SUB '(' expr ',' interval_exp ')' { emit(result, "CALL 3 DATE_SUB"); }
   ;

interval_exp: INTERVAL expr DAY_HOUR { emit(result, "NUMBER 1"); }
   | INTERVAL expr DAY_MICROSECOND { emit(result, "NUMBER 2"); }
   | INTERVAL expr DAY_MINUTE { emit(result, "NUMBER 3"); }
   | INTERVAL expr DAY_SECOND { emit(result, "NUMBER 4"); }
   | INTERVAL expr YEAR_MONTH { emit(result, "NUMBER 5"); }
   | INTERVAL expr YEAR       { emit(result, "NUMBER 6"); }
   | INTERVAL expr HOUR_MICROSECOND { emit(result, "NUMBER 7"); }
   | INTERVAL expr HOUR_MINUTE { emit(result, "NUMBER 8"); }
   | INTERVAL expr HOUR_SECOND { emit(result, "NUMBER 9"); }
   ;

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

expr: BINARY expr %prec UMINUS { emit(result, "STRTOBIN"); }
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

int bison_parse_string(const char* in, struct vector *out) {
  flex_scan_string(in);
  int rv = yyparse(out);
  flex_delete_buffer();
  return rv;
}


