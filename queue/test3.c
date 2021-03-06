/* signal test */
/* sigaction */
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>

#include "queues.h"

#define time_quantum 1
void signal_handler(int signo);
void child_handler(int signo);
void IO_handler(int signo);
void kill_handler(int signo);
//void tik(1);

int t_quantum=time_quantum;
int cpu_burst;
int io_burst;
int msgq;
int ret;
int key;


queue* rqueue;
queue* ioqueue;

typedef struct{
	int mtype;
	pid_t pid;
	int io_time;
	int cpu_time;
} msgbuf;

typedef struct{
	pid_t pid;
	int io_time;
	int cpu_time;
	int t_quantum;
} data;

msgbuf msg;

int main()
{
	struct sigaction old_sa;
	struct sigaction new_sa;
	struct sigaction new1_sa;
	memset(&new_sa, 0, sizeof(new_sa));
	memset(&new_sa, 0, sizeof(new_sa));
	new_sa.sa_handler = &signal_handler;
	new1_sa.sa_handler= &IO_handler;
	sigaction(SIGALRM, &new_sa, &old_sa);
	sigaction(SIGUSR1, &new1_sa, &old_sa);
	//need to make SIGALRM every 1sec


	pid_t pid;
	rqueue = createqueue();
	ioqueue = createqueue();


	for(int i =0; i<2; i++)
	{
		pid=fork();
		if(pid==0)
		{
			key = 0x12345;
			msgq = msgget(key, IPC_CREAT|0666);
			msg.mtype=getpid();
			msg.pid=getpid();
			msg.io_time = 4;
			msg.cpu_time = 4;
			ret = msgsnd(msgq,&msg,sizeof(msg),IPC_NOWAIT);
			new_sa.sa_handler = &child_handler;
			//new1_sa.sa_handler = &kill_handler;
			//if singaled reduce msg.bursttime and requent IO
			sigaction(SIGUSR1, &new_sa, &old_sa);
			//sigaction(SIGUSR2, &new1_sa, &old_sa);
			while(1);
		}
		else if(pid>0)
		{
			printf("%d\n",pid);
			key = 0x12345;
			msgq = msgget(key,IPC_CREAT|0666);
			printf("in\n");
			ret = msgrcv(msgq, &msg, sizeof(msg),pid,IPC_NOWAIT);
			printf("out\n");
			data *dataptr=(data*)malloc(sizeof(data));
			dataptr->pid = msg.pid;
			dataptr->io_time=msg.io_time;
			dataptr->cpu_time=msg.cpu_time;
			dataptr->t_quantum=time_quantum;
			enqueue(rqueue,dataptr);
		}
		else printf("fork error\n");
	}

	struct itimerval new_itimer, old_itimer;
	new_itimer.it_interval.tv_sec = 1;
	new_itimer.it_interval.tv_usec = 0;
	new_itimer.it_value.tv_sec = 1;
	new_itimer.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &new_itimer, &old_itimer);

	while (1);
	return 0;
}

void signal_handler(int signo)
{
	printf("tik\n");
	data *dataptr=NULL;
	if(emptyqueue(rqueue)&&emptyqueue(ioqueue))
		exit(0);
	if(emptyqueue(rqueue)!=1)
	{
		queuefront(rqueue,(void**)&dataptr);
		dataptr->t_quantum--;
		printf("%d\n",dataptr->pid);
		kill(dataptr->pid,SIGUSR1);
		if(dataptr->t_quantum==0)
		{
			dataptr->t_quantum=time_quantum;
			requeue(rqueue);
		}
	}
	if(emptyqueue(ioqueue)!=1)
	{
		queuefront(ioqueue,(void**)&dataptr);
		dataptr->io_time--;
		if(dataptr->io_time==0)
		{
			kill(dataptr->pid,SIGKILL);
			dequeue(ioqueue,(void**)&dataptr);
			free(dataptr);
		}
	}
	//handler code
}

void IO_handler(int signo)
{
	movqueue(rqueue,ioqueue);
}

void child_handler(int signo)
{
	msg.cpu_time--;
	if(msg.cpu_time==0)
	{//request IO
		kill(getppid(),SIGUSR1);
	}
}

void kill_handler(int signo)
{
	exit(0);
}










