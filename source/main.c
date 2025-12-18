/*
 * Template Nintendo DS
 * May 2011
 */

#include <nds.h>
#include <stdio.h>
#include "graphics.h"

int main(void) {
	//---sub screen graphics setup---
    configureGraphics_Sub();
    configBG2_Sub();
    //------------------------------

    while(1)
        swiWaitForVBlank();	
}
