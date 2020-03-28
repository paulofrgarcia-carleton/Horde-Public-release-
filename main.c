#include"Horde.h"


unsigned int errors = 0;

#define DATUM(x) add_new_datum(#x,0,0)
#define CONST(x,y) add_new_datum(#x,y,1)
#define OPERATOR(z,a,x,y) add_new_operator(#z,a,#x,#y)
#define INPUT(x) add_new_operator(#x,op_INPUT, (char *)0,(char *)0);
#define OUTPUT(x) add_new_operator((char *)0,op_OUTPUT,#x,(char *)0);

#define SUBGRAPH(x) set_scope(#x);

#define EXPAND(x,y)	y add_new_subgraph(#x);
#define MAP_IN(x,y)	add_mapping(#x,#y,direction_IN)
#define MAP_OUT(x,y)	add_mapping(#x,#y,direction_OUT)

#define handle_error(msg) \
       do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error_en(en, msg) \
       do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

void pclock(char *msg, clockid_t cid)
{
   struct timespec ts;

   printf("%s", msg);
   if (clock_gettime(cid, &ts) == -1)
       handle_error("clock_gettime");
   printf("%4ld.%09ld\n", ts.tv_sec, ts.tv_nsec );
}

int to_transfer;

void print_total_time(clockid_t end_time)
{
	FILE *fp;
	fp = fopen("parallel.txt","a+");

	struct timespec te;
	clock_gettime(end_time, &te);
	fprintf(fp,"%d\t%4ld.%09ld\n",to_transfer,te.tv_sec,te.tv_nsec);

	fclose(fp);
}




int main(int argc, char *argv[])
{


  	/*
  	Attention: because of the way Horde processes programs, the top level program
  	(i.e., the subgraph) to evaluate cannot be a recursive function
  	*/


  /*
  Example AGP code

  Calculates the factorial of an input and outputs it
  Illustrated most features of AGP, including recursion, data merging, etc

  */

	SUBGRAPH(fact)
		DATUM(x);
    INPUT(x);
    DATUM(result);
    OUTPUT(result);
    DATUM(resulttrue);
    DATUM(resultfalse);
    DATUM(xis0);
    DATUM(iter);
    DATUM(nextiter);
    DATUM(xminus1);
    DATUM(xminus1cond);
    CONST(zero,0);
    CONST(one,1);
    OPERATOR(xis0,op_ISEQUAL,x,zero);
    OPERATOR(resulttrue,op_IF,xis0,one);
    OPERATOR(resultfalse,op_ELSE,xis0,iter);
    OPERATOR(iter,op_TIMES,x,nextiter);
    EXPAND(fact,MAP_IN(x,xminus1cond);MAP_OUT(result,nextiter););
    OPERATOR(result,op_MERGE,resulttrue,resultfalse);
    OPERATOR(xminus1,op_MINUS,x,one);
    OPERATOR(xminus1cond,op_ELSE,xis0,xminus1);

  SUBGRAPH(main)
		DATUM(x);
		DATUM(y);
		INPUT(x);
    EXPAND(fact,MAP_IN(x,x);MAP_OUT(result,y););
    OUTPUT(y);

	if(errors != 0)
	{
		printf("Compilation finished with %d errors.\n",errors);
		return 0;
	}
	print_all();


	pthread_t thread1, thread2;

	//Both threads evaluate the same scope
	//Point to same data and operators
	//Once one thread acquires an operator for processing (ready)
	//	marks it as taken so we don't double-evaluate

	struct tcb t1_control;
	//struct tcb t2_control;

	t1_control.scope = (char *)malloc(strlen("main")+1);
	strcpy(t1_control.scope, "main");

	//t2_control.scope = (char *)malloc(strlen("main")+1);
	//strcpy(t2_control.scope, "main");

	t1_control.operation = t_MASTER;
	//t2_control.operation = t_SLAVE;

	t1_control.command = t_RUN;
	//t2_control.command = t_NONE;

	//t1_control.slave_command = &(t2_control.command);
	//t2_control.slave_command = (enum t_command *)0;

	pthread_create( &thread1, NULL, eval, &t1_control);
  //pthread_create( &thread2, NULL, eval, &t2_control);

	
	pthread_join( thread1, NULL);
  //pthread_join( thread2, NULL); 



  	if(errors != 0)
	{
		printf("Evaluation finished with %d runtime errors.\n",errors);
		return 0;
	}
	printf("Evaluation finished successfully.\n");
	return 0;
}