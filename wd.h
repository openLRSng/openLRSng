static inline void watchdogReset();
#define WATCHDOG_OFF    (0)
#define WATCHDOG_16MS   (_BV(WDE))
#define WATCHDOG_32MS   (_BV(WDP0) | _BV(WDE))
#define WATCHDOG_64MS   (_BV(WDP1) | _BV(WDE))
#define WATCHDOG_125MS  (_BV(WDP1) | _BV(WDP0) | _BV(WDE))
#define WATCHDOG_250MS  (_BV(WDP2) | _BV(WDE))
#define WATCHDOG_500MS  (_BV(WDP2) | _BV(WDP0) | _BV(WDE))
#define WATCHDOG_1S     (_BV(WDP2) | _BV(WDP1) | _BV(WDE))
#define WATCHDOG_2S     (_BV(WDP2) | _BV(WDP1) | _BV(WDP0) | _BV(WDE))
#define WATCHDOG_4S     (_BV(WDP3) | _BV(WDE))
#define WATCHDOG_8S     (_BV(WDP3) | _BV(WDP0) | _BV(WDE))
void watchdogConfig(uint8_t x);
static bool watchdogUsed = false;

#if defined(__AVR_ATmega32U4__)
bool watchdogAvailable()
{
  return true;
}
#else
#define boot_lock_fuse_bits_get(address)                \
(__extension__({                                        \
    uint8_t __result;                                   \
    __asm__ __volatile__                                \
    (                                                   \
        "sts %1, %2\n\t"                                \
        "lpm %0, Z\n\t"                                 \
        : "=r" (__result)                               \
        : "i" (_SFR_MEM_ADDR(SPMCSR)),                  \
          "r" ((uint8_t)(_BV(SELFPRGEN) | _BV(BLBSET))),\
          "z" ((uint16_t)address)                       \
    );                                                  \
    __result;                                           \
}))
bool watchdogAvailable()
{
  // This basically checks bootloader size to detect optiboot
  return ((boot_lock_fuse_bits_get(3) & 0x06) == 0x6);
}
#endif

void watchdogReset()
{
#if defined(__AVR_ATmega32U4__)
  // this allows leonardo bootloader to work
  if (*(uint16_t *)0x0800 == 0x7777) {
    return;
  }
#endif
  __asm__ __volatile__ ( "wdr\n" );
}

void watchdogConfig(uint8_t x)
{
  if (watchdogAvailable()) {
    uint8_t _sreg = SREG;
    watchdogUsed=1;
    cli();
    watchdogReset();
    WDTCSR = _BV(WDCE) | _BV(WDE);
    WDTCSR = x;
    if (x) {
      watchdogReset();
    }
    SREG = _sreg;
  }
}

