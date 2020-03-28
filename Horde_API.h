#ifndef HORDE_API_H
#define HORDE_API_H




struct datum *get_datum(char *n, struct scope *search_scope);

struct scope_node *get_scope(char *s);


void add_new_datum(char* x, int y, int c);
void add_new_operator(char *z, enum operation op, char *x, char *y);

void set_scope(char *scope);

struct scope_node *get_scope(char *s);
struct datum *get_datum_in_current_scope(char *n);


void print_all();
void print_one(struct scope_node* s);

void print_operators(struct operator_node *o);
void print_mappings(struct mapping *m);

void add_mapping(char *x, char *y, enum direction d);
void add_new_subgraph(char *scope);

void print_data_uses(struct scope *s);


struct scope *create_copy_scope(struct scope * s);
struct mapping *create_copy_mappings(struct mapping *m);

struct scope *replace_inputs(struct scope *s, struct mapping *m, struct scope *current);

int inputs_all_constructed(struct scope *s, struct mapping *m, struct scope *current);

struct scope *replace_outputs(struct scope *s, struct mapping *m, struct scope *current);

struct scope *delete_outputs(struct scope *s, struct mapping *m, struct scope *current);

char *get_mapping_pair(struct mapping *m, char *s);

struct scope *rename_data(struct scope *s, struct mapping *m);

void replace_datum_in_arg_mapping(struct datum *d, struct mapping *m, struct datum *x);

int is_argument_in_mapping_pair(struct mapping *m, char *s);

void dbg_print_operators(struct operator_node *s);

void rename_in_scope(char *scope_name,char *d_name, char *new_name);

void *eval(void *my_tcb);


//Auxiliary functions for debug purposes
void dbg_print_operators(struct operator_node *s);

void print_all();
void print_one(struct scope_node* s);

void print_operators(struct operator_node *o);
void print_mappings(struct mapping *m);
void dbg_print_operators(struct operator_node *s);

void *get_copy_dictionary_entry(void *o);


void dbg_print_op_ll(struct operator_node *l);
void dbg_print_ready_pool(struct ready_node *l);

#endif