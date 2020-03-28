#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include"Horde.h"


extern unsigned int errors;



struct scope_node* scope_ll = (struct scope_node*) 0;

void set_scope(char *scope)
{
	struct scope_node *s = (struct scope_node*)malloc(sizeof(struct scope_node));
	s->next = scope_ll;
	scope_ll = s;

	scope_ll->s.datum_ll = (struct datum_node *)0;
	scope_ll->s.operator_ll = (struct operator_node *)0;

	scope_ll->s.name = (char *)malloc(strlen(scope)+1);
	strcpy(scope_ll->s.name, scope);

	scope_ll->s.expansions = 0;
}

/*
	creates a data structure of argument (input/output) mappings that is later copied
	when "add_new_subgraph" is invoked to create the subgraph operator
*/
struct mapping *args_mapping = (struct mapping *)0;

void add_mapping(char *x, char *y, enum direction d)
{
	struct mapping *tmp = (struct mapping *)malloc(sizeof(struct mapping));

	tmp->next = args_mapping;
	args_mapping = tmp;

	args_mapping->subnode = (char *)malloc(strlen(x)+1);
	strcpy(args_mapping->subnode,x);

	args_mapping->argument = (char *)malloc(strlen(y)+1);
	strcpy(args_mapping->argument,y);

	args_mapping->dir = d;
}

/*
	Counts how many inputs are mapped in subgraph expansion
	To determine what readiness number must be at startup

	called by "add_new_subgraph"
*/
int count_mapped_inputs(struct scope *s, struct mapping *m)
{


	struct operator_node *root_op = s->operator_ll;

	int result = 0;

	//Finds all INPUT operators
	while(s->operator_ll != (struct operator_node *)0)
	{
		if(s->operator_ll->o.op == op_INPUT)
		{
			result++;
		}
		s->operator_ll = s->operator_ll->next;
	}
	s->operator_ll = root_op;
	return result;
}



/*
This is technically adding a new operator, but let's separate the function since 
subgraphing has a few key details...

At the end of this function, "args_mapping" should be NULL so it can be used by a subsequent 
subgraph expansion

*/
void add_new_subgraph(char *scope)
{
	struct operator_node* new_node = (struct operator_node*)malloc(sizeof(struct operator_node));
	new_node->next = scope_ll->s.operator_ll;
	if(scope_ll->s.operator_ll != (struct operator_node*)0)
		scope_ll->s.operator_ll->previous = new_node;
	scope_ll->s.operator_ll = new_node;
	new_node->previous=(struct operator_node*)0;
	
	new_node->o.op = op_EXPANSION;
	//not using src_1, src_2 nor dest
	new_node->o.dest = (struct datum *)0;
	new_node->o.dest = (struct datum *)0;
	new_node->o.dest = (struct datum *)0;

	//We start the readiness of EXPANSION at 0
	//Then increment for each input mapping later
	new_node->o.pending_operators = 0;

	new_node->o.mappings = args_mapping;
	new_node->o.scope = (char *)malloc(strlen(scope)+1);
	strcpy(new_node->o.scope, scope);


	new_node->o.pending_operators = count_mapped_inputs(&(get_scope(scope)->s), args_mapping);

	new_node->taken = 0;	

	//make all input data that are mapped in mappings point here as a destination as well
	while(args_mapping != (struct mapping *)0)
	{
		if(args_mapping->dir == direction_IN)
		{

			struct op_datum_src *tmp = (struct op_datum_src *)malloc(sizeof(struct op_datum_src));
			tmp->op = &(new_node->o);
			
			tmp->next = get_datum_in_current_scope(args_mapping->argument)->ops;
			
			get_datum_in_current_scope(args_mapping->argument)->ops = tmp;
		}
		args_mapping = args_mapping->next;
	}

	args_mapping = (struct mapping *)0;
}



void add_new_datum(char* x, int y, int c)
{
	struct datum_node* new_node = (struct datum_node*)malloc(sizeof(struct datum_node));
	new_node->next = scope_ll->s.datum_ll;
	scope_ll->s.datum_ll = new_node;

	new_node->data.name = x;
	new_node->data.value = y;
	new_node->data.constructed = c;
	new_node->data.ops = (struct op_datum_src *)0; //default

}



void add_new_operator(char *z, enum operation op, char *x, char *y)
{
	struct operator_node* new_node = (struct operator_node*)malloc(sizeof(struct operator_node));
	
	new_node->next = scope_ll->s.operator_ll;
	if(scope_ll->s.operator_ll != (struct operator_node*)0)
	{	
		scope_ll->s.operator_ll->previous = new_node;
	}
	scope_ll->s.operator_ll = new_node;
	new_node->previous=(struct operator_node*)0;

	new_node->o.op = op;
	new_node->o.dest = (struct datum *)0; //default

	new_node->o.mappings = (struct mapping *)0;
	new_node->o.scope = (char *)0;

	new_node->taken = 0;

	//link inputs x, y, output z, to operator
	if(x != (char *)0)
	{
		struct op_datum_src *tmp = (struct op_datum_src *)malloc(sizeof(struct op_datum_src));
		tmp->op = &(new_node->o);
		tmp->next = get_datum_in_current_scope(x)->ops;
		get_datum_in_current_scope(x)->ops = tmp;
		new_node->o.src_1 = get_datum_in_current_scope(x);
	}
	else
		new_node->o.src_1 = (struct datum *)0;	

	if(y != (char *)0)
	{
		struct op_datum_src *tmp = (struct op_datum_src *)malloc(sizeof(struct op_datum_src));
		tmp->op = &(new_node->o);
		tmp->next = get_datum_in_current_scope(y)->ops;
		get_datum_in_current_scope(y)->ops = tmp;
		new_node->o.src_2 = get_datum_in_current_scope(y);
	}
	else
		new_node->o.src_2 = (struct datum *)0;

	if(z != (char *)0)
	{
		new_node->o.dest = get_datum_in_current_scope(z);
	}
	else
		new_node->o.dest = (struct datum *)0;


	switch (op)
	{
		case op_INPUT:
			new_node->o.pending_operators = 0;
			break;
		case op_MERGE:
			new_node->o.pending_operators = 1;
			break;
		case op_OUTPUT:
			new_node->o.pending_operators = 1;
			if(new_node->o.src_1 != (struct datum *)0)
			{
				if(new_node->o.src_1->constructed)
					new_node->o.pending_operators --;
			}
			break;
		case op_EXPANSION:
			new_node->o.pending_operators = 0; 
			break;					
		default:
			new_node->o.pending_operators = 2;
			if(new_node->o.src_1 != (struct datum *)0)
			{
				if(new_node->o.src_1->constructed)
					new_node->o.pending_operators --;
			}
			if(new_node->o.src_2 != (struct datum *)0)
			{
				if(new_node->o.src_2->constructed)
					new_node->o.pending_operators --;
			}
			break;
	}
	
}

