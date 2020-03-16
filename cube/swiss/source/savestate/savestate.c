//
// Created by Shyri on 2020-03-15.
//

#include "swiss.h"
#include "patcher.h"
#include "savestate.h"

void install_savestate() {
    u8 *ptr = savestatepatch_bin;
    u32 size = savestatepatch_bin_size;

    print_gecko("Copying savestate to %08X\r\n", (u32)SAVESTATE_PATCH);
    memcpy(SAVESTATE_PATCH, ptr, size);

    DCFlushRange((void*)SAVESTATE_PATCH, SAVESTATE_PATCH_SPACE);
    ICInvalidateRange((void*)SAVESTATE_PATCH, SAVESTATE_PATCH_SPACE);
}