
%code requires
{
#include "script.h"
using namespace Script;
}

%define parse.error verbose
%define parse.lac full
%locations

%{
#include <stdio.h>
#include <string>
#include "script.h"
using namespace Script;

void yyerror(const char * s);
std::unique_ptr<List> script;
%}

%union
{
	char * s;
	long i;
	class Node * n;
	class List * l;
}

%token ALIGN
%token ALL
%token AND
%token ANY
%token AT
%token BASE
%token CALL
%token CUSTOMFLAG
%token EXECUTE
%token FIXED
%token FOR
%token HEAP
%token HERE
%token <s> IDENTIFIER
%token <i> INTEGER
%token MAXIMUM
%token MERGE
%token MINIMUM
%token NOT
%token OF
%token OPTIONAL
%token OR
%token <s> PARAMETER
%token READ
%token RESOURCE
%token SIZE
//%token SKIP
%token STACK
%token START
%token SUFFIX
%token WRITE
%token ZERO

%token LL
%token RR

%right ':'
%left '|'
%left '^'
%left '&'
%left '+' '-'
%left UNARY LL RR

%type <n> expression
%type <l> expression_list
%type <n> predicate negation conjunction conjunction_or_subcommand disjunction simple_disjunction
%type <n> action
%type <l> actions
%type <n> variable_assignment
%type <n> command
%type <l> commands
%type <n> guard
%type <n> segment
%type <n> directive
%type <l> script

%%

program
	: script
		{
			script = std::unique_ptr<List>($1);
		}
	;

script
	:
		{
			$$ = new List;
		}
	| directive
		{
			$$ = new List($1);
		}
	| script ';'
	| script ';' directive
		{
			$$ = $1->Append($3);
		}
	;

directive
	: segment
	| actions
		{
			$$ = new Node(Node::Sequence, $1);
		}
	| variable_assignment
	| CALL IDENTIFIER
		{
			$$ = new Node(Node::Call, new Value<std::string>($2));
		}
	;

segment
	: IDENTIFIER '{' commands '}'
		{
			$$ = new Node(Node::Segment, new Value<std::string>($1), new List(new Node(Node::Sequence, $3), new Node(Node::Sequence)));
		}
	| IDENTIFIER '{' commands '}' actions
		{
			$$ = new Node(Node::Segment, new Value<std::string>($1), new List(new Node(Node::Sequence, $3), new Node(Node::Sequence, $5)));
		}
	| FOR guard '{' commands '}'
		{
			$$ = new Node(Node::SegmentTemplate, new List($2, new Node(Node::Sequence, $4), new Node(Node::Sequence)));
		}
	| FOR guard '{' commands '}' actions
		{
			$$ = new Node(Node::SegmentTemplate, new List($2, new Node(Node::Sequence, $4), new Node(Node::Sequence, $6)));
		}
	;

guard
	: simple_disjunction
	| simple_disjunction MAXIMUM expression
		{
			$$ = new Node(Node::MaximumSections, new List($1, $3));
		}
	;

commands
	:
		{
			$$ = new List;
		}
	| command
		{
			$$ = new List($1);
		}
	| commands ';'
	| commands ';' command
		{
			$$ = $1->Append($3);
		}
	;

command
	: actions
		{
			$$ = new Node(Node::Sequence, $1);
		}
	| ALL
		{
			$$ = new Node(Node::Collect, new List(new Node(Node::MatchAny), new Node(Node::Sequence)));
		}
	| ALL actions
		{
			$$ = new Node(Node::Collect, new List(new Node(Node::MatchAny), new Node(Node::Sequence, $2)));
		}
	| ALL disjunction
		{
			$$ = new Node(Node::Collect, new List($2, new Node(Node::Sequence)));
		}
	| ALL disjunction actions
		{
			$$ = new Node(Node::Collect, new List($2, new Node(Node::Sequence, $3)));
		}
	| variable_assignment
	;

disjunction
	: conjunction_or_subcommand
	| disjunction OR conjunction_or_subcommand
		{
			$$ = new Node(Node::OrPredicate, new List($1, $3));
		}
	;

conjunction_or_subcommand
	: conjunction
	| '(' disjunction actions ')'
		{
			$$ = new Node(Node::Collect, new List($2, new Node(Node::Sequence, $3)));
		}
	;

simple_disjunction
	: conjunction
	| disjunction OR conjunction
		{
			$$ = new Node(Node::OrPredicate, new List($1, $3));
		}
	;

conjunction
	: negation
	| conjunction AND negation
		{
			$$ = new Node(Node::AndPredicate, new List($1, $3));
		}
	;

negation
	: predicate
	| NOT predicate
		{
			$$ = new Node(Node::NotPredicate, new List($2));
		}
	| ANY
		{
			$$ = new Node(Node::MatchAny);
		}
	;

predicate
	: IDENTIFIER
		{
			$$ = new Node(Node::MatchName, new Value<std::string>($1));
			free($1);
		}
	| SUFFIX IDENTIFIER
		{
			$$ = new Node(Node::MatchSuffix, new Value<std::string>($2));
			free($2);
		}
	| READ
		{
			$$ = new Node(Node::IsReadable);
		}
	| WRITE
		{
			$$ = new Node(Node::IsWritable);
		}
	| EXECUTE
		{
			$$ = new Node(Node::IsExecutable);
		}
	| MERGE
		{
			$$ = new Node(Node::IsMergeable);
		}
	| ZERO
		{
			$$ = new Node(Node::IsZeroFilled);
		}
	| FIXED
		{
			$$ = new Node(Node::IsFixedAddress);
		}
	| RESOURCE
		{
			$$ = new Node(Node::IsResource);
		}
	| OPTIONAL
		{
			$$ = new Node(Node::IsOptional);
		}
	| STACK
		{
			$$ = new Node(Node::IsStack);
		}
	| HEAP
		{
			$$ = new Node(Node::IsHeap);
		}
	| CUSTOMFLAG '(' expression ')'
		{
			$$ = new Node(Node::IsCustomFlag, new List($3));
		}
	| '(' simple_disjunction ')'
		{
			$$ = $2;
		}
	;

actions
	: action
		{
			$$ = new List($1);
		}
	| actions action
		{
			$$ = $1->Append($2);
		}
	;

action
	: AT expression
		{
			$$ = new Node(Node::SetCurrentAddress, new List($2));
		}
	| ALIGN expression
		{
			$$ = new Node(Node::AlignAddress, new List($2));
		}
	| BASE expression
		{
			$$ = new Node(Node::SetNextBase, new List($2));
		}
/*	| SKIP expression*/
	;

variable_assignment
	: IDENTIFIER '=' expression
		{
			$$ = new Node(Node::Assign, new Value<std::string>($1), new List($3));
			free($1);
		}
	;

expression
	: INTEGER
		{
			$$ = new Node(Node::Integer, new Value<long>($1));
		}
	| PARAMETER
		{
			$$ = new Node(Node::Parameter, new Value<std::string>($1));
			free($1);
		}
	| IDENTIFIER
		{
			$$ = new Node(Node::Identifier, new Value<std::string>($1));
			free($1);
		}
	| BASE OF IDENTIFIER
		{
			$$ = new Node(Node::BaseOf, new Value<std::string>($3));
			free($3);
		}
	| START OF IDENTIFIER
		{
			$$ = new Node(Node::StartOf, new Value<std::string>($3));
			free($3);
		}
	| SIZE OF IDENTIFIER
		{
			$$ = new Node(Node::SizeOf, new Value<std::string>($3));
			free($3);
		}
	| HERE
		{
			$$ = new Node(Node::CurrentAddress);
		}
	| ALIGN '(' expression ',' expression ')'
		{
			$$ = new Node(Node::AlignTo, new List($3, $5));
		}
	| MAXIMUM '(' expression_list ')'
		{
			$$ = new Node(Node::Maximum, $3);
		}
	| MINIMUM '(' expression_list ')'
		{
			$$ = new Node(Node::Minimum, $3);
		}
	| '(' expression ')'
		{
			$$ = $2;
		}
	| '+' expression %prec UNARY
		{
			$$ = $2;
		}
	| '-' expression %prec UNARY
		{
			$$ = new Node(Node::Neg, new List($2));
		}
	| '~' expression %prec UNARY
		{
			$$ = new Node(Node::Not, new List($2));
		}
	| expression LL expression
		{
			$$ = new Node(Node::Shl, new List($1, $3));
		}
	| expression RR expression
		{
			$$ = new Node(Node::Shr, new List($1, $3));
		}
	| expression '+' expression
		{
			$$ = new Node(Node::Add, new List($1, $3));
		}
	| expression '-' expression
		{
			$$ = new Node(Node::Sub, new List($1, $3));
		}
	| expression '&' expression
		{
			$$ = new Node(Node::And, new List($1, $3));
		}
	| expression '^' expression
		{
			$$ = new Node(Node::Xor, new List($1, $3));
		}
	| expression '|' expression
		{
			$$ = new Node(Node::Or, new List($1, $3));
		}
	| expression ':' expression
		{
			$$ = new Node(Node::Location, new List($1, $3));
		}
	;

expression_list
	: expression
		{
			$$ = new List($1);
		}
	| expression_list ',' expression
		{
			$$ = $1->Append($3);
		}
	;

%%

void yyerror(const char * s)
{
	fprintf(stderr, "Error in line %d at %d: %s\n", yylloc.first_line, yylloc.first_column, s);
}

std::unique_ptr<List> Script::parse_string(const char * buffer)
{
	int result;
	set_buffer(buffer);
	result = yyparse();
	return result == 0 ? std::move(script) : nullptr;
}

std::unique_ptr<List> Script::parse_file(const char * filename, bool& file_error)
{
	int result;
	file_error = false;
	FILE * file = fopen(filename, "r");
	if(file == NULL)
	{
		file_error = true;
		return nullptr;
	}
	set_stream(file);
	set_buffer(NULL);
	result = yyparse();
	fclose(file);
	set_stream(stdin);
	return result == 0 ? std::move(script) : nullptr;
}

