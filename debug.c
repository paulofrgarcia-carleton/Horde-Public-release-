#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include"Horde.h"


extern unsigned int errors;

extern struct scope_node* scope_ll;


void dbg_print_op_ll(struct operator_node *l)
{
	printf("ll ");
	while(l != (struct operator_node*)0)
	{
		printf("%lu ",(long unsigned int)l);
		l = l->next;
	}
	printf("\n");
}

char *op_names[] = {"INPUT","OUTPUT","PLUS","TIMES","ISEQUAL","ISLESS","ISGREATER","IF","ELSE","MINUS","EXPANSION","MERGE"};
void dbg_print_ready_pool(struct ready_node *l)
{
	printf("ready pool ");
	while(l != (struct ready_node*)0)
	{
		printf("%d-%s ",l->op->o.op,op_names[l->op->o.op]);
		l = l->next;
	}
	printf("\n");
}

void print_all()
{
	struct scope_node* s = scope_ll;
	while(s != (struct scope_node*)0)
	{
		printf("Scope \"%s\":\n",s->s.name);
		struct datum_node* l = s->s.datum_ll;
		while(l != (struct datum_node*)0)
		{
			if((l->data.constructed==1))
				printf("\tDatum \"%s\" value %d\n",l->data.name, l->data.value);
			else
				printf("\tDatum \"%s\" (empty)\n",l->data.name);
			l = l->next;
		}	
		printf("\n");
		print_operators(s->s.operator_ll);
		s = s->next;
	}
}
void print_one(struct scope_node* s)
{
		if(s == (struct scope_node *)0)
		{
			printf("Null scope\n");
			return;
		}
		if(s->s.operator_ll == (struct operator_node *)0)
		{
			printf("Scope is empty...\n");
			return;
		}
		printf("Scope \"%s\":\n",s->s.name);
		struct datum_node* l = s->s.datum_ll;
		while(l != (struct datum_node*)0)
		{
			if((l->data.constructed)!=-1)
			{
				if((l->data.constructed)==1)
					printf("\tDatum \"%s\" value %d\n",l->data.name, l->data.value);
				else
					printf("\tDatum \"%s\" (empty)\n",l->data.name);
			}
			l = l->next;
		}	
		printf("\n");
		print_operators(s->s.operator_ll);
}

//prints, for each datum in a scope, the operators that use it as a source
void print_data_uses(struct scope *s)
{
		struct datum_node* l = s->datum_ll;
		printf("DATUM ARGS:\n");
		while(l != (struct datum_node*)0)
		{
			printf("DATUM %s:	",l->data.name);
			struct op_datum_src *ods_tmp = l->data.ops;
			while(ods_tmp != (struct op_datum_src *)0)
			{
				printf("%lu	",(long unsigned int)(ods_tmp->op));
				ods_tmp = ods_tmp->next;
			}
			l = l->next;
		}	
}

void print_operators(struct operator_node *o)
{
	int tmp=0;
	while(o != (struct operator_node *)0)
	{
		tmp++;
		printf("%d\t",tmp);
		printf("%d ",o->o.pending_operators);
		switch(o->o.op)
		{
			case op_INPUT: 
			{
				printf("input (%s)\n",o->o.dest->name);
			} break;
			case op_OUTPUT:
			{
				printf("output (%s)\n",o->o.src_1->name);
			} break;
			case op_PLUS:
			{
				printf("(%s) = (%s) + (%s)\n",o->o.dest->name,o->o.src_1->name,o->o.src_2->name);
			} break;
			case op_TIMES:
			{
				printf("(%s) = (%s) * (%s)\n",o->o.dest->name,o->o.src_1->name,o->o.src_2->name);
			} break;
			case op_MERGE:
			{
				printf("(%s) = (%s) merge (%s)\n",o->o.dest->name,o->o.src_1->name,o->o.src_2->name);
			} break;
			case op_MINUS:
			{
				printf("(%s) = (%s) - (%s)\n",o->o.dest->name,o->o.src_1->name,o->o.src_2->name);
			} break;
			case op_ISEQUAL:
			{
				printf("(%s) = (%s) == (%s)?\n",o->o.dest->name,o->o.src_1->name,o->o.src_2->name);
			} break;
			case op_IF:
			{
				printf("(%s) = IF (%s) then (%s)\n",o->o.dest->name,o->o.src_1->name,o->o.src_2->name);
			} break;
			case op_ELSE:
			{
				printf("(%s) = ELSE (%s) then (%s)\n",o->o.dest->name,o->o.src_1->name,o->o.src_2->name);
			} break;
			case op_EXPANSION:
			{
				printf("expansion (%s) ",o->o.scope);
				print_mappings(o->o.mappings);
			} break;
			case op_ISLESS:
			{
				printf("(%s) = (%s) < (%s)?\n", o->o.dest->name,o->o.src_1->name,o->o.src_2->name);
			} break;
			case op_ISGREATER:
			{
				printf("(%s) = (%s) > (%s)?\n", o->o.dest->name,o->o.src_1->name,o->o.src_2->name);
			} break;
			default:
			{
				printf("Unknown operator\n");
			} break;
		}
		o=o->next;
	}
	printf("\n%d operators\n",tmp);
}

void print_mappings(struct mapping *m)
{
	while(m != (struct mapping *)0)
	{
		printf("%s <- %s;",m->subnode,m->argument);
		m = m->next;
	}
	printf("\n");
}

void dbg_print_operators(struct operator_node *s)
{
	while(s != (struct operator_node *)0)
	{
		printf("node %ld (%ld) previous %ld next %ld\n",(unsigned long int)s,(unsigned long int)get_copy_dictionary_entry(s),(unsigned long int)s->previous,(unsigned long int)s->next);
		printf("dest %ld src 1 %ld src2 %ld\n\n",(unsigned long int)s->o.dest,(unsigned long int)s->o.src_1,(unsigned long int)s->o.src_2);
		s = s->next;
	}
}
