#include <stdlib.h>
#include <unistd.h>
#include <threads.h>

#include "utils.h"

#define BUF_SIZE 20

void reader1();
void writer1();

char string[] = "To simplify the project, we will use a software terminal emulation rather than working directly with real terminal hardware. This emulation is designed to give you the flavor of developing a real device driver, without as much complexity and frustration. We will also use a simplified threads library instead of the standard POSIX \"Pthreads\" library. We hope that by avoiding the overly large and complex Pthreads API, you will better be able to focus on the project itself.\n This project must be done individually. The second and third projects in this class will be done in groups of 2 students, but you must do this first project by yourself.\n";

int length = sizeof(string) - 1;

// Tests single read & write.
int
main()
{
    InitTerminalDriver();
    InitTerminal(1);

    ThreadCreate(writer1, NULL);

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

    status = WriteTerminal(1, string, length);
    if (status != length)
	fprintf(stderr, "Error: writer1 status = %d, length1 = %d\n",
	    status, length);
}