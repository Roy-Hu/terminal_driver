#include <stdlib.h>
#include <unistd.h>
#include <threads.h>

#include "utils.h"

#define BUF_SIZE 20

void reader_writer1();

char string[] = "To simplify the project, we will use a software terminal emulation rather than working directly with real terminal hardware. This emulation is designed to give you the flavor of developing a real device driver, without as much complexity and frustration. We will also use a simplified threads library instead of the standard POSIX \"Pthreads\" library. We hope that by avoiding the overly large and complex Pthreads API, you will better be able to focus on the project itself.\n This project must be done individually. The second and third projects in this class will be done in groups of 2 students, but you must do this first project by yourself.\n";

int length = sizeof(string) - 1;

// Tests single write.
int
main()
{
    InitTerminalDriver();
    InitTerminal(1);

    ThreadCreate(reader_writer1, NULL);

    ThreadWaitAll();

    struct termstat *stats = malloc(sizeof(struct termstat) * 4);
    
    TerminalDriverStatistics(stats);

    print_stats(stats);

    free(stats);

    exit(0);
}

void
reader_writer1()
{
    char* buf = malloc(sizeof(char) * BUF_SIZE);

    for (int i = 0; i < 10; i++) {
        // Leave the last for null char...
        int num_char_read = ReadTerminal(1, buf, BUF_SIZE - 1);

        printf("Num of characters read : %d\n", num_char_read);
        printf("String read : ");

        append_null(buf, BUF_SIZE - 1);
        print_read_buf(buf, BUF_SIZE - 1);
    }
    free(buf);

    int status;

    status = WriteTerminal(1, string, length);
    if (status != length)
	fprintf(stderr, "Error: writer1 status = %d, length1 = %d\n",
	    status, length);
}