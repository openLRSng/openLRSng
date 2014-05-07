// OpenLRSng binding

// Factory setting values, modify via the CLI

//####### RADIOLINK RF POWER (beacon is always 100/13/1.3mW) #######
// 7 == 100mW (or 1000mW with M3)
// 6 == 50mW (use this when using booster amp), (800mW with M3)
// 5 == 25mW
// 4 == 13mW
// 3 == 6mW
// 2 == 3mW
// 1 == 1.6mW
// 0 == 1.3mW
#define DEFAULT_RF_POWER 7

#define DEFAULT_CARRIER_FREQUENCY 435000000  // Hz  startup frequency
#define DEFAULT_CHANNEL_SPACING 5 // 50kHz
#define DEFAULT_HOPLIST 22,10,19,34,49,41
#define DEFAULT_RF_MAGIC 0xDEADFEED

//  0 -- 4800bps, best range
//  1 -- 9600bps, medium range
//  2 -- 19200bps, medium range
#define DEFAULT_DATARATE 2

#define DEFAULT_BAUDRATE 115200

// TX_CONFIG flag masks
#define MUTE_TX             0x10 // do not beep on telemetry loss
#define MICROPPM            0x20
#define INVERTED_PPMIN      0x40
#define WATCHDOG_USED       0x80 // read only flag, only sent to configurator

// RX_CONFIG flag masks
#define PPM_MAX_8CH         0x01
#define ALWAYS_BIND         0x02
#define SLAVE_MODE          0x04
#define IMMEDIATE_OUTPUT    0x08
#define WATCHDOG_USED       0x80 // read only flag, only sent to configurator

// BIND_DATA flag masks
#define TELEMETRY_OFF       0x00
#define TELEMETRY_PASSTHRU  0x08
#define TELEMETRY_FRSKY     0x10 // covers smartport if used with &
#define TELEMETRY_SMARTPORT 0x18
#define TELEMETRY_MASK      0x18
#define CHANNELS_4_4        0x01
#define CHANNELS_8          0x02
#define CHANNELS_8_4        0x03
#define CHANNELS_12         0x04
#define CHANNELS_12_4       0x05
#define CHANNELS_16         0x06
#define DEFAULT_FLAGS       (CHANNELS_8 | TELEMETRY_PASSTHRU)


// helper macro for European PMR channels
#define EU_PMR_CH(x) (445993750L + 12500L * (x)) // valid for ch1-ch8

// helper macro for US FRS channels 1-7
#define US_FRS_CH(x) (462537500L + 25000L * (x)) // valid for ch1-ch7

#define DEFAULT_BEACON_FREQUENCY 0 // disable beacon
#define DEFAULT_BEACON_DEADTIME 30 // time to wait until go into beacon mode (30s)
#define DEFAULT_BEACON_INTERVAL 10 // interval between beacon transmits (10s)

#define MIN_DEADTIME 0
#define MAX_DEADTIME 255

#define MIN_INTERVAL 1
#define MAX_INTERVAL 255

#define BINDING_POWER     0x06 // not lowest since may result fail with RFM23BP

#define TELEMETRY_PACKETSIZE 9

#define BIND_MAGIC (0xDEC1BE15 + (OPENLRSNG_VERSION & 0xfff0))
#define BINDING_VERSION ((OPENLRSNG_VERSION & 0x0ff0)>>4)

static uint8_t default_hop_list[] = {DEFAULT_HOPLIST};

// HW frequency limits
#if (defined RFMXX_868)
#  define MIN_RFM_FREQUENCY 848000000
#  define MAX_RFM_FREQUENCY 888000000
#  define DEFAULT_CARRIER_FREQUENCY 868000000  // Hz  (ch 0)
#  define BINDING_FREQUENCY 868000000 // Hz
#elif (defined RFMXX_915)
#  define MIN_RFM_FREQUENCY 895000000
#  define MAX_RFM_FREQUENCY 935000000
#  define DEFAULT_CARRIER_FREQUENCY 915000000  // Hz  (ch 0)
#  define BINDING_FREQUENCY 915000000 // Hz
#else
#  define MIN_RFM_FREQUENCY 413000000
#  define MAX_RFM_FREQUENCY 463000000
#  define DEFAULT_CARRIER_FREQUENCY 435000000  // Hz  (ch 0)
#  define BINDING_FREQUENCY 435000000 // Hz
#endif

#define MAXHOPS      24
#define PPM_CHANNELS 16

uint8_t activeProfile = 0;

struct tx_config {
  uint8_t  rfm_type;
  uint32_t max_frequency;
  uint32_t flags;
} tx_config;

struct RX_config {
  uint8_t  rx_type; // RX type fillled in by RX, do not change
  uint8_t  pinMapping[13];
  uint8_t  flags;
  uint8_t  RSSIpwm;
  uint32_t beacon_frequency;
  uint8_t  beacon_deadtime;
  uint8_t  beacon_interval;
  uint16_t minsync;
  uint8_t  failsafeDelay;
  uint8_t  ppmStopDelay;
  uint8_t  pwmStopDelay;
} rx_config;


struct bind_data {
  uint8_t version;
  uint32_t serial_baudrate;
  uint32_t rf_frequency;
  uint32_t rf_magic;
  uint8_t rf_power;
  uint8_t rf_channel_spacing;
  uint8_t hopchannel[MAXHOPS];
  uint8_t modem_params;
  uint8_t flags;
} bind_data;

struct rfm22_modem_regs {
  uint32_t bps;
  uint8_t  r_1c, r_1d, r_1e, r_20, r_21, r_22, r_23, r_24, r_25, r_2a, r_6e, r_6f, r_70, r_71, r_72;
} modem_params[] = {
  { 4800, 0x1a, 0x40, 0x0a, 0xa1, 0x20, 0x4e, 0xa5, 0x00, 0x1b, 0x1e, 0x27, 0x52, 0x2c, 0x23, 0x30 }, // 50000 0x00
  { 9600, 0x05, 0x40, 0x0a, 0xa1, 0x20, 0x4e, 0xa5, 0x00, 0x20, 0x24, 0x4e, 0xa5, 0x2c, 0x23, 0x30 }, // 25000 0x00
  { 19200, 0x06, 0x40, 0x0a, 0xd0, 0x00, 0x9d, 0x49, 0x00, 0x7b, 0x28, 0x9d, 0x49, 0x2c, 0x23, 0x30 }, // 25000 0x01
  { 57600, 0x05, 0x40, 0x0a, 0x45, 0x01, 0xd7, 0xdc, 0x03, 0xb8, 0x1e, 0x0e, 0xbf, 0x00, 0x23, 0x2e },
  { 125000, 0x8a, 0x40, 0x0a, 0x60, 0x01, 0x55, 0x55, 0x02, 0xad, 0x1e, 0x20, 0x00, 0x00, 0x23, 0xc8 },
};

#define DATARATE_COUNT (sizeof(modem_params) / sizeof(modem_params[0]))

struct rfm22_modem_regs bind_params =
{ 9600, 0x05, 0x40, 0x0a, 0xa1, 0x20, 0x4e, 0xa5, 0x00, 0x20, 0x24, 0x4e, 0xa5, 0x2c, 0x23, 0x30 };

// prototype
void fatalBlink(uint8_t blinks);

#include <avr/eeprom.h>

// Save EEPROM by writing just changed data
void myEEPROMwrite(int16_t addr, uint8_t data)
{
  uint8_t retries = 5;
  while ((--retries) && (data != eeprom_read_byte((uint8_t *)addr))) {
    eeprom_write_byte((uint8_t *)addr, data);
  }
  if (!retries) {
    fatalBlink(2);
  }
}

static uint16_t CRC16_value;

inline void CRC16_reset()
{
  CRC16_value = 0;
}

void CRC16_add(uint8_t c) // CCITT polynome
{
  uint8_t i;
  CRC16_value ^= (uint16_t)c << 8;
  for (i = 0; i < 8; i++) {
    if (CRC16_value & 0x8000) {
      CRC16_value = (CRC16_value << 1) ^ 0x1021;
    } else {
      CRC16_value = (CRC16_value << 1);
    }
  }
}

// Halt and blink failure code
void fatalBlink(uint8_t blinks)
{
  while (1) {
    for (uint8_t i=0; i < blinks; i++) {
      Red_LED_ON;
      delay(100);
      Red_LED_OFF;
      delay(100);
    }
    delay(300);
  }
}

#ifndef COMPILE_TX
extern uint16_t failsafePPM[PPM_CHANNELS];
#endif

#define EEPROM_SIZE 1024 // EEPROM is 1k on 328p and 32u4
#define ROUNDUP(x) (((x)+15)&0xfff0)
#define MIN256(x)  (((x)<256)?256:(x))
#ifdef COMPILE_TX
#define EEPROM_DATASIZE MIN256(ROUNDUP((sizeof(tx_config) + sizeof(bind_data) + 4) * 4 + 3))
#else
#define EEPROM_DATASIZE MIN256(ROUNDUP(sizeof(rx_config) + sizeof(bind_data) + sizeof(failsafePPM) + 6))
#endif


bool accessEEPROM(uint8_t dataType, bool write)
{
  void *dataAddress = NULL;
  uint16_t dataSize = 0;

  uint16_t addressNeedle = 0;
  uint16_t addressBase = 0;
  uint16_t CRC = 0;

  do {
start:
#ifdef COMPILE_TX
    if (dataType == 0) {
      dataAddress = &tx_config;
      dataSize = sizeof(tx_config);
      addressNeedle = (sizeof(tx_config) + sizeof(bind_data) + 4) * activeProfile;
    } else if (dataType == 1) {
      dataAddress = &bind_data;
      dataSize = sizeof(bind_data);
      addressNeedle = sizeof(tx_config) + 2;
      addressNeedle += (sizeof(tx_config) + sizeof(bind_data) + 4) * activeProfile;
    } else if (dataType == 2) {
      dataAddress = &activeProfile;
      dataSize = 1;
      addressNeedle = (sizeof(tx_config) + sizeof(bind_data) + 4) * 4; // activeProfile is stored behind all 4 profiles
    }
#else
    if (dataType == 0) {
      dataAddress = &rx_config;
      dataSize = sizeof(rx_config);
      addressNeedle = 0;
    } else if (dataType == 1) {
      dataAddress = &bind_data;
      dataSize = sizeof(bind_data);
      addressNeedle = sizeof(rx_config) + 2;
    } else if (dataType == 2) {
      dataAddress = &failsafePPM;
      dataSize = sizeof(failsafePPM);
      addressNeedle = sizeof(rx_config) + sizeof(bind_data) + 4;
    }
#endif
    addressNeedle += addressBase;
    CRC16_reset();

    for (uint8_t i = 0; i < dataSize; i++, addressNeedle++) {
      if (!write) {
        *((uint8_t*)dataAddress + i) = eeprom_read_byte((uint8_t *)(addressNeedle));
      } else {
        myEEPROMwrite(addressNeedle, *((uint8_t*)dataAddress + i));
      }

      CRC16_add(*((uint8_t*)dataAddress + i));
    }

    if (!write) {
      CRC = eeprom_read_byte((uint8_t *)addressNeedle) << 8 | eeprom_read_byte((uint8_t *)(addressNeedle + 1));

      if (CRC16_value == CRC) {
        // recover corrupted data
        // write operation is performed after every successful read operation, this will keep all cells valid
        write = true;
        addressBase = 0;
        goto start;
      } else {
        // try next block
      }
    } else {
      myEEPROMwrite(addressNeedle++, CRC16_value >> 8);
      myEEPROMwrite(addressNeedle, CRC16_value & 0x00FF);
    }
    addressBase += EEPROM_DATASIZE;
  } while (addressBase <= (EEPROM_SIZE - EEPROM_DATASIZE));
  return (write); // success on write, failure on read
}

bool bindReadEeprom()
{
  if (accessEEPROM(1, false) && (bind_data.version == BINDING_VERSION)) {
    return true;
  }
  return false;
}

void bindWriteEeprom()
{
  accessEEPROM(1, true);
}

void bindInitDefaults(void)
{
  bind_data.version = BINDING_VERSION;
  bind_data.serial_baudrate = DEFAULT_BAUDRATE;
  bind_data.rf_power = DEFAULT_RF_POWER;
  bind_data.rf_frequency = DEFAULT_CARRIER_FREQUENCY;
  bind_data.rf_channel_spacing = DEFAULT_CHANNEL_SPACING;

  bind_data.rf_magic = DEFAULT_RF_MAGIC;

  for (uint8_t c = 0; c < MAXHOPS; c++) {
    bind_data.hopchannel[c] = (c < sizeof(default_hop_list)) ? default_hop_list[c] : 0;
  }

  bind_data.modem_params = DEFAULT_DATARATE;
  bind_data.flags = DEFAULT_FLAGS;
}

#ifdef COMPILE_TX
#define TX_PROFILE_COUNT 4

void profileSet()
{
  accessEEPROM(2, true);
}

void profileInit()
{
  accessEEPROM(2, false);
  if (activeProfile >= TX_PROFILE_COUNT) {
    activeProfile = 0;
    profileSet();
  }
}

void profileSwap(uint8_t profile)
{
  profileInit();
  if ((activeProfile != profile) && (profile < TX_PROFILE_COUNT)) {
    activeProfile = profile;
    profileSet();
  }
}

void txInitDefaults()
{
  tx_config.max_frequency = MAX_RFM_FREQUENCY;
  tx_config.flags = 0x00;
}

void txWriteEeprom()
{
  accessEEPROM(0,true);
  accessEEPROM(1,true);
}

void txReadEeprom()
{
  if ((!accessEEPROM(0, false)) || (!accessEEPROM(1, false))) {
    txInitDefaults();
    bindInitDefaults();
    txWriteEeprom();
  }
}

void bindRandomize(void)
{
  uint8_t emergency_counter = 0;
  uint8_t c;
  uint32_t t = 0;
  while (t == 0) {
    t = micros();
  }
  srandom(t);

  bind_data.rf_magic = 0;
  for (c = 0; c < 4; c++) {
    bind_data.rf_magic = (bind_data.rf_magic << 8) + (random() % 255);
  }

  // TODO: verify if this works properly
  for (c = 0; (c < MAXHOPS) && (bind_data.hopchannel[c] != 0); c++) {
again:
    if (emergency_counter++ == 255) {
      bindInitDefaults();
      return;
    }

    uint8_t ch = (random() % 50) + 1;

    // don't allow same channel twice
    for (uint8_t i = 0; i < c; i++) {
      if (bind_data.hopchannel[i] == ch) {
        goto again;
      }
    }

    // don't allow frequencies higher then tx_config.max_frequency
    uint32_t real_frequency = bind_data.rf_frequency + ch * bind_data.rf_channel_spacing * 10000;
    if (real_frequency > tx_config.max_frequency) {
      goto again;
    }

    bind_data.hopchannel[c] = ch;
  }
}
#endif

// non linear mapping
// 0 - 0
// 1-99    - 100ms - 9900ms (100ms res)
// 100-189 - 10s  - 99s   (1s res)
// 190-209 - 100s - 290s (10s res)
// 210-255 - 5m - 50m (1m res)
uint32_t delayInMs(uint16_t d)
{
  uint32_t ms;
  if (d < 100) {
    ms = d;
  } else if (d < 190) {
    ms = (d - 90) * 10UL;
  } else if (d < 210) {
    ms = (d - 180) * 100UL;
  } else {
    ms = (d - 205) * 600UL;
  }
  return ms * 100UL;
}

// non linear mapping
// 0-89    - 10s - 99s
// 90-109  - 100s - 290s (10s res)
// 110-255 - 5m - 150m (1m res)
uint32_t delayInMsLong(uint8_t d)
{
  return delayInMs((uint16_t)d + 100);
}

#ifndef COMPILE_TX
// following is only needed on receiver
void failsafeSave(void)
{
  accessEEPROM(2, true);
}

void failsafeLoad(void)
{
  if (!accessEEPROM(2, false)) {
    failsafePPM[0]=0xffff;
  }
}

void rxInitDefaults(bool save)
{
#if (BOARD_TYPE == 3)
  uint8_t i;
  rx_config.rx_type = RX_FLYTRON8CH;
  rx_config.pinMapping[0] = PINMAP_RSSI; // the CH0 on 8ch RX
  for (i = 1; i < 9; i++) {
    rx_config.pinMapping[i] = i - 1; // default to PWM out
  }
  rx_config.pinMapping[9] = PINMAP_ANALOG;
  rx_config.pinMapping[10] = PINMAP_ANALOG;
  rx_config.pinMapping[11] = PINMAP_RXD;
  rx_config.pinMapping[12] = PINMAP_TXD;
#elif (BOARD_TYPE == 5)
  uint8_t i;
  rx_config.rx_type = RX_OLRSNG4CH;
  for (i = 0; i < 4; i++) {
    rx_config.pinMapping[i] = i; // default to PWM out
  }
  rx_config.pinMapping[4] = 4;
  rx_config.pinMapping[5] = 5;
  rx_config.pinMapping[6] = PINMAP_RXD;
  rx_config.pinMapping[7] = PINMAP_TXD;
#elif (BOARD_TYPE == 7)
  rx_config.rx_type = RX_PTOWER;
  rx_config.pinMapping[0] = PINMAP_PPM;
  rx_config.pinMapping[1] = PINMAP_SDA;
  rx_config.pinMapping[2] = PINMAP_RSSI;
  rx_config.pinMapping[3] = PINMAP_SCL;
  // Skipping pinMapping[4] as it is NC
  rx_config.pinMapping[5] = PINMAP_LLIND;
  rx_config.pinMapping[6] = PINMAP_RXD;
  rx_config.pinMapping[7] = PINMAP_TXD;

#else
#error INVALID RX BOARD
#endif

  rx_config.flags = ALWAYS_BIND;
  rx_config.RSSIpwm = 255; // off
  rx_config.failsafeDelay = 10; //1s
  rx_config.ppmStopDelay = 0;
  rx_config.pwmStopDelay = 0;
  rx_config.beacon_frequency = DEFAULT_BEACON_FREQUENCY;
  rx_config.beacon_deadtime = DEFAULT_BEACON_DEADTIME;
  rx_config.beacon_interval = DEFAULT_BEACON_INTERVAL;
  rx_config.minsync = 3000;

  if (save) {
    accessEEPROM(0, true);
    failsafePPM[0] = 0xffff;
    failsafeSave();
  }
}

void rxReadEeprom()
{
  if (!accessEEPROM(0, false)) {
    rxInitDefaults(1);
  }
}

#endif

