/*
 * Template Nintendo DS
 * May 2011
 */

#include <nds.h>
#include <stdio.h>

#include "home_page.h"

int main(void) {
    //---sub screen graphics setup---
    configureGraphics_Sub_home_page();
    configBG2_Sub_homepage();
    //------------------------------

    while (1)
        swiWaitForVBlank();
}
