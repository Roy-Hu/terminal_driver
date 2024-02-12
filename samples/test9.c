#include <stdlib.h>
#include <unistd.h>
#include <threads.h>

#include "utils.h"

#define BUF_SIZE 20

void writer1();
void writer2();

char string1[] = "To simplify the project, we will use a software terminal emulation rather than working directly with real terminal hardware. This emulation is designed to give you the flavor of developing a real device driver, without as much complexity and frustration. We will also use a simplified threads library instead of the standard POSIX \"Pthreads\" library. We hope that by avoiding the overly large and complex Pthreads API, you will better be able to focus on the project itself.\n This project must be done individually. The second and third projects in this class will be done in groups of 2 students, but you must do this first project by yourself.\n";
char string2[] = "Through this assignment, you will be able to gain practical experience in programming concurrent systems using threads with a monitor for synchronization. You will also gain experience with the design and operation of device drivers in operating systems. In particular, in this project you will implement a terminal device driver using a Mesa-style monitor for synchronization of interrupts and concurrent driver requests from different user threads that share an address space.";

int length1 = sizeof(string1) - 1;
int length2 = sizeof(string2) - 1;

// Tests single read & write.
int
main()
{
    InitTerminalDriver();
    InitTerminal(1);

    ThreadCreate(writer1, NULL);
    ThreadCreate(writer2, NULL);

    ThreadWaitAll();

    struct termstat *stats = malloc(sizeof(struct termstat) * 4);

    sleep(1);
        
    TerminalDriverStatistics(stats);

    print_stats(stats);

    free(stats);

    exit(0);
}

void
writer1()
{
    int status;

    status = WriteTerminal(1, string1, length1);
    if (status != length1)
	fprintf(stderr, "[1]Error: writer1 status = %d, length1 = %d\n",
	    status, length1);
}

void
writer2()
{
    int status;

    status = WriteTerminal(1, string2, length2);
    if (status != length2)
	fprintf(stderr, "[2]Error: writer2 status = %d, length2 = %d\n",
	    status, length2);
}