#include <stdio.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>
#include "queue.c"
#include "pcb.h"
#include "message.c"
#define MAX 10


int maxTimeBetweenNewProcsNS = 1000000000;
int maxTimeBetweenNewProcsSecs = 2;
int processCap = 20;
int vector[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


unsigned int *ossClock;
PCB *pTable;
message m;

int main()
{
 

    srand(time(NULL));
    int size = sizeof(PCB) *18;
    FILE *log = fopen("logfile", "w");
    pid_t temp;

    pid_t thisPid = getpid();
    pid_t forkPID = getpid();
    int index;
    //key creation
    key_t keyClock = 43967;
    key_t keyPCBTable = 43968;
    key_t keyMessage = 43969;
    key_t queueStatus = 43970;

    //create shared memory for process control block, system clock, and message queue
    int clockID = shmget(keyClock, 100, IPC_CREAT | 0666);
    int pcbID = shmget(keyPCBTable, size, IPC_CREAT | 0666);
    int mID = msgget(keyMessage, IPC_CREAT | 0666 );
    int qID = shmget(queueStatus, sizeof(int), IPC_CREAT | 0666);

    //attach memory
    ossClock = (unsigned int *) shmat(clockID, 0, 0);
    pTable = (PCB *) shmat(pcbID, 0, 0);
    int *qstat = (int *) shmat(qID, 0, 0);

    
    *qstat = 0;


    //initialize clock 
    ossClock[0] = 0; //seconds
    ossClock[1] = 0; //nanoseconds

    //initialize  PCB
    for (int i = 0; i < 18; i++)
    {
        pTable[i].tableIndex = i;
    }

    //queue initialization
    Queue* lowPriority = createQueue(100);
    Queue* highPriority = createQueue(100);
    
    int processCount = 0;
    int newProcessId = 0;
    int priority;
    int processStatus = 0;
    int vecIndex;
    int nextProcessNS = rand() % maxTimeBetweenNewProcsNS;
    int nextProcessSec = rand() % maxTimeBetweenNewProcsSecs;
    int range;

    for(int i =0; i<processCap;i++)
    {
        if((ossClock[0]+ossClock[1]) < (nextProcessNS+nextProcessSec))
        {
            ossClock[0] = nextProcessNS;
            ossClock[1] = nextProcessSec;
            nextProcessNS = rand() % maxTimeBetweenNewProcsNS;
            nextProcessSec = rand() % maxTimeBetweenNewProcsSecs;
            nextProcessNS += ossClock[0];
            nextProcessSec += ossClock[1];
            processStatus = 1;
        }

        if(processStatus)
        {
            for(int j = 0; j<processCap; j++)
            {
                if(vector[j] == 0)
                {
                    vecIndex = j;
                    break;
                }
            }
            forkPID = fork();
            perror("");
            if(forkPID==0)
            {
                vector[vecIndex] = getpid();
                char index[3];
                sprintf(index, "%d", vecIndex);
                execl("./child", "child", index, NULL);

            }
            
            range = ( rand() % ( 20 ) ) + 1;
            if(range <=4 && range>0)
            {
                priority = 1;
            }
            else
                priority = 0;

                pTable[vecIndex].pid = forkPID;
                pTable[vecIndex].priority = priority;
                pTable[vecIndex].nano = 0;
                pTable[vecIndex].sec = 0;

                if(priority == 1)
                {
                    fprintf(log, "High Priority Process with id: %d has just been created. It will be put in the high priority queue.\n", forkPID);
                    enqueue(highPriority, forkPID);
                }
                else
                {
                    fprintf(log, "Low Priority Process with id: %d has just been created. It will be put in the low priority queue.\n", forkPID);
                    enqueue(lowPriority, forkPID);
                }
                processCount++;
        }
    }
    do
    {
        //if high priority processes are queued 
        if(!isEmpty(highPriority))
        {
            temp = dequeue(highPriority);
            m.type = temp;

            msgsnd(mID, &m, sizeof(m), 0);
            sleep(2);
            fprintf(log, "Process starting message sent.\n");
            msgrcv(mID, &m, sizeof(m), thisPid, 0);

            fprintf(log, "Process %d was able to run for %d seconds and %d nanoseconds.\n", m.pid, pTable[m.index].sec, pTable[m.index].nano);

            if (m.quantUsed == 1)
            {
                fprintf(log, "Entire time quantum of %d seconds and %d nanoseconds was used.\n", pTable[m.index].sec, pTable[m.index].nano);
            }
            else if (m.quantUsed == 0)
            {
                fprintf(log, "Only %d seconds and %d nanoseconds of time quantum were used.\n", pTable[m.index].sec, pTable[m.index].nano);
            }

            ossClock[0] += (ossClock[1] / 1000000000) + pTable[m.pid].systemTime[0];
			ossClock[1] += (ossClock[1] % 1000000000) + pTable[m.pid].systemTime[1];
        }
        //all other queued processes
        else if (!isEmpty(lowPriority))
        {
            *qstat = 1;
            temp = dequeue(lowPriority);
            m.type = temp;

            msgsnd(mID, &m, sizeof(m), 0);
            fprintf(log, "Process starting message sent.\n");
            msgrcv(mID, &m, sizeof(m), thisPid, 0);

            fprintf(log, "Process %d has finished running.\n", m.pid);
            if (m.quantUsed == 1)
            {
                fprintf(log, "Entire time quantum of %d seconds and %d nanoseconds was used.\n", pTable[m.pid].sec, pTable[m.pid].nano);
            }
            else if (m.quantUsed == 0)
            {
                fprintf(log, "Only %d seconds and %d nanoseconds of time quantum were used.\n", pTable[m.pid].sec, pTable[m.pid].nano);
            }
            
			ossClock[0] += (ossClock[1] / 1000000000) + pTable[m.pid].systemTime[0];
			ossClock[1] += (ossClock[1] % 1000000000) + pTable[m.pid].systemTime[1];
        }
        else
        break;
    }while(1);
                
    //delete shared memory
    shmctl(pcbID, IPC_RMID, NULL);
    shmctl(clockID, IPC_RMID, NULL);
    shmctl(qID, IPC_RMID, NULL);
    msgctl(mID, IPC_RMID, NULL);


    return 0;
}