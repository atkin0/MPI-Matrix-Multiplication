#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
int main(int argCount, char* args[])
{
    int p_count = atoi(args[2]);

    pid_t child_pid, wpid;
    int status = 0;
    char p_num[100];
    char *args_exec[] = {args[1], p_num, args[2], NULL};

    for (int i=0;i<p_count;i++)
    {
        sprintf(p_num, "%d", i);
        if(fork()==0)
        {
            execv(args[1], args_exec);
            exit(0);
        }
    }
    while((wpid = wait(&status)) > 0);
    return 0;
}