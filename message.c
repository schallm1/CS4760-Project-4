typedef struct{
    long type;
    char msgString[100];
    int index;
    int quantUsed;
    int pid;		// Store the sending process's pid.

}message;