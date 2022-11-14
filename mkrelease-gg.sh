#!/bin/sh

#edit your ESP-IDF path here, tested with release/v3.3 branch
export IDF_PATH=~/esp/esp-idf

#tune this to match yours
export ESPLAY_SDK=~/esp32/esplay/esplay-retro-emulation/esplay-sdk

export PATH="$HOME/esp/xtensa-esp32-elf/bin:$PATH"

#ffmpeg path
export PATH=/c/ffmpeg/bin:$PATH

cd esplay-smsplusgx
make -j4

cd ..

ffmpeg -i assets/Tile.png -f rawvideo -pix_fmt rgb565 assets/tile.raw -y
~/esp32/esplay/esplay-base-firmware/tools/mkfw/mkfw super-columns assets/tile.raw 0 16 1179648 esplay-smsplusgx esplay-smsplusgx/build/esplay-smsplusgx.bin
# 0 17 131072 supcol supcol.bin

rm gamegear.fw
mv firmware.fw gamegear.fw
