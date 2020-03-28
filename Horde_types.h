#ifndef HORDE_TYPES_H
#define HORDE_TYPES_H


struct datum;
struct operator;
struct mapping;

enum operation{op_INPUT=0, op_OUTPUT, op_PLUS, op_TIMES, op_ISEQUAL, op_ISLESS, op_ISGREATER, op_IF, op_ELSE, op_MINUS, op_EXPANSION, op_MERGE};//MINUS, EQUAL, ISEQUAL, ISNOTEQUAL, IF_BOOL, IF_ONCE, IF_EVER};

struct operator
{
	enum operation op;
	struct datum *dest;
	struct datum *src_1;
	struct datum *src_2;
	char *scope;
	struct mapping *mappings;
	int pending_operators;
};

//used as a linked list inside datum: points to each operator where datum is source
struct op_datum_src
{
	struct operator *op;
	struct op_datum_src *next;	
};

struct datum
{
	char *name;
	int value;
	int constructed;
	struct op_datum_src *ops;
};

struct operator_node
{
	struct operator o;
	struct operator_node *next;
	struct operator_node *previous;
	//0 default - 1 after it's been added to a ready pool
	int taken;
};

struct datum_node
{
	struct datum data;
	struct datum_node *next;
};

struct ready_node 
{
	struct operator_node* op;
	struct ready_node* prev;
	struct ready_node* next;
};


struct scope
{
	char *name;
	//number of expansions
	unsigned int expansions;
	//linked list of all data in the scope
	struct datum_node* datum_ll;
	//linked list of all operators in the scope
	struct operator_node* operator_ll;
};

struct scope_node
{
	struct scope s;
	struct scope_node* next;
};

enum direction{direction_IN, direction_OUT};

struct mapping
{
	char *subnode;
	char *argument;
	enum direction dir;
	struct mapping *next;
};


#endif