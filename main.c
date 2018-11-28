#include <stdio.h>
#include "kernel/kernel.h"

int main(int argc, char** argv) {

    Spyre_State* S = Spyre_init(1024);   
    for (int i = 0; i < 2; i++) {
        Spyre_request_process(S, 76);
    }

    Spyre_take_control(S);

    return 0;
    
}   
