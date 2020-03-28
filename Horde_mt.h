#ifndef HORDE_MT_H
#define HORDE_MT_H

//Control structures for multithreaded operation


//operation type
enum t_operation{t_MASTER, t_SLAVE};


//command type
//synchronizes operation between master and slave threads:
//master can issue commands to slave
//t_RUN: run without constraints (master thread)
//t_NONE: no command, wait for a command (slave thread)
//t_COPY: update ready pool from master thread (slave thread)
//t_END: immediately finish operation
enum t_command{t_RUN, t_NONE,t_COPY, t_END};

//Thread control block
struct tcb
{
	//scope thread should evaluate
	char *scope;
	//thread operation (master/slave)
	//master, full control of graph
	//	allowed to transfer ready operators from its ready queue to slave ready queue
	//slave, cannot add operators to ready queue at startup
	//	must wait for master to pass them
	enum t_operation operation;
	//if slave, command to perform operation
	enum t_command command;
	//if master, pointer to slave thread command
	enum t_command *slave_command;
};

//for multithreaded execution time measurement
void pclock(char *msg, clockid_t cid);



#endif