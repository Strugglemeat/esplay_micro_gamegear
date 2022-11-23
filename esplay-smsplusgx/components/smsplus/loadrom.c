#include "shared.h"
#include "rom/supcol.h"

#include "rom/shenzhen.h"
#include "rom/poker.h"
#include "rom/4in1.h"
#include "rom/funpak.h"
//#include "rom/bakubaku.h"
#include "rom/tetris.h"

int load_rom(char *filename)
{
size_t actual_size = 262144;

    cart.size = actual_size;

    cart.rom = ESP32_PSRAM;
/*
if(strcmp(filename,"shenzhen.gg")==0)memcpy(cart.rom,shenzhen_rom,32768);
else if(strcmp(filename,"poker.gg")==0)memcpy(cart.rom,poker_rom,131072);
else if(strcmp(filename,"4in1.gg")==0)memcpy(cart.rom,fourinone_rom,262144);
else if(strcmp(filename,"funpak.gg")==0)memcpy(cart.rom,funpak_rom,262144);
else if(strcmp(filename,"bakubaku.gg")==0)memcpy(cart.rom,bakubaku_rom,262144);
else if(strcmp(filename,"tetris.gg")==0)memcpy(cart.rom,tetris_rom,65536);
else memcpy(cart.rom,supcol_rom,131072);
*/


if(strcmp(filename,"shenzhen.gg")==0)memcpy(cart.rom,shenzhen_rom,cart.size);
else if(strcmp(filename,"poker.gg")==0)memcpy(cart.rom,poker_rom,cart.size);
else if(strcmp(filename,"4in1.gg")==0)memcpy(cart.rom,fourinone_rom,cart.size);
else if(strcmp(filename,"funpak.gg")==0)memcpy(cart.rom,funpak_rom,cart.size);
//else if(strcmp(filename,"bakubaku.gg")==0)memcpy(cart.rom,bakubaku_rom,cart.size);
else if(strcmp(filename,"tetris.gg")==0)memcpy(cart.rom,tetris_rom,cart.size);
else memcpy(cart.rom,supcol_rom,cart.size);


//memcpy(cart.rom,supcol_rom,cart.size);
//memcpy(cart.rom,shenzhen_rom,cart.size);

    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
    __asm__("memw");

    /* Take care of image header, if present */
    if ((cart.size / 512) & 1)
    {
        cart.size -= 512;
        memcpy(cart.rom, cart.rom + 512, cart.size);
    }

    /* 16k pages */
    cart.pages = cart.size / 0x4000;

    cart.loaded = 1;

//set config
    cart.mapper = MAPPER_SEGA;
    sms.console = CONSOLE_GG;
    sms.territory = TERRITORY_EXPORT;
    sms.display = DISPLAY_NTSC;
    sms.glasses_3d = 0;
    sms.device[0] = DEVICE_PAD2B;
    sms.use_fm = 0;

    return 1;
}
