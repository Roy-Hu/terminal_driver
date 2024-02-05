#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <hardware.h>   /* Defines the interface to the hardware */
#include <terminals.h>  /* Definitions of the the user level routines */
#include <threads.h>	/* COMP 421 threads package definitions */

#define BUF_SIZE 512  

static int num_readers[NUM_TERMINALS], num_writers[NUM_TERMINALS];

// lock for the driver
static cond_id_t reader[NUM_TERMINALS], writer[NUM_TERMINALS], read[NUM_TERMINALS];

// lock for the hardware
static cond_id_t echo, r_reg, w_reg;

static int num_echo, num_echo_wait, r_reg_num, w_reg_num;

static char *echo_buf[NUM_TERMINALS];
static int end_echo_buf[NUM_TERMINALS];

static char *write_buf[NUM_TERMINALS];
static int end_write_buf[NUM_TERMINALS];

static char *read_buf[NUM_TERMINALS];
static int end_read_buf[NUM_TERMINALS];

static int tmp = 0;

extern void 
ReceiveInterrupt(int term) 
{
    Declare_Monitor_Entry_Procedure();

    // Lock for the read reg
    while (r_reg_num > 0) CondWait(r_reg);

    r_reg_num++;

    char c = ReadDataRegister(term);
    
    r_reg_num--;

    CondSignal(r_reg);

    if (c == '\r') c = '\n';

    if (end_read_buf[term] < BUF_SIZE) {
        read_buf[term][end_read_buf[term]++] = c;
        // Need to wake up the reader if there are originaly have no char in the read buf
        if (c == '\n') CondSignal(read[term]);
    } else {
        printf("[WARNING][INTERRUPT][TERM %d] Read Buf Full\n", term);
        return;
    }

    int echo_char_num = 0;
    if (end_echo_buf[term] < BUF_SIZE) {
        if (c == '\n') {
            if (end_echo_buf[term] >= BUF_SIZE - 1) {
                printf("[WARNING][INTERRUPT][TERM %d] Echo Buf is Full, do not have space for \\r\\n\n", term);
            } else {
                echo_buf[term][end_echo_buf[term]++] = '\r';
                echo_char_num++;
            }
        } 

        echo_buf[term][end_echo_buf[term]++] = c;
        echo_char_num++;

        // echo_buf[term][end_echo_buf[term]++] = '\r';
        // echo_char_num++;

        // echo_buf[term][end_echo_buf[term]++] = '\n';
        // echo_char_num++;

        // echo_buf[term][end_echo_buf[term]++] = ((tmp++) % 10) + '0';
        // echo_char_num++;
    } else {
        printf("[WARNING][INTERRUPT][TERM %d] Echo Buf Full\n", term);
    }


    for (int i = 0; i < echo_char_num; i++) {
        bool wait = false;

        while (num_echo > 0 || w_reg_num > 0) {
            num_echo_wait++;
            wait = true;
            CondWait(echo);
        }

        num_echo++;

        if (wait) {
            num_echo_wait--;
        } 

        printf("[DEBUG][INTERRUPT][TERM %d] Echo buf len %d\n", term, end_echo_buf[term]);

        WriteDataRegister(term, echo_buf[term][0]);
    }
}

extern int 
ReadTerminal(int term, char *buf, int buflen)
{
    Declare_Monitor_Entry_Procedure();

    while (num_readers[term] > 0) CondWait(reader[term]);

    num_readers[term]++;

    bool newline = false;

    for (int i = 0; i < end_read_buf[term]; i++) {
        if (read_buf[term][i] == '\n') {
            newline = true;
            break;
        }
    }
    
    // Wait while there are no newline in the read buf
    if (!newline) CondWait(read[term]);

    int cnt;
    for (cnt = 0; cnt < buflen; cnt++) {
        buf[cnt] = read_buf[term][0];

        // Shift the read buf since we have read a char
        for (int i = 0; i < end_read_buf[term] - 1; i++) {
            read_buf[term][i] = read_buf[term][i + 1];
        }
        
        end_read_buf[term]--;

        if (buf[cnt] == '\n') {
            cnt++;
            break;
        } 
    }

    
    num_readers[term]--;

    CondSignal(reader[term]);
    
    return cnt;
}

extern void 
TransmitInterrupt(int term) 
{
    Declare_Monitor_Entry_Procedure();
    
    if (num_echo > 1 || w_reg_num > 1 || (num_echo > 0 && w_reg_num > 0)) {
        fprintf(stderr, "[ERROR]: Multiple write at the same time, Echo: %d, W_REG: %d",num_echo, w_reg_num);
        exit(0);
    } else if (num_echo > 0) {
        for (int i = 0; i < end_echo_buf[term] - 1; i++) {
            echo_buf[term][i] = echo_buf[term][i + 1];
        }

        end_echo_buf[term]--;

        num_echo--;
    } else if (w_reg_num > 0){
        for (int i = 0; i < end_write_buf[term] - 1; i++) {
            write_buf[term][i] = write_buf[term][i + 1];
        }
        
        end_write_buf[term]--;

        w_reg_num--;
    }

    // Make sure echo have the higher piorirty
    if (num_echo_wait > 0) {
        CondSignal(echo);
    } else {
        // If there are no echo waiting, wake up the writer
        CondSignal(w_reg);
    }
}

extern int 
WriteTerminal(int term, char *buf, int buflen)
{
    Declare_Monitor_Entry_Procedure();

    while (num_writers[term] > 0) CondWait(writer[term]);
    
    num_writers[term]++;

    int write_char_num = 0;
    for (int i = 0; i < buflen && end_write_buf[term] < BUF_SIZE; i++) {
        if (buf[i] == '\n') {
            write_buf[term][end_write_buf[term]++] = '\r';

            // should not count the extra \r
            write_char_num--;
            i--;
        }

        write_buf[term][end_write_buf[term]++] = buf[i];

        while (num_echo > 0 || w_reg_num > 0) {
            CondWait(w_reg);
        } 

        w_reg_num++;
        
        printf("[DEBUG][INTERRUPT][TERM %d] Write buf len %d\n", term, end_read_buf[term]);

        WriteDataRegister(term, write_buf[term][0]);

        write_char_num++;

        printf("[DEBUG][TERM %d] Write Char Num %d / Buf len %d\n", term, write_char_num, buflen);
    }

    num_writers[term]--;

    CondSignal(writer[term]);

    return write_char_num;
}

extern int 
InitTerminal(int term)
{
    Declare_Monitor_Entry_Procedure();

    InitHardware(term);

    echo_buf[term] = (char *)malloc(BUF_SIZE * sizeof(char));
    end_echo_buf[term] = 0;

    write_buf[term] = (char *)malloc(BUF_SIZE * sizeof(char));
    end_write_buf[term] = 0;
    
    read_buf[term] = (char *)malloc(BUF_SIZE * sizeof(char));
    end_read_buf[term] = 0;

    num_readers[term] = 0;
    num_writers[term] = 0;

    reader[term] = CondCreate();
    writer[term] = CondCreate();
    read[term] = CondCreate();

    return 0;
}

extern int 
InitTerminalDriver(void)
{
    Declare_Monitor_Entry_Procedure();

    echo = CondCreate();
    r_reg = CondCreate();
    w_reg = CondCreate();

    num_echo = 0;
    r_reg_num = 0;
    w_reg_num = 0;

    num_echo_wait = 0;
    
    return 0;
}

extern int 
TerminalDriverStatistics(struct termstat *stats)
{
    Declare_Monitor_Entry_Procedure();
    stats->tty_in = 0;
    stats->tty_out = 0;
    stats->user_in = 0;
    stats->user_out = 0;

    printf("stats\n");
    printf("\t tty in %d\n", stats->tty_in);
    printf("\t tty out in %d\n", stats->tty_out);
    printf("\t user in %d\n", stats->user_in);
    printf("\t user out %d\n", stats->user_out);
    
    return 0;
}