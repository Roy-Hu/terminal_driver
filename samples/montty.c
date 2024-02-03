#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <hardware.h>   /* Defines the interface to the hardware */
#include <terminals.h>  /* Definitions of the the user level routines */
#include <threads.h>	/* COMP 421 threads package definitions */

#define BUF_SIZE 1024  

#define IDLE 0
#define WAIT 1
#define BUSY 2

static int num_readers[NUM_TERMINALS], num_writers[NUM_TERMINALS];

// lock for the driver
static cond_id_t reader[NUM_TERMINALS], writer[NUM_TERMINALS];

// lock for the hardware
static cond_id_t echo, r_reg, w_reg;

static int num_echo, r_reg_num, w_reg_num;

static int echo_state;

static char *echo_buf[NUM_TERMINALS];
static int end_echo_buf[NUM_TERMINALS];

static char *write_buf[NUM_TERMINALS];
static int end_write_buf[NUM_TERMINALS];


extern void 
ReceiveInterrupt(int term) 
{
    Declare_Monitor_Entry_Procedure();
    // Lock for the reading reg
    while (r_reg_num > 0) CondWait(r_reg);

    r_reg_num++;

    char c = ReadDataRegister(term);
    
    r_reg_num--;

    while (num_echo > 0 || w_reg_num > 0) {
        echo_state = WAIT;
        CondWait(echo);
    }

    echo_state = BUSY;
    num_echo++;

    echo_buf[term][end_echo_buf[term]++] = c;
    
    for (int i = 0; i < end_echo_buf[term] - 1; i++) {
        echo_buf[term][i] = echo_buf[term][i + 1];
    }
    
    end_echo_buf[term]--;

    WriteDataRegister(term, echo_buf[term][0]);
}

extern int 
ReadTerminal(int term, char *buf, int buflen)
{
    Declare_Monitor_Entry_Procedure();

    return 0;
}

extern void 
TransmitInterrupt(int term) 
{
    Declare_Monitor_Entry_Procedure();
    
    if (num_echo > 0) {
        num_echo--;
    } else if (w_reg_num > 0){
        w_reg_num--;
    }

    // Make sure echo have the higher piorirty
    if (echo_state == WAIT) {
        CondSignal(echo);
    } else {
        // If there are no echo waiting, set it's state to idle
        echo_state = IDLE;

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
    for (int i = 0; i < buflen && i < BUF_SIZE; i++) {
        while (num_echo > 0 || w_reg_num > 0) {
            CondWait(w_reg);
        } 

        w_reg_num++;
        
        write_buf[term][end_write_buf[term]++] = buf[i];
        
        for (int i = 0; i < end_write_buf[term] - 1; i++) {
            write_buf[term][i] = write_buf[term][i + 1];
        }
        
        end_write_buf[term]--;

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
    
    num_readers[term] = 0;
    num_writers[term] = 0;

    reader[term] = CondCreate();
    writer[term] = CondCreate();

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

    echo_state = IDLE;
    
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