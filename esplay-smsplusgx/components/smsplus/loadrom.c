/******************************************************************************
 *  Sega Master System / GameGear Emulator
 *  Copyright (C) 1998-2007  Charles MacDonald
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *   ROM File Loading support
 *
 ******************************************************************************/

#include "shared.h"

extern unsigned long crc32(crc, buf, len);

#define GAME_DATABASE_CNT 93

typedef struct
{
    uint32 crc;
    uint8 glasses_3d;
    uint8 device;
    uint8 mapper;
    uint8 display;
    uint8 territory;
    uint8 console;
    const char *name;
} rominfo_t;

const rominfo_t game_list[GAME_DATABASE_CNT] =
    {
};

void set_config()
{
    int i;

    /* default sms settings */
    cart.mapper = MAPPER_SEGA;
    sms.console = CONSOLE_SMS2;
    sms.territory = TERRITORY_EXPORT;
    sms.display = DISPLAY_NTSC;
    sms.glasses_3d = 0;
    sms.device[0] = DEVICE_PAD2B;
    sms.device[1] = DEVICE_PAD2B;
    sms.use_fm = option.fm;

    /* console type detection */
    /* SMS Header is located at 0x7ff0 */
    if ((cart.size > 0x7000) && (!memcmp(&cart.rom[0x7ff0], "TMR SEGA", 8)))
    {
        uint8 region = (cart.rom[0x7fff] & 0xf0) >> 4;

        switch (region)
        {
        case 5:
            sms.console = CONSOLE_GG;
            sms.territory = TERRITORY_DOMESTIC;
            break;

        case 6:
        case 7:
            sms.console = CONSOLE_GG;
            sms.territory = TERRITORY_EXPORT;
            break;

        case 3:
            sms.console = CONSOLE_SMS;
            sms.territory = TERRITORY_DOMESTIC;
            break;

        default:
            sms.console = CONSOLE_SMS2;
            sms.territory = TERRITORY_EXPORT;
            break;
        }
    }

    sms.gun_offset = 20; /* default offset */

    /* retrieve game settings from database */
    for (i = 0; i < GAME_DATABASE_CNT; i++)
    {
        if (cart.crc == game_list[i].crc)
        {
            cart.mapper = game_list[i].mapper;
            sms.display = game_list[i].display;
            sms.territory = game_list[i].territory;
            sms.glasses_3d = game_list[i].glasses_3d;
            sms.console = game_list[i].console;
            sms.device[0] = game_list[i].device;
            if (game_list[i].device != DEVICE_LIGHTGUN)
                sms.device[1] = game_list[i].device;

            if ((strcmp(game_list[i].name, "Spacegun") == 0) ||
                (strcmp(game_list[i].name, "Gangster Town") == 0))
            {
                /* these games seem to use different gun position calculation method */
                sms.gun_offset = 16;
            }
            i = GAME_DATABASE_CNT;
        }
    }

    /* enable BIOS on SMS only */
    bios.enabled &= 2;
    if (IS_SMS)
        bios.enabled |= option.use_bios;

#if 1
    /* force settings if AUTO is not set*/
    if (option.console == 1)
        sms.console = CONSOLE_SMS;
    else if (option.console == 2)
        sms.console = CONSOLE_SMS2;
    else if (option.console == 3)
        sms.console = CONSOLE_GG;
    else if (option.console == 4)
        sms.console = CONSOLE_GGMS;
    else if (option.console == 5)
    {
        sms.console = CONSOLE_SG1000;
        cart.mapper = MAPPER_NONE;
    }
    else if (option.console == 6)
    {
        sms.console = CONSOLE_COLECO;
        cart.mapper = MAPPER_NONE;
    }

    if (option.country == 1) /* USA */
    {
        sms.display = DISPLAY_NTSC;
        sms.territory = TERRITORY_EXPORT;
    }
    else if (option.country == 2) /* EUROPE */
    {
        sms.display = DISPLAY_PAL;
        sms.territory = TERRITORY_EXPORT;
    }
    else if (option.country == 3) /* JAPAN */
    {
        sms.display = DISPLAY_NTSC;
        sms.territory = TERRITORY_DOMESTIC;
    }
#endif
}

int load_rom(char *filename)
{
    FILE *fd = NULL;

    size_t nameLength = strlen(filename);
    if (nameLength < 4)
    {
        printf("%s: file name too short. filename='%s', nameLength=%d\n",
               __func__, filename, nameLength);
        abort();
    }

    fd = fopen(filename, "rb");
    if (!fd)
        abort();

    /* Seek to end of file, and get size */

    fseek(fd, 0, SEEK_END);
    size_t actual_size = ftell(fd);
    fseek(fd, 0, SEEK_SET);

    cart.size = actual_size;
    if (cart.size < 0x4000)
        cart.size = 0x4000;

    cart.rom = ESP32_PSRAM;
    size_t cnt = fread(cart.rom, cart.size, 1, fd);
    //if (cnt != 1) abort();
    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
    __asm__("memw");

    fclose(fd);

    //cart.sram = ESP32_PSRAM + 0x280000;

    /* Take care of image header, if present */
    if ((cart.size / 512) & 1)
    {
        cart.size -= 512;
        memcpy(cart.rom, cart.rom + 512, cart.size);
    }

    /* 16k pages */
    cart.pages = cart.size / 0x4000;

    cart.crc = crc32(0, cart.rom, option.console == 6 ? actual_size : cart.size);
    cart.loaded = 1;

    set_config();

    printf("%s: OK. cart.crc=%#010lx\n", __func__, cart.crc);

    return 1;
}
