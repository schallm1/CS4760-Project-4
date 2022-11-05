#include <stdio.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>

#include "queue.c"
#include "pcb.h"
#include "message.c"

#define MAX 10

int maxTimeBetweenNewProcsNS = 1000000000;
int maxTimeBetweenNewProcsSecs = 2;
int cap = 18;
int processes = 20;

unsigned int *ossClock;
PCB *pTable;
message m;

int main(int argc, char*argv[])
{
    FILE *log = fopen("logfile", "w");

    const int quant = 40000;
	int priority;
	unsigned int timeElapsed[2];
	unsigned int quantum;
    int index = atoi ( argv[1] );	
	int quantumUsed;
    int timeUsed;

    //get pids
    pid_t parPid = getppid();
    pid_t pid = getpid();

    //key creation
    key_t keyClock = 43967;
    key_t keyPCBTable = 43968;
    key_t keyMessage = 43969;
    key_t queueStatus = 43970;


    //shared memory for PCB and ossclock
    int clockID = shmget(keyClock, 100, IPC_CREAT | 0666);
    int pcbID = shmget(keyPCBTable, cap*sizeof(PCB), IPC_CREAT | 0666);
    int mID = msgget(keyMessage, IPC_CREAT | 0666 );
    int qID = shmget(queueStatus, sizeof(int), IPC_CREAT | 0666);

    //attach memory here
    ossClock = (unsigned int *) shmat(clockID, 0, 0);
    pTable = (PCB *) shmat(pcbID, 0, 0);
    int *qstat = (int *) shmat(qID, 0, 0);
    
    timeElapsed[0] = ossClock[0];
    timeElapsed[1] = ossClock[1];

    srand(time(NULL));
        msgrcv(mID, &m, sizeof(m), pid, 0);

            
            quantumUsed = rand() % 2;
            
            if(quantumUsed == 1)
            {
                m.quantUsed = 1;
				pTable[index].nano += quant;
                pTable[index].nano = pTable[index].nano % 1000000000;
				pTable[index].sec = pTable[index].sec / 1000000000;
				m.type = parPid;		
				m.pid = pid;		
				m.index = index;		
            }
            else if(quantumUsed == 0)
            {
                m.quantUsed = 0;
                timeUsed = rand() % (quantum -1);
				pTable[index].nano += timeUsed;
                pTable[index].nano = pTable[index].nano % 1000000000;
				pTable[index].sec = pTable[index].sec / 1000000000;
				m.type = parPid;		
				m.pid = pid;		
				m.index = index;
            }
				
                pTable[index].systemTime[0] = ossClock[0] + timeElapsed[0];
                pTable[index].systemTime[1] = ossClock[1] + timeElapsed[1];

			msgsnd( mID, &m, sizeof (m), 0 );

		// Send message to OSS indicating that the process has finished reached its quantum or slice of quantum.
		m.type = parPid;		
		m.pid = pid;		
		m.index = index;	
    


    return 0;
}