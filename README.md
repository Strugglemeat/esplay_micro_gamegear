# esplay_micro_gamegear

This is a modification of pebri86's esplay micro firmware to boot directly into games (no menus)

You can find the original firmware here https://github.com/pebri86/esplay-retro-emulation

My intention was to make this device operate similarly to an original gamegear: when you turn it on, a game starts up immediately. there are no menus to select your game from. there are, however, multiple games available: you can select a different game than default by holding one of the dpad directions or the start button.

There's still one big goal to accomplish here: I want to load the games from the internal flash memory of the ESP32 rather than from the SD card. I'm still trying to figure that one out, if anyone can provide me with insight it'd be greatly appreciated.

Here's an example of an NES emulator that loads directly from the internal flash memory: https://github.com/badvision/esp32_nesemu

youtube video I made about this firmware: https://youtu.be/DklmjbdR9SU

youtube video about the esplay micro device #1: https://www.youtube.com/watch?v=7j7IRWV2tp8 (not my video)

youtube video about the esplay micro device #2: https://www.youtube.com/watch?v=p0fNz_PFzWU (not my video)
