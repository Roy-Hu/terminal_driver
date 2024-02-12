#include <stdlib.h>
#include <unistd.h>
#include <threads.h>

#include "utils.h"

#define BUF_SIZE 20

void writer1();
void writer2();
void writer3();
void writer4();

char string1[] = "To simplify the project, we will use a software terminal emulation rather than working directly with real terminal hardware. This emulation is designed to give you the flavor of developing a real device driver, without as much complexity and frustration. We will also use a simplified threads library instead of the standard POSIX \"Pthreads\" library. We hope that by avoiding the overly large and complex Pthreads API, you will better be able to focus on the project itself.\n This project must be done individually. The second and third projects in this class will be done in groups of 2 students, but you must do this first project by yourself.\n";
char string2[] = "Through this assignment, you will be able to gain practical experience in programming concurrent systems using threads with a monitor for synchronization. You will also gain experience with the design and operation of device drivers in operating systems. In particular, in this project you will implement a terminal device driver using a Mesa-style monitor for synchronization of interrupts and concurrent driver requests from different user threads that share an address space.\n";
char string3[] = "For the terminal hardware simulation to work correctly, you must be running a local X11 Windows server program on your own local computer, and must log in to CLEAR using ssh in a way that allows X Window operations to be forwarded back to your local X Windows server program through this ssh connection. This use of X Windows and ssh is explained more fully in Section 9.\n";
char string4[] = "Your terminal device driver must guarantee that no deadlock is possible and must generally also ensure that no starvation is possible as well. The one case in this project where starvation may be allowed is due simply to the use of Mesa monitor semantics rather than Hoare semantics. Specifically, before most condition variable waits, a Hoare monitor only requires an if whereas a Mesa monitor generally requires a while. This use of a while is generally required for correctness but may itself be the source of possible starvation, if every time the process is awoken from a signal, the process ends of going around the loop and waiting again on the same condition variable.\n";

int length1 = sizeof(string1) - 1;
int length2 = sizeof(string2) - 1;
int length3 = sizeof(string3) - 1;
int length4 = sizeof(string4) - 1;

// Tests single read & write.
int
main()
{
    InitTerminalDriver();
    InitTerminal(1);
    InitTerminal(2);

    ThreadCreate(writer1, NULL);
    ThreadCreate(writer2, NULL);
    ThreadCreate(writer3, NULL);
    ThreadCreate(writer4, NULL);
    ThreadWaitAll();

    struct termstat *stats = malloc(sizeof(struct termstat) * 4);

    sleep(5);
        
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

    status = WriteTerminal(2, string2, length2);
    if (status != length2)
	fprintf(stderr, "[2]Error: writer2 status = %d, length2 = %d\n",
	    status, length2);
}


void
writer3()
{
    int status;

    status = WriteTerminal(1, string3, length3);
    if (status != length3)
	fprintf(stderr, "[3]Error: writer3 status = %d, length3 = %d\n",
	    status, length3);
}

void
writer4()
{
    int status;

    status = WriteTerminal(2, string4, length4);
    if (status != length4)
	fprintf(stderr, "[4]Error: writer4 status = %d, length4 = %d\n",
	    status, length4);
}
