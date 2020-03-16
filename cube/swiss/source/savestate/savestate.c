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

static CheatEntries _cheats;


int savestate_get_maxsize() {
    return CHEATS_MAX_SIZE((savestatepatch_bin_size));
}


void install_savestate() {
//    int isDebug = swissSettings.wiirdDebug;
    // If high memory is in use, we'll use low, otherwise high.
    u8 *ptr = savestatepatch_bin;
    u32 size = savestatepatch_bin_size;

    print_gecko("Copying savestate%s to %08X\r\n", "",(u32)CHEATS_ENGINE);
    memcpy(CHEATS_ENGINE, ptr, size);
    memcpy(CHEATS_GAMEID, (void*)0x80000000, CHEATS_GAMEID_LEN);
//    if(!isDebug) {
        CHEATS_ENABLE_CHEATS = CHEATS_TRUE;
//    }
    CHEATS_START_PAUSED = CHEATS_FALSE;
    memset(CHEATS_LOCATION(size), 0, savestate_get_maxsize());
    print_gecko("Copying %i bytes of cheats to %08X\r\n", getEnabledCheatsSize(),(u32)CHEATS_LOCATION(size));
    u32 *cheatsLocation = (u32*)CHEATS_LOCATION(size);
    cheatsLocation[0] = 0x00D0C0DE;
    cheatsLocation[1] = 0x00D0C0DE;
    cheatsLocation+=2;

    int i = 0, j = 0;
    for(i = 0; i < _cheats.num_cheats; i++) {
        CheatEntry *cheat = &_cheats.cheat[i];
        if(cheat->enabled) {
            for(j = 0; j < cheat->num_codes; j++) {
                // Copy & fix cheats that want to jump to the old cheat engine location 0x800018A8 -> CHEATS_ENGINE+0xA8
                cheatsLocation[0] = cheat->codes[j][0];
                cheatsLocation[1] = cheat->codes[j][1] == 0x800018A8 ? (u32)(CHEATS_ENGINE+0xA8) : cheat->codes[j][1];
                cheatsLocation+=2;
            }
        }
    }
    cheatsLocation[0] = 0xFF000000;
    DCFlushRange((void*)CHEATS_ENGINE, WIIRD_ENGINE_SPACE);
    ICInvalidateRange((void*)CHEATS_ENGINE, WIIRD_ENGINE_SPACE);
}