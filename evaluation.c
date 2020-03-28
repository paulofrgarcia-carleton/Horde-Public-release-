#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"Horde.h"
#include <time.h>

extern unsigned int errors;



#define clean_operator	if(s->s.operator_ll->next == (struct operator_node *)0)\
						{\
							free(s->s.operator_ll);\
							s->s.operator_ll = l = (struct operator_node*)0;\
						}\
						else\
						{\
							if(l == s->s.operator_ll)\
							{\
								s->s.operator_ll = (s->s.operator_ll)->next;\
								(s->s.operator_ll)->previous = (struct operator_node*)0;\
								free(l);\
								l = s->s.operator_ll;\
							}\
							else\
							{\
								if(l->next == (struct operator_node *)0)\
								{\
									l->previous->next = (struct operator_node *)0;\
									free(l);\
									l = s->s.operator_ll;\
								}\
								else\
								{\
									struct operator_node* tmp=l;\
									(l->previous)->next = l->next;\
									(l->next)->previous = l->previous;\
									l = l->next;\
									free(tmp);\
								}\
							}\
						}

#define clean_this_operator(X)	if(s->s.operator_ll->next == (struct operator_node *)0)\
						{\
							free(s->s.operator_ll);\
							s->s.operator_ll = (struct operator_node*)0;\
						}\
						else\
						{\
							if(X == s->s.operator_ll)\
							{\
								s->s.operator_ll = (s->s.operator_ll)->next;\
								(s->s.operator_ll)->previous = (struct operator_node*)0;\
								free(X);\
							}\
							else\
							{\
								if(X->next == (struct operator_node *)0)\
								{\
									(X->previous)->next = (struct operator_node *)0;\
									free(X);\
								}\
								else\
								{\
									struct operator_node* tmp=X;\
									(X->previous)->next = X->next;\
									(X->next)->previous = X->previous;\
									free(tmp);\
								}\
							}\
						}

//This is only called upon initialization by "populate ready pool"
//adds all ready elements, adding to the head of the list 
#define add_to_ready_pool 	struct ready_node* newnode = (struct ready_node*)malloc(sizeof(struct ready_node));\
							newnode->op = l;\
							newnode->next = ready_pool;\
							newnode->prev = (struct ready_node*)0;\
							if(ready_pool != (struct ready_node*)0) {\
								ready_pool->prev = newnode;\
							}\
							ready_pool = newnode;\
							if(ready_pool_tail == (struct ready_node *)0)\
								ready_pool_tail = ready_pool;\
							ready_pool_length++;\
							l->taken=1;


#define copy_to_ready_pool 	struct ready_node* newnode = (struct ready_node*)malloc(sizeof(struct ready_node));\
							newnode->op = test_tmp;\
							newnode->next = ready_pool;\
							newnode->prev = (struct ready_node*)0;\
							if(ready_pool != (struct ready_node*)0) {\
								ready_pool->prev = newnode;\
							}\
							ready_pool = newnode;\
							if(ready_pool_tail == (struct ready_node *)0)\
								ready_pool_tail = ready_pool;

//Called during program evaluation
//adds a new node to the tail of the ready queue
#define add_node_to_ready_pool(NODE) struct ready_node* newnode = (struct ready_node*)malloc(sizeof(struct ready_node));\
									newnode->op = (struct operator_node *)NODE;\
									newnode->next = (struct ready_node*)0;\
									newnode->prev = ready_pool_tail;\
									ready_pool_tail->next = newnode;\
									ready_pool_tail = newnode;\
									ready_pool_length++;\
									((struct operator_node *)NODE)->taken = 1;

#define remove_from_ready_pool	if(ready_pool == ready_pool_tail)\
								{\
									free(ready_pool);\
									ready_pool = ready_pool_tail = ready_pool_current = (struct ready_node*)0;\
								}\
								else\
								{\
									if(ready_pool_current == ready_pool)\
									{\
										ready_pool = ready_pool->next;\
										ready_pool->prev = (struct ready_node*)0;\
										free(ready_pool_current);\
										ready_pool_current = ready_pool;\
									}\
									else\
									{\
										if(ready_pool_current == ready_pool_tail)\
										{\
											ready_pool_tail = ready_pool_tail->prev;\
											ready_pool_tail->next = (struct ready_node*)0;\
											free(ready_pool_current);\
											ready_pool_current = ready_pool;\
										}\
										else\
										{\
											struct ready_node* tmp=ready_pool_current;\
											ready_pool_current->prev->next = ready_pool_current->next;\
											ready_pool_current->next->prev = ready_pool_current->prev;\
											ready_pool_current = ready_pool_current->next;\
											free(tmp);\
										}\
									}\
								}\
								ready_pool_length--;\
								if(ready_pool != (struct ready_node *)0)\
									l = ready_pool_current->op;\
								else\
									l = (struct operator_node *)0;
					
/*
	Initializes the ready pool upon program start with all top-level inputs
	and operators that just depend on constants 
*/								
#define populate_ready_pool l = s->s.operator_ll;\
							while(l != (struct operator_node*)0)\
							{\
								if(l->o.pending_operators == 0)\
								{\
									if(l->taken == 0)\
									{	add_to_ready_pool}\
								}\
								l = l->next;\
							}

/*
	Identical to "populate_ready_pool", except it's only used by slave thread
	to copy marked nodes (by master) to its own ready pool
*/								
#define copy_from_ready_pool l = s->s.operator_ll;\
							while(l != (struct operator_node*)0)\
							{\
								if(l->o.pending_operators == 0)\
								{\
									if(l->taken == -1)\
									{	add_to_ready_pool}\
								}\
								l = l->next;\
							}


#define clean_ready_pool 	while (ready_pool != (struct ready_node*)0)\
							{\
								remove_from_ready_pool\
							}


#define get_next_ready 	if(ready_pool == ready_pool_tail)\
						{}\
						else\
						{\
							if(ready_pool_current == ready_pool_tail)\
								ready_pool_current = ready_pool;\
							else\
							{\
								ready_pool_current = ready_pool_current->next;\
							}\
						}\
						l = ready_pool_current->op;




void propagate_destruction_d(struct datum *d, struct scope_node* s);
void propagate_destruction_o(struct operator *o, struct scope_node* s);
//If an operator destroys its target datum, "eval" calls this function to propagate the destruction
						//throughout the graph
//I.e., pruning everying in its path that can be pruned
void propagate_destruction_d(struct datum *d, struct scope_node* s)
{
	struct operator *tmp_op;
	if(d != (struct datum *)0)
	{
		//check if datum has already been destroyed to prevent infinite recursion
		if(d->constructed == -1)
			return;
		d->constructed = -1;
		while(d->ops != (struct op_datum_src *)0)
		{
			tmp_op = d->ops->op;
			d->ops->op = (struct operator *)0;
			propagate_destruction_o(tmp_op,s);
			d->ops = d->ops->next;
		}
	}
}


void propagate_destruction_o(struct operator *o, struct scope_node* s)
{
	struct operator_node* tmp;
	struct mapping *m_tmp;

	//never destroy MERGE operators
	if(((o == (struct operator *)0)) || (o->op == op_MERGE))
	{
		return;
	}


	if(o->dest != (struct datum *)0)
	{
		propagate_destruction_d(o->dest,s);
	}

	//In case operator is EXPANSION
	//Also need to check all its output mappings and propagate there
	if(o->op == op_EXPANSION)
	{
		while((o->mappings) != ((struct mapping *)0))
		{
			m_tmp = o->mappings;
			o->mappings = (struct mapping *)0;
			if(m_tmp->dir == direction_OUT)
				propagate_destruction_d(get_datum(m_tmp->argument,&(s->s)),s);
			o->mappings = m_tmp->next;
		}
	}

	clean_this_operator(((struct operator_node *)o));

}

void timespec_diff(struct timespec *start, struct timespec *stop,
                   struct timespec *result)
{
    if ((stop->tv_nsec - start->tv_nsec) < 0) {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }

    return;
}

extern int to_transfer;


void calc_time(clockid_t start_time, clockid_t end_time)
{
	FILE *fp;
	fp = fopen("test_rq_results.txt","a+");

	struct timespec ts, te, tr;
	clock_gettime(start_time, &ts);
	clock_gettime(end_time, &te);
	timespec_diff(&ts, &te, &tr);
	fprintf(fp,"%d\t%4ld.%09ld\n",to_transfer,tr.tv_sec,tr.tv_nsec);

	fclose(fp);
}

//runs the program
void *eval(void *my_tcb)
{
	char *scope_name = ((struct tcb *)my_tcb)->scope;

	//This is the pointer to the scope to evaluate
	struct scope_node* s;

	//This points to the current operator we're evaluating
	//"l" is necessarily in the ready list
	struct operator_node* l;

	clockid_t cid;
	//counts number of evaluated operators
	int evaluated = 0;
  

	struct operator_node* tmp;
	char c;
	
	
	s = get_scope((char *)scope_name);

	if(s == (struct scope_node *)0)
	{
		printf("Could not find a scope named \"%s\". Exiting...\n",(char *)scope_name);
		return (void *)0;
	}

	struct ready_node* ready_pool;
	struct ready_node* ready_pool_tail;
	struct ready_node* ready_pool_current;
	ready_pool = (struct ready_node*)0;
	ready_pool_tail = (struct ready_node*)0;

	//keeps consistent track of number of ready nodes
	int ready_pool_length = 0;

	
	//Called just once, when program begins execution
	//after that, no "population", just add/remove as we go along
	//We only do this if we are a master thread: if slave, wait further down...
	if(((struct tcb *)my_tcb)->operation == t_MASTER)
		populate_ready_pool;


	while(1)
	{		
		// Pick the next node in the ready pool
		if(ready_pool != (struct ready_node *)0)
		{	
			ready_pool_current = ready_pool;
			l = ready_pool_current->op;


			
			/*
			If we want to transfer nodes to the slave thread, do it here
			"if" and "while" conditions dictated by the reconfiguration algorithm

			if(...)
			{

				while(...)
				{
					ready_pool->op->taken = -1;
					remove_from_ready_pool;
					transfered++;	
				}
				*(((struct tcb *)my_tcb)->slave_command) = t_COPY;
			}*/
			
		}
		else
		{
			//If we finished evaluating the entire subgraph
			if(s->s.operator_ll == (struct operator_node *)0)
				l = (struct operator_node *)0;
			//else, we're waiting for something to become ready
			//Meaning we are a slave thread
			else
			{
				//if our current command is "t_NONE", wait doing nothing
				while(((struct tcb *)my_tcb)->command == t_NONE){}

				//check for end of execution
				if(((struct tcb *)my_tcb)->command == t_END)
					return (void *)0;

				//Then we have to figure out how to copy ready pool and evaluate
				//our state is then back to RUN to execute
				if(((struct tcb *)my_tcb)->command == t_COPY)
				{
					copy_from_ready_pool;

					((struct tcb *)my_tcb)->command = t_RUN;
				}
				ready_pool_current = ready_pool;
				if(ready_pool != (struct ready_node *)0)
					l = ready_pool_current->op;
				else
					l = (struct operator_node *)0;		
			}
			
		}
		
		// Check against null node (program termination?)
		if(l == (struct operator_node *)0)
			break;

		
		//Now we're guaranteed to only evaluate ready nodes
		while(l != (struct operator_node *)0)
		{
			
			switch(l->o.op)
			{
				case op_INPUT: 
				{
					printf("input (%s): ",l->o.dest->name);
					scanf("%d",&(l->o.dest->value));
					l->o.dest->constructed = 1;
					//Update readiness of operator that uses constructed datum as source 
					struct op_datum_src *op_tmp = l->o.dest->ops;
					while(op_tmp != (struct op_datum_src *)0)
					{
						op_tmp->op->pending_operators--;
						if(op_tmp->op->pending_operators == 0)
						{
							add_node_to_ready_pool(op_tmp->op);
						}
						op_tmp = op_tmp->next;
					}
					//Removes from operator list
					clean_operator;
					//must also remove from ready pool if it's been evaluated
					remove_from_ready_pool;
				} break;
				case op_OUTPUT:
				{
					if(l->o.src_1->constructed == 1)
					{
						printf("output (%s): %d\n",l->o.src_1->name,l->o.src_1->value);
						//Removes from operator list
						//Output doesn't construct anything, hence no operator can be made ready
						clean_operator;						
						//must also remove from ready pool if it's been evaluated
						remove_from_ready_pool;
					
					}
					else
					{
						if((l->o.src_1->constructed==-1))
						{
							//Removes from operator list
							clean_operator;
							//must also remove from ready pool if it's been evaluated
							remove_from_ready_pool;
						}
						else
							get_next_ready;
					}
				} break;
				case op_PLUS:
				{
					if((l->o.src_1->constructed == 1) && (l->o.src_2->constructed == 1))
					{
						l->o.dest->value = l->o.src_1->value + l->o.src_2->value;
						l->o.dest->constructed = 1;

						//Update readiness of operator that uses constructed datum as source 
						struct op_datum_src *op_tmp = l->o.dest->ops;
						while(op_tmp != (struct op_datum_src *)0)
						{
							op_tmp->op->pending_operators--;
							if(op_tmp->op->pending_operators == 0)
							{
								add_node_to_ready_pool(op_tmp->op);
							}
							op_tmp = op_tmp->next;
						}
						//Removes from operator list
						clean_operator;
						//must also remove from ready pool if it's been evaluated
						remove_from_ready_pool;
					}
					else
					{
						if((l->o.src_1->constructed==-1) || (l->o.src_2->constructed==-1))
						{
							propagate_destruction_d(l->o.dest,s);
							//Removes from operator list
							clean_operator;
							//must also remove from ready pool if it's been evaluated
							remove_from_ready_pool;
						}
						else
							get_next_ready;
					}
				} break;
				case op_TIMES:
				{
					if((l->o.src_1->constructed == 1) && (l->o.src_2->constructed == 1))
					{
						l->o.dest->value = l->o.src_1->value * l->o.src_2->value;
						l->o.dest->constructed = 1;
						//Update readiness of operator that uses constructed datum as source 
						struct op_datum_src *op_tmp = l->o.dest->ops;
						while(op_tmp != (struct op_datum_src *)0)
						{
							op_tmp->op->pending_operators--;
							if(op_tmp->op->pending_operators == 0)
							{
								add_node_to_ready_pool(op_tmp->op);
							}
							op_tmp = op_tmp->next;
						}
						//Removes from operator list
						clean_operator;
						//must also remove from ready pool if it's been evaluated
						remove_from_ready_pool;
					}
					else
					{
						if((l->o.src_1->constructed==-1) || (l->o.src_2->constructed==-1))
						{
							propagate_destruction_d(l->o.dest,s);
							//Removes from operator list
							clean_operator;
							//must also remove from ready pool if it's been evaluated
							remove_from_ready_pool;
						}
						else
							get_next_ready;
					}
				} break;
				case op_MERGE:
				{
					//First source that is constructed 
					if((l->o.src_1->constructed == 1) || (l->o.src_2->constructed == 1))
					{

						if((l->o.src_1->constructed == 1))
							l->o.dest->value = l->o.src_1->value;
						else
							l->o.dest->value = l->o.src_2->value;
						l->o.dest->constructed = 1;
						//Update readiness of operator that uses constructed datum as source 
						struct op_datum_src *op_tmp = l->o.dest->ops;
						while(op_tmp != (struct op_datum_src *)0)
						{
							op_tmp->op->pending_operators--;
							if(op_tmp->op->pending_operators == 0)
							{
								add_node_to_ready_pool(op_tmp->op);
							}
							op_tmp = op_tmp->next;
						}
						//Removes from operator list
						clean_operator;
						//must also remove from ready pool if it's been evaluated
						remove_from_ready_pool;
					}
					else
					{
						if((l->o.src_1->constructed==-1) && (l->o.src_2->constructed==-1))
						{
							propagate_destruction_d(l->o.dest,s);
							//Removes from operator list
							clean_operator;
							//must also remove from ready pool if it's been evaluated
							remove_from_ready_pool;
						}
						else
							get_next_ready;
					}
				} break;
				case op_MINUS:
				{
					
					if ((l->o.src_1->constructed == 1) && (l->o.src_2->constructed == 1))
					{
						l->o.dest->value = l->o.src_1->value - l->o.src_2->value;
						l->o.dest->constructed = 1;
					
						//Update readiness of operator that uses constructed datum as source 
						struct op_datum_src *op_tmp = l->o.dest->ops;
						while(op_tmp != (struct op_datum_src *)0)
						{
							op_tmp->op->pending_operators--;
							if(op_tmp->op->pending_operators == 0)
							{
								
								add_node_to_ready_pool(op_tmp->op);
							}
							op_tmp = op_tmp->next;
						}
						//Removes from operator list
						clean_operator;
						//must also remove from ready pool if it's been evaluated
						remove_from_ready_pool;
					}
					else
					{
						if((l->o.src_1->constructed==-1) || (l->o.src_2->constructed==-1))
						{
							propagate_destruction_d(l->o.dest,s);
							//Removes from operator list
							clean_operator;
							//must also remove from ready pool if it's been evaluated
							remove_from_ready_pool;
						}
						else
							get_next_ready;
					}
				} break;
				case op_ISEQUAL:
				{
					if((l->o.src_1->constructed == 1) && (l->o.src_2->constructed==1))
					{
						l->o.dest->value = (l->o.src_1->value == l->o.src_2->value ? 1: 0);
						l->o.dest->constructed = 1;
						//Update readiness of operator that uses constructed datum as source 
						struct op_datum_src *op_tmp = l->o.dest->ops;
						while(op_tmp != (struct op_datum_src *)0)
						{
							op_tmp->op->pending_operators--;
							if(op_tmp->op->pending_operators == 0)
							{
								add_node_to_ready_pool(op_tmp->op);
							}
							op_tmp = op_tmp->next;
						}
						//Removes from operator list
						clean_operator;
						//must also remove from ready pool if it's been evaluated
						remove_from_ready_pool;
					}
					else
					{
						if((l->o.src_1->constructed==-1) || (l->o.src_2->constructed==-1))
						{
							propagate_destruction_d(l->o.dest,s);
							//Removes from operator list
							clean_operator;
							//must also remove from ready pool if it's been evaluated
							remove_from_ready_pool;
						}
						else
							get_next_ready;
					}
				} break;
				case op_ISLESS:
				{
					if((l->o.src_1->constructed==1) && (l->o.src_2->constructed==1))
					{
						l->o.dest->value = (l->o.src_1->value < l->o.src_2->value ? 1: 0);
						l->o.dest->constructed = 1;
						//Update readiness of operator that uses constructed datum as source 
						struct op_datum_src *op_tmp = l->o.dest->ops;
						while(op_tmp != (struct op_datum_src *)0)
						{
							op_tmp->op->pending_operators--;
							if(op_tmp->op->pending_operators == 0)
							{
								add_node_to_ready_pool(op_tmp->op);
							}
							op_tmp = op_tmp->next;
						}
						//Removes from operator list
						clean_operator;
						//must also remove from ready pool if it's been evaluated
						remove_from_ready_pool;
					}
					else
					{
						if((l->o.src_1->constructed==-1) || (l->o.src_2->constructed==-1))
						{
							propagate_destruction_d(l->o.dest,s);
							//Removes from operator list
							clean_operator;
							//must also remove from ready pool if it's been evaluated
							remove_from_ready_pool;
						}
						else
							get_next_ready;
					}
				} break;
				case op_ISGREATER:
				{
					if((l->o.src_1->constructed==1) && (l->o.src_2->constructed==1))
					{
						l->o.dest->value = (l->o.src_1->value > l->o.src_2->value ? 1: 0);
						l->o.dest->constructed = 1;
						//Update readiness of operator that uses constructed datum as source 
						struct op_datum_src *op_tmp = l->o.dest->ops;
						while(op_tmp != (struct op_datum_src *)0)
						{
							op_tmp->op->pending_operators--;
							if(op_tmp->op->pending_operators == 0)
							{
								add_node_to_ready_pool(op_tmp->op);
							}
							op_tmp = op_tmp->next;
						}
						//Removes from operator list
						clean_operator;
						//must also remove from ready pool if it's been evaluated
						remove_from_ready_pool;
					}
					else
					{
						if((l->o.src_1->constructed==-1) || (l->o.src_2->constructed==-1))
						{
							propagate_destruction_d(l->o.dest,s);
							//Removes from operator list
							clean_operator;
							//must also remove from ready pool if it's been evaluated
							remove_from_ready_pool;
						}
						else
							get_next_ready;
					}
				} break;
				case op_IF:
				{
					if((l->o.src_1->constructed==1) && (l->o.src_2->constructed==1))
					{
						if(l->o.src_1->value) 
						{
							l->o.dest->value = l->o.src_2->value;
							l->o.dest->constructed = 1;
							//Update readiness of operator that uses constructed datum as source 
							struct op_datum_src *op_tmp = l->o.dest->ops;
							while(op_tmp != (struct op_datum_src *)0)
							{
								op_tmp->op->pending_operators--;
								if(op_tmp->op->pending_operators == 0)
								{
									add_node_to_ready_pool(op_tmp->op);
								}
								op_tmp = op_tmp->next;
							}
						}
						else
						{
							propagate_destruction_d(l->o.dest,s);
						}
						
						//Removes from operator list
						clean_operator;
						//must also remove from ready pool if it's been evaluated
						remove_from_ready_pool;
					}
					else
					{
						if((l->o.src_1->constructed==-1) || (l->o.src_2->constructed==-1))
						{
							propagate_destruction_d(l->o.dest,s);
							//Removes from operator list
							clean_operator;
							//must also remove from ready pool if it's been evaluated
							remove_from_ready_pool;
						}
						else
							get_next_ready;
					}
				} break;
				case op_ELSE:
				{
					if((l->o.src_1->constructed==1) && (l->o.src_2->constructed==1))
					{
						if(!(l->o.src_1->value))
						{
							l->o.dest->value = l->o.src_2->value;
							l->o.dest->constructed = 1;
							//Update readiness of operator that uses constructed datum as source 
							struct op_datum_src *op_tmp = l->o.dest->ops;
							while(op_tmp != (struct op_datum_src *)0)
							{
								op_tmp->op->pending_operators--;
								if(op_tmp->op->pending_operators == 0)
								{
									add_node_to_ready_pool(op_tmp->op);
								}
								op_tmp = op_tmp->next;
							}
						}
						else
						{
							propagate_destruction_d(l->o.dest,s);
						}
						
						//Removes from operator list
						clean_operator;
						
						//must also remove from ready pool if it's been evaluated
						remove_from_ready_pool;
						
					}
					else
					{
						if((l->o.src_1->constructed==-1) || (l->o.src_2->constructed==-1))
						{
							propagate_destruction_d(l->o.dest,s);
							//Update readiness of operator that uses constructed datum as source 
							struct op_datum_src *op_tmp = l->o.dest->ops;
							while(op_tmp != (struct op_datum_src *)0)
							{
								op_tmp->op->pending_operators--;
								if(op_tmp->op->pending_operators == 0)
								{
									add_node_to_ready_pool(op_tmp->op);
								}
								op_tmp = op_tmp->next;
							}
							//Removes from operator list
							clean_operator;
							//must also remove from ready pool if it's been evaluated
							remove_from_ready_pool;
						}
						else
							get_next_ready;
					}
				} break;
				case op_EXPANSION:
				{
					struct scope *sub_scope = &(get_scope(l->o.scope))->s;
					//get mappings
					struct mapping *mappings = l->o.mappings;

					int created = 0;

					//Find all data mapped as input
					created = inputs_all_constructed(sub_scope,mappings,&(s->s));
				
					//Find if they have been created in current scope
					if(created == 0)
					{
						get_next_ready;
					}
					else
					{
						if(created == -1)
						{
							//sets all output mappings to -1
							sub_scope=delete_outputs(sub_scope, mappings, &(s->s));
							//Removes from operator list
							clean_operator;
							//must also remove from ready pool if it's been evaluated
							remove_from_ready_pool;
						}
						else
						{
							sub_scope = create_copy_scope(sub_scope);
							mappings = create_copy_mappings(mappings);

							//Let's find all INPUT operators, delete them, find data they point to, delete them, 
							//replace with data from current scope TODO
							sub_scope=replace_inputs(sub_scope, mappings, &(s->s));

							//Eliminating all data connected to outputs
							//And replacing all mentions to that data (operator destinations) to mentions to data in current
							//	scope, according to mappings
							sub_scope=replace_outputs(sub_scope, mappings, &(s->s));
							//What's wrong if we rename data here is that we are renaming data that has already been replaced
							//from top scope (which should remain the same!)
							//So, can we pass mappings as an argument to filter - i.e., don't rename if it shows up in mappings?!
							sub_scope=rename_data(sub_scope, mappings);

							//Must add all data that are not input or output to current scope data list
							//Let-s add this list to our scope's data list
							struct datum_node *last_datum = s->s.datum_ll;
							while(last_datum->next != (struct datum_node *)0)
								last_datum = last_datum->next;
							last_datum->next = sub_scope->datum_ll;
							//Add all operators in new scope to ready queue, if they're ready
							//get last operator in new subgraph
							struct operator_node *last = sub_scope->operator_ll;

							while(last->next != (struct operator_node *)0)
							{
								if(last->o.pending_operators == 0)
								{	
									add_node_to_ready_pool((struct operator *)last);
								}
								last=last->next;
							}
							if(last->o.pending_operators == 0)
							{	
								add_node_to_ready_pool((struct operator *)last);
							}
							last->next = s->s.operator_ll;
							s->s.operator_ll->previous = last;
							s->s.operator_ll = sub_scope->operator_ll;
							//Removes from operator list
							clean_operator;
							//must also remove from ready pool if it's been evaluated
							remove_from_ready_pool;
						}
					}			
				} break;
				default: 
				{
					printf("ERROR: Unknown type\n");
					//Removes from operator list
					clean_operator;
					//must also remove from ready pool if it's been evaluated
					remove_from_ready_pool;
				} 
			}
			print_one(s);
			evaluated++;
		}
		
	}
	return (void *)0;
}


