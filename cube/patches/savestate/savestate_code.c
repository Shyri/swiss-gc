#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

volatile unsigned long *ebase = (void *) 0xCC006800;

/* exi */
#define EXI_SR                ebase[0]
#define EXI_DMA_MEM           ebase[1]
#define EXI_DMA_LEN           ebase[2]
#define EXI_CR                ebase[3]
#define EXI_DATA              ebase[4]
#define EXI_WAIT_EOT          while((EXI_CR)&1);

int USBSend(unsigned char c);

int sendCommands() {


    USBSend('M');
    USBSend('Y');
    USBSend(' ');
    USBSend('F');
    USBSend('O');
    USBSend('C');
    USBSend('K');
    USBSend('I');
    USBSend('N');
    USBSend('G');
    USBSend(' ');
    USBSend('A');
    USBSend('S');
    USBSend('S');

    return 0;
}

int USBSend(unsigned char c) {
    EXI_SR = 0x250; // serial
    EXI_DATA = 0xA0000000 | (c << 20); // write byte
    EXI_CR = 0x19;  // bidir
    EXI_WAIT_EOT;
    unsigned int i = EXI_DATA;
    EXI_SR = 0x0;

    if (i & 0x04000000)
        return 1;

    return 0;
}
