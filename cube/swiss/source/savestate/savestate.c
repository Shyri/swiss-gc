//
// Created by Shyri on 2020-03-15.
//

#include <stdio.h>
#include <gccore.h>		/*** Wrapper to include common libogc headers ***/
#include <ogcsys.h>		/*** Needed for console support ***/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <malloc.h>
#include <ctype.h>
#include "swiss.h"
#include "main.h"
#include "cheats.h"
#include "patcher.h"
#include "deviceHandler.h"
#include "FrameBufferMagic.h"
#include "savestate.h"

void install_savestate() {
    u8 *ptr = savestatepatch_bin;
    u32 size = savestatepatch_bin_size;

    print_gecko("Copying savestate%s to %08X\r\n", "",(u32)CHEATS_ENGINE);
    memcpy(CHEATS_ENGINE, ptr, size);
    memcpy(CHEATS_GAMEID, (void*)0x80000000, CHEATS_GAMEID_LEN);

    DCFlushRange((void*)CHEATS_ENGINE, WIIRD_ENGINE_SPACE);
    ICInvalidateRange((void*)CHEATS_ENGINE, WIIRD_ENGINE_SPACE);
}