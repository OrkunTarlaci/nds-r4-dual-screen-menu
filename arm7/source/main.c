// macro DS Lite - ARM7 cekirdegi
// BlocksDS varsayilan ARM7 cekirdeginin sadelestirilmis hali + oyun baslatma icin
// guvenilir "yukleyiciye gec" islemi (BIOS SoftReset yerine dogrudan ARM kipinde atlama).

#include <nds.h>

#define MACRO_LAUNCH_MAGIC 0x4C4F4144  // "LOAD"

volatile bool exit_loop = false;

void power_button_callback(void)
{
    exit_loop = true;
}

void vblank_handler(void)
{
    inputGetAndSend();
}

// ARM9 yukleyiciyi VRAM'e koydu ve adresini 0x02FFFE34'e yazdi.
// Iki islemci ayni anda gecsin diye libnds'teki el sikisma kullanilir,
// sonra ARM7 kesmeleri kapatip yukleyiciye ARM kipinde atlar.
static void launch_handler(u32 value, void *userdata)
{
    (void)userdata;
    if (value != MACRO_LAUNCH_MAGIC)
        return;

    REG_IME = 0;
    REG_IE = 0;
    REG_IF = ~0;

    REG_IPC_SYNC = 0x100;               // ARM9'a "hazirim" (1)
    while ((REG_IPC_SYNC & 0x0f) != 1)  // ARM9'un cevabini bekle
        ;
    REG_IPC_SYNC = 0;                   // ARM9'a "gidiyorum" (0)

    void (*loader)(void) = (void (*)(void))(*(vu32 *)0x02FFFE34);
    loader();  // cift adres -> ARM kipinde baslar
    while (1)
        ;
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    enableSound();
    readUserSettings();
    ledBlink(LED_ALWAYS_ON);
    touchInit();

    irqInit();
    fifoInit();

    installSoundFIFO();
    installSystemFIFO();

    fifoSetValue32Handler(FIFO_USER_08, launch_handler, NULL);

    setPowerButtonCB(power_button_callback);
    initClockIRQTimer(LIBNDS_DEFAULT_TIMER_RTC);

    irqSet(IRQ_VBLANK, vblank_handler);
    irqEnable(IRQ_VBLANK);

    while (!exit_loop)
        swiWaitForVBlank();

    return 0;
}
