/*
 *  The scanner definition for COOL.
 */

/*
 *  Stuff enclosed in %{ %} in the first section is copied verbatim to the
 *  output, so headers and global definitions are placed here to be visible
 * to the code in the file.  Don't remove anything that was here initially
 */
%{
#include <cool-parse.h>
#include <stringtab.h>
#include <utilities.h>

/* The compiler assumes these identifiers. */
#define yylval cool_yylval
#define yylex  cool_yylex

/* Max size of string constants */
#define MAX_STR_CONST 1025
#define YY_NO_UNPUT   /* keep g++ happy */

extern FILE *fin; /* we read from this file */

/* define YY_INPUT so we read from the FILE fin:
 * This change makes it possible to use this scanner in
 * the Cool compiler.
 */
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( (result = fread( (char*)buf, sizeof(char), max_size, fin)) < 0) \
		YY_FATAL_ERROR( "read() in flex scanner failed");

char string_buf[MAX_STR_CONST]; /* to assemble string constants */
char *string_buf_ptr;

extern int curr_lineno;
extern int verbose_flag;

extern YYSTYPE cool_yylval;

/*
 *  Add Your own definitions here
 */
int nested_comment=0;
int str_end=0;
void comment();
int scan_str();

%}

/*
 * Define names for regular expressions here.
 */

DARROW          =>
delim [ \t\n\f\r\v]
Digit [0-9]
Letter [a-zA-Z_]
Char [a-fA-F0-9_]
ws {delim}+
id {letter}({letter}|{digit})*

%x comment
%x line_comment
%x str_const
%x null_in_str

%%

\n { curr_lineno++;}

 /*
  *  Nested comments
  */
<INITIAL>\-\- { BEGIN(line_comment); }
<line_comment>\n { BEGIN(INITIAL); curr_lineno++; }
<line_comment>. {}

"(*" { BEGIN(comment); nested_comment=1; }
<comment><<EOF>> {
BEGIN(INITIAL);
cool_yylval.error_msg="EOF in comment";
return (ERROR);
}

<comment>. {}
<comment>\n { curr_lineno++; }
<comment>"(*" { nested_comment++; }
<comment>"*)" { --nested_comment;
         if(nested_comment==0)
 	 {
		BEGIN(INITIAL);
	 }
}
"*)" { cool_yylval.error_msg="Unmatched *)";
       return (ERROR);
       }


 /*
  *  The multiple-character operators.
  */
{DARROW}		{ return (DARROW); }
"<-"  { return (ASSIGN); }
"<="  { return (LE);}
{ws} {}

 /*
  * Keywords are case-insensitive except for the values true and false,
  * which must begin with a lower-case letter.
  */

[Cc][Ll][Aa][Ss][Ss]  { return (CLASS); }
[Ee][Ll][Ss][Ee]      { return (ELSE); }
[Ff][Ii]	      { return (FI); }
[Ii][Ff]	      { return (IF); }
[Ii][Nn]	      { return (IN); }
[Ii][Nn][Hh][Ee][Rr][Ii][Tt][Ss] { return (INHERITS); }
[Ll][Ee][Tt]	      { return (LET); }
[Ll][Oo][Oo][Pp]      { return (LOOP); }
[Pp][Oo][Oo][Ll]      { return (POOL); }
[Tt][Hh][Ee][Nn]      { return (THEN); }
[Ww][Hh][Ii][Ll][Ee]  { return (WHILE);}
[Cc][Aa][Ss][Ee]      { return (CASE); }
[Ee][Ss][Aa][Cc]      { return (ESAC); }
[Oo][Ff]	      { return (OF); }
[Nn][Ee][Ww]	      { return (NEW); }
[Ii][Ss][Vv][Oo][Ii][Dd] { return (ISVOID); }
[Nn][Oo][Tt]		 { return (NOT); }
f[Aa][Ll][Ss][Ee]	 { cool_yylval.boolean=0;
			 return (BOOL_CONST);}
t[Rr][Uu][Ee]		 { cool_yylval.boolean=1;
			  return (BOOL_CONST);
}

 /*
  *  String constants (C syntax)
  *  Escape sequence \c is accepted for all characters c. Except for 
  *  \n \t \b \f, the result is c.
  *
  */
<INITIAL>\" { BEGIN(str_const);
	    string_buf_ptr=string_buf;}
<str_const><<EOF>> {
BEGIN(INITIAL);
cool_yylval.error_msg="Unterminated string constant";
return (ERROR);
}
<str_const>\" {
*string_buf_ptr='\0';
BEGIN(INITIAL);
cool_yylval.symbol=inttable.add_string(string_buf);
return (STR_CONST);
}
<str_const>\\\0 {
BEGIN(null_in_str);
cool_yylval.error_msg="String contains escaped null character";
return (ERROR);
}
<str_const>\0 {
BEGIN(null_in_str);
cool_yylval.error_msg="String contains null character";
return (ERROR);
}

<str_const>\n {
BEGIN(INITIAL);
curr_lineno++;
cool_yylval.error_msg="Unterminated string constant";
return (ERROR);
}

<str_const>. { *string_buf_ptr = *yytext;
string_buf_ptr++;
if(string_buf_ptr-string_buf >=MAX_STR_CONST){
BEGIN(null_in_str);
cool_yylval.error_msg="String constant too long";
return (ERROR);
}
}
<str_const>\\t { *string_buf_ptr='\t'; string_buf_ptr++;}
<str_const>\\n { *string_buf_ptr='\n'; string_buf_ptr++;
if(string_buf_ptr-string_buf >=MAX_STR_CONST){
BEGIN(null_in_str);
cool_yylval.error_msg="String constant too long";
return (ERROR);
}
}
<str_const>\\\n { *string_buf_ptr='\n'; string_buf_ptr++;
if(string_buf_ptr-string_buf >=MAX_STR_CONST){
BEGIN(null_in_str);
cool_yylval.error_msg="String constant too long";
return (ERROR);
}
}
<str_const>\\b { *string_buf_ptr='\b'; string_buf_ptr++;}
<str_const>\\f { *string_buf_ptr='\f'; string_buf_ptr++;}
<str_const>\\. { *string_buf_ptr=yytext[1]; string_buf_ptr++;
if(string_buf_ptr-string_buf >=MAX_STR_CONST){
BEGIN(null_in_str);
cool_yylval.error_msg="String constant too long";
return (ERROR);
}
}

<null_in_str>\" {BEGIN(INITIAL); }
<null_in_str>\n { curr_lineno++; BEGIN(INITIAL);}
<null_in_str>. {}

[0-9]+ { cool_yylval.symbol=inttable.add_string(yytext); 
       return (INT_CONST);
}
[A-Z][a-zA-Z0-9_]* {
cool_yylval.symbol=inttable.add_string(yytext);
return (TYPEID);
}

[a-zA-Z][a-zA-Z0-9_]* {
cool_yylval.symbol=inttable.add_string(yytext);
return (OBJECTID);
}

\0 { cool_yylval.error_msg="\000";
return (ERROR);
}

[\(\)\/\~\{\}i\<\:\+\-i\*\,\.\=\@\;] { return (yytext[0]);}

. { cool_yylval.error_msg=yytext;
    return (ERROR);
}
%%
