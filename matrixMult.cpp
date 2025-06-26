#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

using namespace std;

int my_id, p_count;

void workSplit(int *start_stop, int rows)
{
    int remainder = rows % p_count;
    int rows_to_stop = rows/p_count;
    int row_to_start = rows_to_stop*my_id;
    
    if (remainder != 0)
    {
        if (my_id < remainder)
        {
            rows_to_stop += 1;
            row_to_start += my_id;
        }
        else{row_to_start += remainder;}
    }
    start_stop[0] = row_to_start;
    start_stop[1] = rows_to_stop;
}

void printMatrix(int *currPointer, int rows, int cols)
{
    for(int r=0;r<rows;r++)
    {
        for(int c=0;c<cols;c++)
        {
            cout << *currPointer << ' ';
            currPointer++;
        }
        cout << '\n';
    }
    cout << '\n';
}

void synch(int par_id, int par_count, int*ready)
{
    int synchid = ready[par_count]+1;
    ready[par_id]=synchid;
    int breakout = 0;
    while(1)
    {
        breakout=1;
        for(int i=0;i<par_count;i++)
        {
            if(ready[i]<synchid)
            {
                breakout=0;
                break;
            }
        }
        if(breakout==1)
        {
            ready[par_count]=synchid;
            break;
        }
    }
}

int main(int argCount, char* args[])
{
    sscanf(args[1], "%d", &my_id);
    sscanf(args[2], "%d", &p_count);

    int fd_M, fd_A, fd_B, fd_ready;
    int *A, *B, *M, *ready;

    int rows_in_A = 100;
    int cols_in_A = 100;

    int rows_in_B = 100;
    int cols_in_B = 100;

    int *curr;
    int start_stop[2];

    timespec stopTime;
    timespec startTime;
    srand(time(0));
    if (my_id == 0)
    {
        fd_M = shm_open("CSC357-Assignment3-AtkinRong-M", O_RDWR | O_CREAT, 0777);
        fd_A = shm_open("CSC357-Assignment3-AtkinRong-A", O_RDWR | O_CREAT, 0777);
        fd_B = shm_open("CSC357-Assignment3-AtkinRong-B", O_RDWR | O_CREAT, 0777);
        fd_ready = shm_open("CSC357-Assignment3-AtkinRong-ready", O_RDWR | O_CREAT, 0777);


        ftruncate(fd_M, 100000);
        ftruncate(fd_A, 100000);
        ftruncate(fd_B, 100000);
        ftruncate(fd_ready, (p_count+1)*4);

        A = (int *) mmap(0, 100000, PROT_WRITE, MAP_SHARED, fd_A, 0);
        B = (int *) mmap(0, 100000, PROT_WRITE, MAP_SHARED, fd_B, 0);
        M = (int *) mmap(0, 100000, PROT_WRITE, MAP_SHARED, fd_M, 0);
        ready = (int *) mmap(0, (p_count+1)*4, PROT_WRITE, MAP_SHARED, fd_ready, 0);
        memset(ready, 0, (p_count+1)*4);

        curr = (int *) A;
        for(int i=0;i<rows_in_A*cols_in_A; i++)
        {
            *curr = std::rand()%10;
            curr++;
        }

        curr = (int *) B;
        for(int i=0;i<rows_in_B*cols_in_B; i++)
        {
            *curr = std::rand()%10;
            curr++;
        }

        // printMatrix(A, rows_in_A, cols_in_A);
        // printMatrix(B, rows_in_B, cols_in_B);

    }
    else
    {
        sleep(2);
        fd_M = shm_open("CSC357-Assignment3-AtkinRong-M", O_RDWR, 0777);
        fd_A = shm_open("CSC357-Assignment3-AtkinRong-A", O_RDWR, 0777);
        fd_B = shm_open("CSC357-Assignment3-AtkinRong-B", O_RDWR, 0777);
        fd_ready = shm_open("CSC357-Assignment3-AtkinRong-ready", O_RDWR, 0777);

        A = (int *) mmap(0, 100000, PROT_WRITE, MAP_SHARED, fd_A, 0);
        B = (int *) mmap(0, 100000, PROT_WRITE, MAP_SHARED, fd_B, 0);
        M = (int *) mmap(0, 100000, PROT_WRITE, MAP_SHARED, fd_M, 0);
        ready = (int *) mmap(0, (p_count+1)*4, PROT_WRITE, MAP_SHARED, fd_ready, 0);
    }

    synch(my_id, p_count, ready);

    if(my_id==0){clock_gettime(CLOCK_MONOTONIC, &startTime);}


    workSplit(start_stop, rows_in_A);
    curr = M + start_stop[0] * cols_in_B;
    for(int r_A=start_stop[0];r_A<start_stop[0]+start_stop[1];r_A++)
    {
        int *row_A = A + (r_A * cols_in_A);
        for(int c_B=0;c_B<cols_in_B;c_B++)
        {
            int product = 0;
            for(int r_B=0; r_B<rows_in_B; r_B++)
            {
                int *curr_B = B + c_B + (cols_in_B * r_B);
                product += *curr_B * *(row_A+r_B);
            }
            *curr = product;
            curr++;
        }
    }
    int rows_in_M = rows_in_A;
    int cols_in_M = cols_in_B;
    synch(my_id, p_count, ready);
    //if(my_id==0){cout << "synch" << endl;}


    workSplit(start_stop, rows_in_M);
    curr = B + start_stop[0] * cols_in_A;
    for(int r_A=start_stop[0];r_A<start_stop[0]+start_stop[1];r_A++)
    {
        int *row_A = M + (r_A * cols_in_M);
        for(int c_B=0;c_B<cols_in_A;c_B++)
        {
            int product = 0;
            for(int r_B=0; r_B<rows_in_A; r_B++)
            {
                int *curr_B = A + c_B + (cols_in_A * r_B);
                product += *curr_B * *(row_A+r_B);
            }
            *curr = product;
            curr++;
        }
    }
    rows_in_B = rows_in_M;
    cols_in_B = cols_in_A;
    synch(my_id, p_count, ready);
    //if(my_id==0){cout << "synch" << endl;}

    workSplit(start_stop, rows_in_B);

    curr = M + start_stop[0] * cols_in_A;
    for(int r_A=start_stop[0];r_A<start_stop[0]+start_stop[1];r_A++)
    {
        int *row_A = B + (r_A * cols_in_B);
        for(int c_B=0;c_B<cols_in_A;c_B++)
        {
            int product = 0;
            for(int r_B=0; r_B<rows_in_A; r_B++)
            {
                int *curr_B = A + c_B + (cols_in_A * r_B);
                product += *curr_B * *(row_A+r_B);
            }
            *curr = product;
            curr++;
        }
    }
    rows_in_M = rows_in_A;
    cols_in_M = cols_in_B;
    synch(my_id, p_count, ready);
    //if(my_id==0){cout << "synch" << endl;}

    if(my_id==0)
    {
        clock_gettime(CLOCK_MONOTONIC, &stopTime);

        long sum = 0;
        for(int i=0;i<100;i++)
        {
            sum += M[i+i*100];
        }
        cout << sum << endl;

        
        long nseconds = stopTime.tv_nsec - startTime.tv_nsec;
        long seconds = stopTime.tv_sec - startTime.tv_sec;
        if (nseconds < 0) {
            nseconds += 1000000000;
        }
        printf("%lf\n", (double)nseconds/1000000000.0 + seconds);
    }


    munmap(A, 100000);
    munmap(B, 100000);
    munmap(M, 100000);
    munmap(ready, 4*(p_count+1));
    close(fd_A);
    close(fd_B);
    close(fd_M);
    close(fd_ready);
    shm_unlink("CSC357-Assignment3-AtkinRong-M");
    shm_unlink("CSC357-Assignment3-AtkinRong-A");
    shm_unlink("CSC357-Assignment3-AtkinRong-B");
    shm_unlink("CSC357-Assignment3-AtkinRong-ready");
    return 0;
}
