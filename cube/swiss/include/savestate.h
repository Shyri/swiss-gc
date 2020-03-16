//
// Created by Shyri on 2020-03-15.
//

#ifndef SAVESTATE_H
#define SAVESTATE_H

extern int savestatepatch_bin_size;
extern u8 savestatepatch_bin[];

#define SAVESTATE_PATCH_START		((void*)(SAVESTATE_PATCH+0xA8))

void install_savestate();

#endif //SAVESTATE_H
