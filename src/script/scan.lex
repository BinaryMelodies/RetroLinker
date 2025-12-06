
%{
#include "parse.tab.hh"

#define YY_USER_ACTION \
	yylloc.first_line = yylloc.last_line; \
	yylloc.first_column = yylloc.last_column; \
	yylloc.last_line = yylineno; \
	if(strchr(yytext, '\n')) \
		yylloc.last_column = strlen(yytext) - (strrchr(yytext, '\n') - yytext); \
	else \
		yylloc.last_column += strlen(yytext);
%}

%option noyywrap
%option yylineno

%%

[ \n\t]+	;
[#%][^\n]*	;
"//"[^\n]*	;

"align"	return ALIGN;
"all"	return ALL;
"and"	return AND;
"any"	return ANY;
"at"	return AT;
"base"	return BASE;
"call"	return CALL;
"customflag"	return CUSTOMFLAG; /* section type */
"executable"|"execute"|"exec"	return EXECUTE; /* section type */
"fixed"	return FIXED; /* section type */
"for"	return FOR;
"heap"	return HEAP;
"here"	return HERE;
"maximum"|"max"	return MAXIMUM;
"mergeable"|"merge"	return MERGE; /* section type */
"minimum"|"min"	return MINIMUM;
"not"	return NOT;
"of"	return OF;
"optional"|"option"|"opt"	return OPTIONAL; /* section type */
"or"	return OR;
"readable"|"read"	return READ; /* section type */
"resource"|"rsrc"	return RESOURCE; /* section type */
"size"	return SIZE;
"start"	return START;
"stack"	return STACK;
"suffix"	return SUFFIX;
"writable"|"write"	return WRITE; /* section type */
"zerofilled"|"zero"	return ZERO;	 /* section type */

"<<"	return LL;
">>"	return RR;

[A-Za-z_.$][A-Za-z_.$0-9]*	yylval.s = strdup(yytext); return IDENTIFIER;
\"[^\"]*\"	yytext[strlen(yytext) - 1] = '\0'; yylval.s = strdup(yytext + 1); return IDENTIFIER;
\?[^\?]+\?	yytext[strlen(yytext) - 1] = '\0'; yylval.s = strdup(yytext + 1); return PARAMETER;

0|[1-9][0-9]*	yylval.i = strtol(yytext, NULL, 10); return INTEGER;
0[Oo][0-7]+	yylval.i = strtol(yytext + 2, NULL, 8); return INTEGER;
0[Xx][0-9A-Fa-f]+	yylval.i = strtol(yytext + 2, NULL, 16); return INTEGER;

.	return yytext[0];

%%

static YY_BUFFER_STATE yybuffer;

static void cleanup(void);
static bool cleanup_set;

void set_buffer(const char * buffer)
{
	if(yybuffer != NULL)
		yy_delete_buffer(yybuffer);
	if(buffer != NULL)
	{
		yyin = stdin;
		atexit(cleanup);
		cleanup_set = true;
		yybuffer = yy_scan_string(buffer);
	}
}

static void cleanup(void)
{
	(void)yyunput;
	set_buffer(NULL);
}

void set_stream(FILE * file)
{
	/* TODO: do we need to switch the yylex buffer? */
	yyin = file;
}

