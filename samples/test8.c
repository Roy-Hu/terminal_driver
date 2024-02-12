#include <stdlib.h>
#include <unistd.h>
#include <threads.h>

#include "utils.h"

#define BUF_SIZE 10

void reader1();
void reader2();

// Tests multi read.
int
main()
{
    InitTerminalDriver();
    InitTerminal(1);

    ThreadCreate(reader1, NULL);
    ThreadCreate(reader2, NULL);

    ThreadWaitAll();

    struct termstat *stats = malloc(sizeof(struct termstat) * 4);
    
    // sleep(1);

    TerminalDriverStatistics(stats);

    print_stats(stats);

    free(stats);

    exit(0);
}

void
reader1()
{
    char* buf = malloc(sizeof(char) * BUF_SIZE);

    for (int i = 0; i < 10; i++) {
        // Leave the last for null char...
        int num_char_read = ReadTerminal(1, buf, BUF_SIZE - 1);

        printf("[1]Num of characters read : %d\n", num_char_read);
        printf("[1]String read : ");

        append_null(buf, BUF_SIZE - 1);
        print_read_buf(buf, BUF_SIZE - 1);
    }
    free(buf);
}

void
reader2()
{
    char* buf = malloc(sizeof(char) * BUF_SIZE);

    for (int i = 0; i < 5; i++) {
        // Leave the last for null char...
        int num_char_read = ReadTerminal(1, buf, BUF_SIZE - 1);

        printf("[2]Num of characters read : %d\n", num_char_read);
        printf("[2]String read : ");

        append_null(buf, BUF_SIZE - 1);
        print_read_buf(buf, BUF_SIZE - 1);
    }
    free(buf);
}