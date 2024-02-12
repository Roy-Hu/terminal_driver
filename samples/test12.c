#include <stdlib.h>
#include <unistd.h>
#include <threads.h>

#include "utils.h"

#define BUFFER_SIZE 20

void reader();
void writer();

void reader_wrong_term();
void writer_wrong_term();

char string[] = "Hello world\n";
int length = sizeof(string) - 1;

int
main()
{    
    // Test 0
    if (InitTerminalDriver() == -1) {
        printf("FAIL Test 0\n");
        return -1;
    }

    // Test 1
    ThreadCreate(reader, NULL); 
    ThreadCreate(writer, NULL); 

    // Test 2
    if (InitTerminalDriver() == 0) {
        printf("FAIL Test 2\n");
        return -1;
    }

    // Test 3
    if (InitTerminal(1) == -1) {
        printf("FAIL Test 3\n");
        return -1;
    }

    // Test 4
    if (InitTerminal(1) == 0) {
        printf("FAIL Test 4\n");
        return -1;
    }

    // Test 5
    ThreadCreate(reader_wrong_term, NULL);

    // Test 6
    ThreadCreate(writer_wrong_term, NULL); 

    ThreadWaitAll();

    sleep(1);

    struct termstat *stats = NULL;
    
    // Test 7
    if (TerminalDriverStatistics(stats) == 0) {
        printf("FAIL Test 7\n");
        print_stats(stats);
        return -1;
    }

    stats = malloc(sizeof(struct termstat) * NUM_TERMINALS);
    // Test 8
    if (TerminalDriverStatistics(stats) == -1) {
        printf("FAIL Test 8\n");
        print_stats(stats);
        return -1;
    }

    free(stats);

    exit(0);
}

void
reader()
{
    char* buf = malloc(sizeof(char) * BUFFER_SIZE);

    int num_char_read = ReadTerminal(1, buf, BUFFER_SIZE - 1);
    printf("[2]Num of characters read : %d\n", num_char_read);
    
    free(buf);
}

void
writer()
{ 
    int num_char_write = WriteTerminal(1, string, length);
    printf("[2]Num of characters write : %d\n", num_char_write);
    fflush(stdout);
}

void
reader_wrong_term()
{
    char* buf = NULL;

    int num_char_read = ReadTerminal(1, buf, BUFFER_SIZE - 1);
    printf("[5-1]Num of characters read : %d\n", num_char_read);

    buf = malloc(sizeof(char) * BUFFER_SIZE);
    
    num_char_read = ReadTerminal(NUM_TERMINALS + 1, buf, BUFFER_SIZE - 1);
    printf("[5-3]Num of characters read : %d\n", num_char_read);

    num_char_read = ReadTerminal(-1, buf, BUFFER_SIZE - 1);
    printf("[5-4]Num of characters read : %d\n", num_char_read);

    free(buf);
}

void
writer_wrong_term()
{
    char* buf = NULL;

    int num_char_write = WriteTerminal(1, buf, length);
    printf("[6-1]Num of characters write : %d\n", num_char_write);

    num_char_write = WriteTerminal(NUM_TERMINALS + 1, string, length);
    printf("[6-3]Num of characters write : %d\n", num_char_write);

    num_char_write = WriteTerminal(-1, string, length);
    printf("[6-4]Num of characters write : %d\n", num_char_write);
}