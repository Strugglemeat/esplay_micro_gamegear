#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_partition.h"
#include "driver/i2s.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
//#include "esp_sleep.h"
//#include "driver/rtc_io.h"
#include <limits.h>

#include "../components/smsplus/shared.h"

//#include "settings.h"
#include "audio.h"
#include "gamepad.h"
#include "system.h"
#include "display.h"
#include "sdcard.h"
#include "power.h"
#include "display_sms.h"

#include <dirent.h>

int32_t scaleAlg;

const char *SD_BASE_PATH = "/sd";

#define AUDIO_SAMPLE_RATE (32000)

uint16 palette[PALETTE_SIZE];
uint8_t *framebuffer[2];
int currentFramebuffer = 0;

uint32_t *audioBuffer = NULL;
int audioBufferCount = 0;
int showOverlay = 0;

//spi_flash_mmap_handle_t hrom;

QueueHandle_t vidQueue;
TaskHandle_t videoTaskHandle;

int Volume;
battery_state battery;

volatile bool videoTaskIsRunning = false;
esplay_scale_option opt;

void videoTask(void *arg)
{
    uint8_t *param;

    videoTaskIsRunning = true;

    const bool isGameGear = (sms.console == CONSOLE_GG) | (sms.console == CONSOLE_GGMS);

    while (1)
    {
        xQueuePeek(vidQueue, &param, portMAX_DELAY);

        if (param == 1)
            break;

            render_copy_palette(palette);
            write_sms_frame(param, palette, isGameGear, opt);

        battery_level_read(&battery);

        xQueueReceive(vidQueue, &param, portMAX_DELAY);
    }

    videoTaskIsRunning = false;
    vTaskDelete(NULL);

    while (1)
    {
    }

}


//Read an unaligned byte.
char unalChar(const unsigned char *adr)
{
    //See if the byte is in memory that can be read unaligned anyway.
    if (!(((int)adr) & 0x40000000))
        return *adr;
    //Nope: grab a word and distill the byte.
    int *p = (int *)((int)adr & 0xfffffffc);
    int v = *p;
    int w = ((int)adr & 3);
    if (w == 0)
        return ((v >> 0) & 0xff);
    if (w == 1)
        return ((v >> 8) & 0xff);
    if (w == 2)
        return ((v >> 16) & 0xff);
    if (w == 3)
        return ((v >> 24) & 0xff);

    abort();
    return 0;
}

//char cartName[1024];
void app_main(void)
{
    framebuffer[0] = heap_caps_malloc(256 * 192, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    if (!framebuffer[0])
        abort();
    printf("app_main: framebuffer[0]=%p\n", framebuffer[0]);

    framebuffer[1] = heap_caps_malloc(256 * 192, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    if (!framebuffer[1])
        abort();
    printf("app_main: framebuffer[1]=%p\n", framebuffer[1]);

    settings_init();

    esplay_system_init();

    // Joystick.
    gamepad_init();

    // audio
    audio_init(AUDIO_SAMPLE_RATE);

    // battery
    battery_level_init();

    display_prepare();

    display_init();
    //int brightness;

     set_display_brightness(70);

    // load alghorithm
    //settings_load(SettingAlg, &scaleAlg);

    // Open SD card
    esp_err_t r = sdcard_open(SD_BASE_PATH);
    if (r != ESP_OK)
    {
        abort();
    }

	const char *FILENAME = NULL;
	FILENAME = "/sd/roms/gg/supcol.gg";

        input_gamepad_state bootState = gamepad_input_read_raw();

        if (bootState.values[GAMEPAD_INPUT_START])
        {
	FILENAME = "/sd/roms/gg/poker.gg";
        }

        if (bootState.values[GAMEPAD_INPUT_UP])
        {
	FILENAME = "/sd/roms/gg/bakubaku.gg";
        }

        if (bootState.values[GAMEPAD_INPUT_DOWN])
        {
	FILENAME = "/sd/roms/gg/4in1.gg";
        }

        if (bootState.values[GAMEPAD_INPUT_LEFT])
        {
	FILENAME = "/sd/roms/gg/funpak.gg";
        }

        if (bootState.values[GAMEPAD_INPUT_RIGHT])
        {
	FILENAME = "/sd/roms/gg/tetris.gg";
        }

    // Load the ROM
    load_rom(FILENAME);

    write_sms_frame(NULL, NULL, false, SCALE_STRETCH);

    vidQueue = xQueueCreate(1, sizeof(uint16_t *));
    xTaskCreatePinnedToCore(&videoTask, "videoTask", 1024 * 4, NULL, 5, &videoTaskHandle, 1);

    sms.use_fm = 0;

    bitmap.width = 256;
    bitmap.height = 192;
    bitmap.pitch = bitmap.width;
    //bitmap.depth = 8;
    bitmap.data = framebuffer[0];

    set_option_defaults();

    option.sndrate = AUDIO_SAMPLE_RATE;
    option.overscan = 0;
    option.extra_gg = 0;

    system_init2();
    system_reset();

    input_gamepad_state previousState;
    gamepad_read(&previousState);

    uint startTime;
    uint stopTime;
    uint totalElapsedTime = 0;
    int frame = 0;
    uint16_t muteFrameCount = 0;
    //uint16_t powerFrameCount = 0;

    while (true)
    {
        input_gamepad_state joystick;
        gamepad_read(&joystick);

        startTime = xthal_get_ccount();

        int smsButtons = 0;
        if (joystick.values[GAMEPAD_INPUT_UP])
            smsButtons |= INPUT_UP;
        if (joystick.values[GAMEPAD_INPUT_DOWN])
            smsButtons |= INPUT_DOWN;
        if (joystick.values[GAMEPAD_INPUT_LEFT])
            smsButtons |= INPUT_LEFT;
        if (joystick.values[GAMEPAD_INPUT_RIGHT])
            smsButtons |= INPUT_RIGHT;
        if (joystick.values[GAMEPAD_INPUT_A])
            smsButtons |= INPUT_BUTTON2;
        if (joystick.values[GAMEPAD_INPUT_B])
            smsButtons |= INPUT_BUTTON1;

        int smsSystem = 0;
        if (joystick.values[GAMEPAD_INPUT_START])
            smsSystem |= INPUT_START;

        input.pad[0] = smsButtons;
        input.system = smsSystem;

        if (0 || (frame % 2) == 0)
        {
            system_frame(0);

            xQueueSend(vidQueue, &bitmap.data, portMAX_DELAY);

            currentFramebuffer = currentFramebuffer ? 0 : 1;
            bitmap.data = framebuffer[currentFramebuffer];
        }
        else
        {
            system_frame(1);
        }

        // Create a buffer for audio if needed
        if (!audioBuffer || audioBufferCount < snd.sample_count)
        {
            if (audioBuffer)
                free(audioBuffer);

            size_t bufferSize = snd.sample_count * 2 * sizeof(int16_t);
            audioBuffer = heap_caps_malloc(bufferSize, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
            if (!audioBuffer)
                abort();

            audioBufferCount = snd.sample_count;

            printf("app_main: Created audio buffer (%d bytes).\n", bufferSize);
        }

        // Process audio
        for (int x = 0; x < snd.sample_count; x++)
        {
            uint32_t sample;

            if (muteFrameCount < 60 * 2)
            {
                // When the emulator starts, audible poping is generated.
                // Audio should be disabled during this startup period.
                sample = 0;
                ++muteFrameCount;
            }
            else
            {
                sample = (snd.output[0][x] << 16) + snd.output[1][x];
            }

            audioBuffer[x] = sample;
        }

        // send audio

        audio_submit((short *)audioBuffer, snd.sample_count - 1);

        stopTime = xthal_get_ccount();

        previousState = joystick;

        int elapsedTime;
        if (stopTime > startTime)
            elapsedTime = (stopTime - startTime);
        else
            elapsedTime = ((uint64_t)stopTime + (uint64_t)0xffffffff) - (startTime);

        totalElapsedTime += elapsedTime;
        ++frame;

        if (frame == 60)
        {
            float seconds = totalElapsedTime / (CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ * 1000000.0f);
            float fps = frame / seconds;

            printf("HEAP:0x%x, FPS:%f, BATTERY:%d [%d]\n", esp_get_free_heap_size(), fps, battery.millivolts, battery.percentage);

            frame = 0;
            totalElapsedTime = 0;
        }
    }
}
