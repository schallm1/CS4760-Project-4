#ifndef PCB_H
#define PCB_H

typedef struct {
	int pid;
	int priority;
	int nano;
	int sec;
	int tableIndex;
	int systemTime[0];

} PCB;

#endif
