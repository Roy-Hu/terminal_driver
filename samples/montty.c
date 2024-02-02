#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <hardware.h>   /* Defines the interface to the hardware */
#include <terminals.h>  /* Definitions of the the user level routines */
#include <threads.h>	/* COMP 421 threads package definitions */

#define BUF_SIZE 64  

static int num_readers, num_writers, num_echo, r_reg_num, w_reg_num;
static cond_id_t readers, writers, echo, r_reg, w_reg;

static char *echo_buf[NUM_TERMINALS];
static int end_echo_buf[NUM_TERMINALS];

extern void 
ReceiveInterrupt(int term) 
{
    Declare_Monitor_Entry_Procedure();
    // Lock for the reading reg
    while (r_reg_num > 0) CondWait(r_reg);

    r_reg_num++;

    char c = ReadDataRegister(term);
    
    r_reg_num--;

    while (num_echo > 0) CondWait(echo);

    num_echo++;

    echo_buf[term][end_echo_buf[term]++] = c;
    
    end_echo_buf[term]--;

    for (int i = 0; i < end_echo_buf[term]; i++) {
        echo_buf[term][i] = echo_buf[term][i + 1];
    }
    
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
    

    num_echo--;
    CondSignal(echo);
}

extern int 
WriteTerminal(int term, char *buf, int buflen)
{
    Declare_Monitor_Entry_Procedure();

    while (num_writers > 0) CondWait(writers);
    
    num_writers++;

    int out_char = 0;
    for (int i = 0; i < buflen && i < BUF_SIZE; i++) {

        printf("%c", buf[i]);

        out_char++;
    }

    num_writers--;

    CondSignal(writers);

    return out_char;
}

extern int 
InitTerminal(int term)
{
    Declare_Monitor_Entry_Procedure();

    InitHardware(term);

    echo_buf[term] = (char *)malloc(BUF_SIZE * sizeof(char));
    end_echo_buf[term] = 0;

    return 0;
}

extern int 
InitTerminalDriver(void)
{
    Declare_Monitor_Entry_Procedure();

    // create condition variables
    readers = CondCreate();
    writers = CondCreate();
    echo = CondCreate();
    r_reg = CondCreate();
    w_reg = CondCreate();

    num_readers = 0;
    num_writers = 0;
    num_echo = 0;
    r_reg_num = 0;
    w_reg_num = 0;

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