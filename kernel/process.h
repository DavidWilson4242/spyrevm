#ifndef PROCESS_H
#define PROCESS_H

#include "../vm_types.h"

struct Spyre_Process { 

    SPY_ADDR     process_memory;
    SPY_ADDR     ip;
    SPY_ADDR     sp;
    SPY_ADDR     bp;
    U8           pid;
    int          running;
    Spyre_State* master;
    
};

void Process_step(Spyre_Process*);

#endif
