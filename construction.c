#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include"Horde.h"


extern unsigned int errors;

extern struct scope_node* scope_ll;

//returns pointer to datum named "n" if it exists in current scope
//only called by macro expansions for program compilation
struct datum *get_datum_in_current_scope(char *n)
{
	// Traverse linked list searching for node with matching name
	struct datum_node* l = scope_ll->s.datum_ll;
	while(l != (struct datum_node*)0)
	{
		if(strcmp(n, l->data.name)==0)
			return &(l->data);

		l = l->next;
	}
	printf("Error: could not find datum \"%s\" in current scope.\n",n);
	errors++;
	return (struct datum*)0;
}


//returns pointer to datum named "n"
struct datum *get_datum(char *n, struct scope *search_scope)
{
	// Traverse linked list searching for node with matching name
	struct datum_node* l = search_scope->datum_ll;
	while(l != (struct datum_node*)0)
	{
		if(strcmp(n, l->data.name)==0)
			return &(l->data);

		l = l->next;
	}
	printf("Error: could not find datum \"%s\" in current scope.\n",n);
	errors++;
	return (struct datum*)0;
}

struct scope_node *get_scope(char *s)
{
	struct scope_node* tmp = scope_ll; 
	while(strcmp(tmp->s.name,s)!=0)
	{
		tmp = tmp->next;
		if(tmp == (struct scope_node *)0)
			break;
	}
	return tmp;
}


struct copy_dictionary_entry
{
	void *original;
	void *copy;
	struct copy_dictionary_entry *next;	
};
struct copy_dictionary_entry *copy_dictionary = (struct copy_dictionary_entry *)0;

void print_copy_dictionary()
{
	struct copy_dictionary_entry *tmp = copy_dictionary;
	printf("Copy dictionary:\n");
	while(tmp != (struct copy_dictionary_entry *)0)
	{
		printf("%lu %lu\n",(long unsigned int)(tmp->original),(long unsigned int)(tmp->copy));
		tmp = tmp->next;
	}	
}

void add_copy_dictionary_entry(void *o, void *c)
{
	struct copy_dictionary_entry *tmp;
	tmp = (struct copy_dictionary_entry *)malloc(sizeof(struct copy_dictionary_entry));

	tmp->original = o;
	tmp->copy = c;

	tmp->next = copy_dictionary;
	copy_dictionary = tmp;
}

void *get_copy_dictionary_entry(void *o)
{
	struct copy_dictionary_entry *tmp = copy_dictionary;
	if(o == (void *)0)	
		return o;
	while(tmp != (struct copy_dictionary_entry *)0)
	{
		if(tmp->original == o)
			return tmp->copy;
		if(tmp->copy == o)
			return tmp->original;
		tmp=tmp->next;
	}	
	return (void *)0;
}

struct scope *create_copy_scope(struct scope *s)
{
	struct scope *copied_graph = (struct scope *)malloc(sizeof(struct scope));	

	copied_graph->datum_ll = (struct datum_node *)0;
	copied_graph->operator_ll = (struct operator_node *)0;

	char expansion_str[10];

	sprintf(expansion_str,"%d",s->expansions);

	
	copied_graph->name = (char *)malloc(strlen(s->name)+1);
	strcpy(copied_graph->name,s->name);


	copied_graph->expansions = s->expansions;

	//increment expansions for next copying
	s->expansions++;

	//Let's replicate every element in both lists
	//Whenever we replicate, we add to dictionary

	struct datum_node *d_tmp = s->datum_ll; 
	struct datum_node *d_copy; 
	while(d_tmp != (struct datum_node *)0)
	{
		d_copy = (struct datum_node *)malloc(sizeof(struct datum_node));
		//Shallow copy
		*d_copy = *d_tmp;
		d_copy->data.name = strdup(d_tmp->data.name);

		if(d_tmp->data.ops != (struct op_datum_src *)0)
		{
			struct op_datum_src *src_original = d_tmp->data.ops;
			struct op_datum_src *src_copy = (struct op_datum_src *)malloc(sizeof(struct op_datum_src));

			d_copy->data.ops = src_copy;
			*src_copy = *src_original;

			src_original = src_original->next;

			while(src_original != (struct op_datum_src *)0)
			{
				src_copy->next = (struct op_datum_src *)malloc(sizeof(struct op_datum_src));
				src_copy = src_copy->next;		
				*src_copy = *src_original;		
				src_original = src_original->next;
			}
		}

		if(copied_graph->datum_ll == (struct datum_node *)0)
			copied_graph->datum_ll = d_copy;
		add_copy_dictionary_entry((void *)d_tmp, (void *)d_copy);
		d_tmp = d_tmp->next;
	}
	struct operator_node *o_tmp = s->operator_ll; 
	struct operator_node *o_copy; 
	while(o_tmp != (struct operator_node *)0)
	{
		o_copy = (struct operator_node *)malloc(sizeof(struct operator_node));
		//Shallow copy
		*o_copy = *o_tmp;
		o_copy->o.mappings = create_copy_mappings(o_tmp->o.mappings);
		
		if(copied_graph->operator_ll == (struct operator_node *)0)
			copied_graph->operator_ll = o_copy;
		add_copy_dictionary_entry((void *)o_tmp, (void *)o_copy);
		o_tmp = o_tmp->next;
	}

	//All elements are copied (shallow), i.e., pointing to original graph nodes
	//Now go through every element, and replace with reference to copied list
	d_tmp = copied_graph->datum_ll;
	while(d_tmp != (struct datum_node *)0)
	{
		
		struct operator *o = &(((struct operator_node *)get_copy_dictionary_entry((struct datum_node *)get_copy_dictionary_entry((void *)d_tmp)))->o); 

		//all operators should be in mapping list by now
		struct op_datum_src *ods_tmp = d_tmp->data.ops;
		while(ods_tmp != (struct op_datum_src *)0)
		{
			
			ods_tmp->op = get_copy_dictionary_entry((void *)(ods_tmp->op));
			
			ods_tmp = ods_tmp->next;
		}
		
		//replace next pointer
		d_tmp->next = (struct datum_node *)get_copy_dictionary_entry(d_tmp->next);
		d_tmp = d_tmp->next;
	}

	o_tmp = copied_graph->operator_ll;
	while(o_tmp != (struct operator_node *)0)
	{
		//replace previous and next
		o_tmp->next = (struct operator_node *)get_copy_dictionary_entry(o_tmp->next);
		o_tmp->previous = (struct operator_node *)get_copy_dictionary_entry(o_tmp->previous);

		//replace dest, src1, src2, mappings
		o_tmp->o.dest = (struct datum *)get_copy_dictionary_entry(((struct operator_node *)get_copy_dictionary_entry(o_tmp))->o.dest);
		o_tmp->o.src_1 = (struct datum *)get_copy_dictionary_entry(((struct operator_node *)get_copy_dictionary_entry(o_tmp))->o.src_1);
		o_tmp->o.src_2 = (struct datum *)get_copy_dictionary_entry(((struct operator_node *)get_copy_dictionary_entry(o_tmp))->o.src_2);

		o_tmp = o_tmp->next;
	}

	//Cleanup...
	//clear copy dictionary
	struct copy_dictionary_entry *copy_tmp;
	while(copy_dictionary != (struct copy_dictionary_entry *)0)
	{
		copy_tmp = copy_dictionary;
		copy_dictionary = copy_dictionary->next;
		free(copy_tmp);
	}
	
	return copied_graph;
}

struct mapping *create_copy_mappings(struct mapping *m)
{
	struct mapping *tmp;
	struct mapping *previous = (struct mapping *)0;

	struct mapping *root = (struct mapping *)0;

	if(m == (struct mapping *)0)
		return (struct mapping *)0;

	while(m != (struct mapping *)0)
	{
		tmp = (struct mapping *)malloc(sizeof(struct mapping));

		if(root == (struct mapping *)0)
		{
			root = tmp;
		}

		tmp->subnode = (char *)malloc(strlen(m->subnode)+1);
		tmp->argument = (char *)malloc(strlen(m->argument)+1);
		tmp->dir = m->dir;
		strcpy(tmp->subnode,m->subnode);
		strcpy(tmp->argument,m->argument);
		if(previous == (struct mapping *)0)
		{
			previous = tmp;
		}
		else
		{	
			previous->next = tmp;
		}
		m = m->next;
	}
	tmp->next = (struct mapping *)0;
	return root;
}


char *get_mapping_pair(struct mapping *m, char *s)
{
	while(strcmp(m->subnode,s)!=0)
	{
		m=m->next;
		if(m == (struct mapping *)0)
		{
			errors++;
			printf("Error: could not find mapping pair %s\n",s);
			break;
		}
	}
	if(m != (struct mapping *)0)
	{
		return m->argument;
	}
	else
		return (char *)0;
}

//Returns TRUE if s is found as an argument
int is_argument_in_mapping_pair(struct mapping *m, char *s)
{
	while(m != (struct mapping *)0)
	{
		if(strcmp(m->argument,s)==0)
		{
			return 1;
		}
		m=m->next;
	}
	return 0;
}


//Finds "d" on the right hand side (second operator) of m, and, if it exists, replaces it with "x"
void replace_datum_in_arg_mapping(struct datum *d, struct mapping *m, struct datum *x)
{
	while(m != (struct mapping *)0)
	{
		if(strcmp(m->argument,d->name)==0)
			m->argument = x->name;
		m = m->next;
	}
}


struct scope *replace_inputs(struct scope *s, struct mapping *m, struct scope *current)
{
	struct datum *input_datum;
	struct datum_node *datum_tmp;
	struct operator_node *tmp;

	struct operator_node *root_op = s->operator_ll;
	struct datum_node *root_d = s->datum_ll;

	int dbg_count = 0;

	//Finds all INPUT operators
	while(s->operator_ll != (struct operator_node *)0)
	{
		if(s->operator_ll->o.op == op_INPUT)
		{
			//no sources, just get destination datum pointer (sx)
			input_datum = s->operator_ll->o.dest;
			
			struct datum *x=get_datum(get_mapping_pair(m,input_datum->name),current);

			//We need to replace all mentions of that datum with mapped datum in current scope
			for(tmp = root_op; tmp != (struct operator_node *)0; tmp=tmp->next)
			{
				if(tmp->o.dest == input_datum)
					tmp->o.dest = x;
				if(tmp->o.src_1 == input_datum)
				{
					tmp->o.src_1 = x;
					tmp->o.pending_operators--;
				}
				if(tmp->o.src_2 == input_datum)
				{
					tmp->o.src_2 = x;
					tmp->o.pending_operators--;
				}

				
				if(tmp->o.op == op_EXPANSION)
				{
					//replace second (right hand side) datum "input_datum", if it exists, with "x"
					replace_datum_in_arg_mapping(input_datum, tmp->o.mappings, x);
				}
								
			}

			//And delete that datum in subgraph scope
			datum_tmp = s->datum_ll;
			if(&(datum_tmp->data) == input_datum)
			{
				root_d = s->datum_ll->next;
				dbg_count++;
				free(datum_tmp);
			}
			else
			{
				while(&(datum_tmp->next->data) != input_datum)
					datum_tmp = datum_tmp->next;
				datum_tmp->next = ((struct datum_node *)(input_datum))->next;
				dbg_count++;
				free(input_datum);
			}

			
			//delete input operator
			tmp=root_op;
			if(tmp == s->operator_ll)
			{
				tmp = tmp->next;
				free(root_op);
				root_op = tmp;
				s->operator_ll = tmp;
			}
			else
			{
				while(tmp->next != s->operator_ll)
					tmp = tmp->next;
				if(root_op == s->operator_ll)
				{
					root_op = s->operator_ll->next;
					s->operator_ll = s->operator_ll->next;
				}
				tmp->next = s->operator_ll->next;
				free(s->operator_ll);
				s->operator_ll = tmp->next;
			}
		}
		else
			s->operator_ll = s->operator_ll->next;
	}
	s->operator_ll = root_op;
	s->datum_ll = root_d;
	return s;
}

struct scope *replace_outputs(struct scope *s, struct mapping *m, struct scope *current)
{
	struct datum *output_datum;
	struct datum_node *datum_tmp;
	struct operator_node *tmp;

	struct operator_node *root_op = s->operator_ll;
	struct datum_node *root_d = s->datum_ll;

	int dbg_count = 0;

	//Finds all OUTPUT operators
	while(s->operator_ll != (struct operator_node *)0)
	{
		if(s->operator_ll->o.op == op_OUTPUT)
		{
			//no destination, just get source 1 datum pointer (sx)
			output_datum = s->operator_ll->o.src_1;
			struct datum *x=get_datum(get_mapping_pair(m,output_datum->name),current);

			//We need to replace all mentions of that datum with mapped datum in current scope
			for(tmp = root_op; tmp != (struct operator_node *)0; tmp=tmp->next)
			{
				if(tmp->o.dest == output_datum)
					tmp->o.dest = x;
				if(tmp->o.src_1 == output_datum)
					tmp->o.src_1 = x;
				if(tmp->o.src_2 == output_datum)
					tmp->o.src_2 = x;

				if(tmp->o.op == op_EXPANSION)
				{
					//replace second (right hand side) datum "input_datum", if it exists, with "x"
					replace_datum_in_arg_mapping(output_datum, tmp->o.mappings, x);
				}
			}

			//And delete that datum in subgraph scope
			datum_tmp = s->datum_ll;
			if(&(datum_tmp->data) == output_datum)
			{
				root_d = s->datum_ll->next;
				dbg_count++;
				free(datum_tmp);
			}
			else
			{
				while(&(datum_tmp->next->data) != output_datum)
					datum_tmp = datum_tmp->next;
				datum_tmp->next = ((struct datum_node *)(output_datum))->next;
				dbg_count++;
				free(output_datum);
			}

			//delete output operator
			tmp=root_op;
			if(tmp == s->operator_ll)
			{
				tmp = tmp->next;
				free(root_op);
				root_op = tmp;
				s->operator_ll = tmp;
			}
			else
			{
				while(tmp->next != s->operator_ll)
					tmp = tmp->next;
				if(root_op == s->operator_ll)
				{
					root_op = s->operator_ll->next;
					s->operator_ll = s->operator_ll->next;
				}
				tmp->next = s->operator_ll->next;
				free(s->operator_ll);
				s->operator_ll = tmp->next;
			}
		}
		else
			s->operator_ll = s->operator_ll->next;
	}
	s->operator_ll = root_op;
	s->datum_ll = root_d;
	return s;
}


struct scope *delete_outputs(struct scope *s, struct mapping *m, struct scope *current)
{
	struct datum *output_datum;
	struct datum_node *datum_tmp;
	struct operator_node *tmp;

	struct operator_node *root_op = s->operator_ll;
	struct datum_node *root_d = s->datum_ll;

	int dbg_count = 0;

	//Finds all OUTPUT operators
	while(s->operator_ll != (struct operator_node *)0)
	{
		if(s->operator_ll->o.op == op_OUTPUT)
		{
			//no destination, just get source 1 datum pointer (sx)
			output_datum = s->operator_ll->o.src_1;
			struct datum *x=get_datum(get_mapping_pair(m,output_datum->name),current);

			x->constructed = -1;
		}
		s->operator_ll = s->operator_ll->next;
	}
	s->operator_ll = root_op;
	s->datum_ll = root_d;
	return s;
}



struct scope *rename_data(struct scope *s, struct mapping *m)
{
	struct datum_node *d = s->datum_ll;
	struct operator_node *o = s->operator_ll;

	struct operator_node *root_op = s->operator_ll;
	struct datum_node *root_d = s->datum_ll;
	struct mapping *root_map = o->o.mappings;

	char count[4];

	sprintf(count,"%d",s->expansions);


	while(d != (struct datum_node *)0) 
	{
	
		if(!is_argument_in_mapping_pair(m,d->data.name))
		{
			char *tmp = (char *)malloc(strlen(d->data.name)+strlen(s->name)+strlen(count)+1);
			strcpy(tmp,s->name);
			strcat(tmp,count);
			strcat(tmp,d->data.name);

			d->data.name=tmp;

		}
		d=d->next;
	}
	
	//Now let's do the mappings
	for(o = s->operator_ll; o != (struct operator_node *)0; o=o->next)
	{
		if(o->o.op == op_EXPANSION)
		{
			struct mapping *map = o->o.mappings;
			if(map != (struct mapping *)0)
		
			while(map != (struct mapping *)0)
			{
			
				if(!is_argument_in_mapping_pair(m,map->argument))
				{
	
					char *tmp = (char *)malloc(strlen(map->argument)+strlen(s->name)+strlen(count)+1);
					strcpy(tmp,s->name);
					strcat(tmp,count);
					strcat(tmp,map->argument);
					
					map->argument = tmp;
					if(map == o->o.mappings)
						o->o.mappings->argument = tmp;

				}
		
				map = map->next;
			}
		}
	
	}	
	s->operator_ll = root_op;
	s->datum_ll = root_d;	
	s->operator_ll->o.mappings = root_map;
	return s;	
}



int inputs_all_constructed(struct scope *s, struct mapping *m, struct scope *current)
{
	struct datum *input_datum;

	struct operator_node *o = s->operator_ll;

	int constructed = 1;
	int cnt = 0;


	//Finds all INPUT operators
	while(s->operator_ll != (struct operator_node *)0)
	{

		if(s->operator_ll->o.op == op_INPUT)
		{
			//no sources, just get destination datum pointer (sx)
			input_datum = s->operator_ll->o.dest;

			struct datum *x=get_datum(get_mapping_pair(m,input_datum->name),current);

			if((x->constructed)==-1)
				constructed = -1;
			else
			{	
				if((x->constructed)==0)
				{	
					if(constructed != -1)
						constructed = 0;
				}
			}
		}
		s->operator_ll = s->operator_ll->next;
	}
	s->operator_ll = o;
	return constructed;
}

void rename_in_scope(char *scope_name,char *d_name, char *new_name)
{
	struct scope_node *s = scope_ll;
	struct datum_node *d = s->s.datum_ll; 
	//find correct scope
	while(strcmp(s->s.name,scope_name)!=0)
	{
		s=s->next;	
	}
	//got correct scope: find datum
	while(strcmp(d->data.name,d_name)==0)
	{
		d=d->next;
	}
	d->data.name = new_name;
}