#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <hardware.h>   /* Defines the interface to the hardware */
#include <terminals.h>  /* Definitions of the the user level routines */
#include <threads.h>	/* COMP 421 threads package definitions */

#define BUF_SIZE 512  
#define BYPASS_LIMIT 1

static bool isInit[NUM_TERMINALS];
static bool isInitMonitor = false;

static int num_readers[NUM_TERMINALS], num_writers[NUM_TERMINALS];

// lock for the driver
static cond_id_t reader[NUM_TERMINALS], writer[NUM_TERMINALS], read[NUM_TERMINALS], last_write[NUM_TERMINALS];

// lock for the hardware
static cond_id_t write;

int num_last_write[NUM_TERMINALS];

static int num_echo, num_write, num_stas;

static int waiting_write[NUM_TERMINALS];
static int bypass_echo[NUM_TERMINALS], bypass_write[NUM_TERMINALS];

static int tty_in[NUM_TERMINALS], tty_out[NUM_TERMINALS], user_in[NUM_TERMINALS], user_out[NUM_TERMINALS];
static int tty_in_tmp[NUM_TERMINALS], tty_out_tmp[NUM_TERMINALS], user_in_tmp[NUM_TERMINALS], user_out_tmp[NUM_TERMINALS];

static char *read_buf[NUM_TERMINALS];
static int read_in[NUM_TERMINALS], read_out[NUM_TERMINALS], read_count[NUM_TERMINALS];
static int newline_num[NUM_TERMINALS];

static char *echo_buf[NUM_TERMINALS];
static int echo_in[NUM_TERMINALS], echo_out[NUM_TERMINALS], echo_count[NUM_TERMINALS];


static void
addItem(char *buf, int *in, int *cnt, char c)
{
    buf[*in] = c;

    *cnt = *cnt + 1;

    *in = (*in + 1) % BUF_SIZE;
}

static char
removeItem(char *buf, int *out, int *cnt)
{   
    // If the buffer is empty, return null
    if (*cnt == 0) {
        return '\0';
    }

    char c = buf[*out];
    
    *cnt = *cnt - 1;

    *out = (*out + 1) % BUF_SIZE;

    return c;
}

static void
SafeWriteReg(int term, char c)
{
    bool bypass_limit = false;
    for (int i = 0; i < NUM_TERMINALS; i++) {
        if ((bypass_write[i] > BYPASS_LIMIT || bypass_echo[i] > BYPASS_LIMIT) && i != term) {
            bypass_limit = true;
            break;
        }
    }

    while (num_echo > 0 || num_write > 0 || bypass_limit) {
        waiting_write[term]++;
        CondWait(write);
        waiting_write[term]--;
    }

    bypass_write[term] = 0;

    for (int i = 0; i < NUM_TERMINALS; i++) {
        if (waiting_write[i] > 0 && i != term) bypass_write[i]++;
        if (echo_count[i] > 0 && i != term) bypass_echo[i]++;
    }

    num_write++;
    
    WriteDataRegister(term, c);
}

// Since the finish time of ReceiveInterrupt is fast enought, everyone can access to ReadDataRegister without starvation
extern void 
ReceiveInterrupt(int term) 
{
    Declare_Monitor_Entry_Procedure();

    char c = ReadDataRegister(term);
    
    if (num_stas > 0) {
        tty_in_tmp[term]++;
    } else {
        tty_in[term]++;
    }

    if (c == '\r') c = '\n';
    if (c == '\177') c = '\b';

    if (c == '\b') {
        read_in[term]--;
        read_count[term]--;
    } else if (read_count[term] < BUF_SIZE) {
        addItem(read_buf[term], &read_in[term], &read_count[term], c);

        if (c == '\n') {
            newline_num[term]++;
            CondSignal(read[term]);
        }
    } else {
        printf("[WARNING][INTERRUPT][TERM %d] Read Buf Full\n", term);
        return;
    }

    if (c == '\b') {
        addItem(echo_buf[term], &echo_in[term], &echo_count[term], '\b');
        addItem(echo_buf[term], &echo_in[term], &echo_count[term], ' ');
        addItem(echo_buf[term], &echo_in[term], &echo_count[term], '\b');
    } else if (c == '\n') {
        addItem(echo_buf[term], &echo_in[term], &echo_count[term], '\r');
        addItem(echo_buf[term], &echo_in[term], &echo_count[term], '\n');
    } else {
        addItem(echo_buf[term], &echo_in[term], &echo_count[term], c);
    }

    // Directly write to the terminal if no need to wait
    if (num_echo == 0 && num_write == 0) {
        num_echo++;

        WriteDataRegister(term, removeItem(echo_buf[term], &echo_out[term], &echo_count[term]));
    }
}

extern int 
ReadTerminal(int term, char *buf, int buflen)
{
    Declare_Monitor_Entry_Procedure();
    
    if (term < 0 || term >= NUM_TERMINALS) {
        printf("[ERROR]: Invalid terminal number %d\n", term);
        return -1;
    }

    if (!isInitMonitor || !isInit[term]) {
        printf("[ERROR]: Terminal %d is not initialized\n", term);
        return -1;
    }

    if (buf == NULL) {
        printf("[ERROR]: Buf is NULL\n");
        return -1;
    }

    if (buflen == 0) return 0;

    while (num_readers[term] > 0) CondWait(reader[term]);

    num_readers[term]++;
    
    int cnt = 0;
    // Read from the read_buf if newline is found or buflen is reached
    while (cnt < buflen) {
        char c = removeItem(read_buf[term], &read_out[term], &read_count[term]);
        if (c == '\0') {
            CondWait(read[term]);
        } else {
            buf[cnt++] = c;
        }

        if (c == '\n') break;
    }
    
    while (newline_num == 0) CondWait(read[term]);

    newline_num[term]--;

    num_readers[term]--;

    CondSignal(reader[term]);
    
    if (num_stas > 0) {
        user_out_tmp[term] += cnt;
    } else {
        user_out[term] += cnt;
    }

    return cnt;
}

extern void 
TransmitInterrupt(int term) 
{
    Declare_Monitor_Entry_Procedure();
    
    if (num_stas > 0) {
        tty_out_tmp[term]++;
    } else {
        tty_out[term]++;
    }


    if (num_echo > 1 || num_write > 1 || (num_echo > 0 && num_write > 0)) {
        fprintf(stderr, "[ERROR]: Multiple write at the same time, Echo: %d, write: %d",num_echo, num_write);
        exit(0);
    } else if (num_echo > 0) {
        num_echo--;
    } else if (num_write > 0){
        num_write--;
    }

    // Make sure echo have the higher piorirty
    bool next_echo = false;
    int next_term = 0;
    
    for (int i = 0; i < NUM_TERMINALS; i++) {
        if (echo_count[i] > 0) {
            next_echo = true;
            next_term = i;
        }
        if (bypass_echo[i] > BYPASS_LIMIT) {
            next_echo = true;
            next_term = i;
            break;
        }
    }

    if (next_echo) {
        for (int i = 0; i < NUM_TERMINALS; i++) {
            if (echo_count[i] > 0 && i != next_term) {
                printf("[DEBUG]Bypass echo %d\n", i);
                bypass_echo[i]++;
            }
        }

        if (bypass_write[next_term] > BYPASS_LIMIT) {
            printf("[DEBUG] Write due to reach Bypass limit %d\n", term);
        }

        bypass_echo[next_term] = 0;

        num_echo++;

        char c = removeItem(echo_buf[next_term], &echo_out[next_term], &echo_count[next_term]);
        WriteDataRegister(next_term, c);
    } else {
        if (num_last_write[term] > 0) {
            num_last_write[term]--;
            CondSignal(last_write[term]);
        }
        // If there are no echo waiting, wake up the writer
        CondSignal(write);
    }
}

extern int 
WriteTerminal(int term, char *buf, int buflen)
{
    Declare_Monitor_Entry_Procedure();

    if (term < 0 || term >= NUM_TERMINALS) {
        printf("[ERROR]: Invalid terminal number %d\n", term);
        return -1;
    }
    
    if (!isInitMonitor || !isInit[term]) {
        printf("[ERROR]: Terminal %d is not initialized\n", term);
        return -1;
    }

    if (buf == NULL) {
        printf("[ERROR]: Buf is NULL\n");
        return -1;
    }

    while (num_writers[term] > 0) CondWait(writer[term]);

    num_writers[term]++;

    int write_char_num = 0;

    for (int i = 0; i < buflen; i++) {      
        if (i == buflen - 1) num_last_write[term]++;

        if (buf[i] == '\n') {
            // Special case where the last character is newline
            if (i == buflen - 1) num_last_write[term]++;

            SafeWriteReg(term, '\r');
            SafeWriteReg(term, '\n');
        } else {
            SafeWriteReg(term, buf[i]);
        }

        write_char_num++;
    }

    // Wait for the last write to finish
    while (num_last_write[term] > 0) {
        CondWait(last_write[term]);
    }

    num_writers[term]--;

    CondSignal(writer[term]);

    if (num_stas > 0) {
        user_out_tmp[term] += write_char_num;
    } else {
        user_out[term] += write_char_num;
    }

    return write_char_num;
}

extern int 
InitTerminal(int term)
{
    Declare_Monitor_Entry_Procedure();

    InitHardware(term);
    
    if (!isInitMonitor) return -1;
    if (isInit[term]) return -1;

    isInit[term] = true;

    read_buf[term] = (char *)malloc(BUF_SIZE * sizeof(char));
    read_count[term] = 0;
    read_in[term] = 0;
    read_out[term] = 0;

    echo_buf[term] = (char *)malloc(BUF_SIZE * sizeof(char));
    echo_count[term] = 0;
    echo_in[term] = 0;
    echo_out[term] = 0;

    num_readers[term] = 0;
    num_writers[term] = 0;
    newline_num[term] = 0;

    reader[term] = CondCreate();
    writer[term] = CondCreate();
    read[term] = CondCreate();
    last_write[term] = CondCreate();
    num_last_write[term] = 0;

    waiting_write[term] = 0;

    tty_in[term] = 0;
    tty_out[term] = 0;
    user_in[term] = 0;
    user_out[term] = 0;

    tty_in_tmp[term] = 0;
    tty_out_tmp[term] = 0;
    user_in_tmp[term] = 0;
    user_out_tmp[term] = 0;
    
    return 0;
}

extern int 
InitTerminalDriver(void)
{
    Declare_Monitor_Entry_Procedure();

    write = CondCreate();

    num_echo = 0;
    num_write = 0;
    
    if (isInitMonitor) return -1;

    isInitMonitor = true;

    for (int i = 0; i < NUM_TERMINALS; i++) {
        isInit[i] = false;
    }


    return 0;
}

extern int 
TerminalDriverStatistics(struct termstat *stats)
{
    Declare_Monitor_Entry_Procedure();
    
    if (stats == NULL) {
        printf("[ERROR]: stats is NULL\n");
        return -1;
    }

    num_stas++;

    for (int i = 0; i < NUM_TERMINALS; i++) {
        if (!isInit[i]) {
            stats[i].tty_in = -1;
            stats[i].tty_out = -1;
            stats[i].user_in = -1;
            stats[i].user_out = -1;
        } else {
            stats[i].tty_in = tty_in[i];
            tty_in[i] = 0;

            stats[i].tty_out = tty_out[i];
            tty_out[i] = 0;

            stats[i].user_in = user_in[i];
            user_in[i] = 0;

            stats[i].user_out = user_out[i];
            user_out[i] = 0;
        }
    }
    
    num_stas--;

    // tty_in, tty_out, ... now will be updated
    // Add the previous stats happen during copying stats into termstat to the stats
    for (int i = 0; i < NUM_TERMINALS; i++) {
        if (isInit[i]) {
            tty_in[i] += tty_in_tmp[i];
            tty_in_tmp[i] = 0;

            tty_out[i] += tty_out_tmp[i];
            tty_out_tmp[i] = 0;

            user_in[i] += user_in_tmp[i];
            user_in_tmp[i] = 0;

            user_out[i] += user_out_tmp[i];
            user_out_tmp[i] = 0;
        } 
    }

    return 0;
}