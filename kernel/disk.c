#include "disk.h"

void Spyre_init_disk(Spyre_State* S) {
    
    S->disk = fopen("../disk.bin", "rb");
    if (!S->disk.is_open()) {
        printf("Failed to open disk\n");
        return;
    }

}
