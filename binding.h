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

// FLAGS: 8bits
#define TELEMETRY_ENABLED 0x08
#define FRSKY_ENABLED     0x10
#define CHANNELS_4_4  1
#define CHANNELS_8    2
#define CHANNELS_8_4  3
#define CHANNELS_12   4
#define CHANNELS_12_4 5
#define CHANNELS_16   6


#define DEFAULT_FLAGS (CHANNELS_8 | TELEMETRY_ENABLED)

// helpper macro for European PMR channels
#define EU_PMR_CH(x) (445993750L + 12500L * (x)) // valid for ch1-ch8

// helpper macro for US FRS channels 1-7
#define US_FRS_CH(x) (462537500L + 25000L * (x)) // valid for ch1-ch7

#define DEFAULT_BEACON_FREQUENCY 0 // disable beacon
#define DEFAULT_BEACON_DEADTIME 30 // time to wait until go into beacon mode (s)
#define DEFAULT_BEACON_INTERVAL 10 // interval between beacon transmits (s)

#define MIN_DEADTIME 10
#define MAX_DEADTIME 65535

#define MIN_INTERVAL 5
#define MAX_INTERVAL 255

#define BINDING_POWER     0x00 // 1 mW
#define BINDING_VERSION   7

#define EEPROM_OFFSET     0x00
#define EEPROM_RX_OFFSET  0x40 // RX specific config struct

#define TELEMETRY_PACKETSIZE 9

#define BIND_MAGIC (0xDEC1BE15 + BINDING_VERSION)
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

#define MAXHOPS 24

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
  { 19200,0x06, 0x40, 0x0a, 0xd0, 0x00, 0x9d, 0x49, 0x00, 0x7b, 0x28, 0x9d, 0x49, 0x2c, 0x23, 0x30 }  // 25000 0x01
};

#define DATARATE_COUNT (sizeof(modem_params)/sizeof(modem_params[0]))

struct rfm22_modem_regs bind_params =
{ 9600, 0x05, 0x40, 0x0a, 0xa1, 0x20, 0x4e, 0xa5, 0x00, 0x20, 0x24, 0x4e, 0xa5, 0x2c, 0x23, 0x30 };

// Save EEPROM by writing just changed data
void myEEPROMwrite(int16_t addr, uint8_t data)
{
  if (data != EEPROM.read(addr)) {
    EEPROM.write(addr,data);
  }
}

int16_t bindReadEeprom()
{
  uint32_t temp = 0;
  for (uint8_t i = 0; i < 4; i++) {
    temp = (temp<<8) + EEPROM.read(EEPROM_OFFSET + i);
  }
  if (temp!=BIND_MAGIC) {
    return 0;
  }

  for (uint8_t i = 0; i < sizeof(bind_data); i++) {
    *((uint8_t*)&bind_data + i) = EEPROM.read(EEPROM_OFFSET + 4 + i);
  }

  if (bind_data.version != BINDING_VERSION) {
    return 0;
  }

  return 1;
}

void bindWriteEeprom(void)
{
  for (uint8_t i = 0; i < 4; i++) {
    myEEPROMwrite(EEPROM_OFFSET + i, (BIND_MAGIC >> ((3-i) * 8))& 0xff);
  }

  for (uint8_t i = 0; i < sizeof(bind_data); i++) {
    myEEPROMwrite(EEPROM_OFFSET + 4 + i, *((uint8_t*)&bind_data + i));
  }
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

void bindRandomize(void)
{
  uint8_t c;

  randomSeed(micros());

  bind_data.rf_magic = 0;
  for (c = 0; c < 4; c++) {
    bind_data.rf_magic = (bind_data.rf_magic << 8) + random(255);
  }

  for (c = 0; bind_data.hopchannel[c] != 0; c++) {
again:
    uint8_t ch = random(50) + 1;

    // don't allow same channel twice
    for (uint8_t i = 0; i < c; i++) {
      if (bind_data.hopchannel[i] == ch) {
        goto again;
      }
    }

    bind_data.hopchannel[c] = ch;
  }
}

#define FAILSAFE_NOPPM    0x01
#define FAILSAFE_NOPWM    0x02
#define PPM_MAX_8CH       0x04
#define ALWAYS_BIND       0x08

#define FAILSAFE_TIME(rxc) (((uint32_t)rxc.failsafe_delay) * 100000UL)

struct RX_config {
  uint8_t  rx_type; // RX type fillled in by RX, do not change
  uint8_t  pinMapping[13];
  uint8_t  flags;
  uint8_t  RSSIpwm;
  uint32_t beacon_frequency;
  uint16_t beacon_deadtime;
  uint8_t  beacon_interval;
  uint16_t minsync;
  uint8_t  failsafe_delay;
} rx_config;

#ifndef COMPILE_TX
// following is only needed on receiver

void rxInitDefaults()
{
  uint8_t i;
#if (BOARD_TYPE == 3)
  rx_config.rx_type = RX_FLYTRON8CH;
  rx_config.pinMapping[0] = PINMAP_RSSI; // the CH0 on 8ch RX
  for (i=1; i < 9; i++) {
    rx_config.pinMapping[i] = i-1; // default to PWM out
  }
  rx_config.pinMapping[9] = PINMAP_ANALOG;
  rx_config.pinMapping[10] = PINMAP_ANALOG;
  rx_config.pinMapping[11] = PINMAP_RXD;
  rx_config.pinMapping[12] = PINMAP_TXD;
#elif (BOARD_TYPE == 5)
  rx_config.rx_type = RX_OLRSNG4CH;
  for (i=0; i<4; i++) {
    rx_config.pinMapping[i]=i; // default to PWM out
  }
  rx_config.pinMapping[4] = PINMAP_RXD;
  rx_config.pinMapping[5] = PINMAP_TXD;
#else
#error INVALID RX BOARD
#endif

  rx_config.flags = ALWAYS_BIND;
  rx_config.RSSIpwm = 255; // off
  rx_config.failsafe_delay = 10; //1s
  rx_config.beacon_frequency = DEFAULT_BEACON_FREQUENCY;
  rx_config.beacon_deadtime = DEFAULT_BEACON_DEADTIME;
  rx_config.beacon_interval = DEFAULT_BEACON_INTERVAL;
  rx_config.minsync = 3000;
}

void rxWriteEeprom()
{
  for (uint8_t i = 0; i < 4; i++) {
    myEEPROMwrite(EEPROM_RX_OFFSET + i, (BIND_MAGIC >> ((3-i) * 8))& 0xff);
  }

  for (uint8_t i = 0; i < sizeof(rx_config); i++) {
    myEEPROMwrite(EEPROM_RX_OFFSET + 4 + i, *((uint8_t*)&rx_config + i));
  }
}

void rxReadEeprom()
{
  uint32_t temp = 0;

  for (uint8_t i = 0; i < 4; i++) {
    temp = (temp<<8) + EEPROM.read(EEPROM_RX_OFFSET + i);
  }

  if (temp!=BIND_MAGIC) {
    Serial.println("RXconf reinit");
    rxInitDefaults();
    rxWriteEeprom();
  } else {
    for (uint8_t i = 0; i < sizeof(rx_config); i++) {
      *((uint8_t*)&rx_config + i) = EEPROM.read(EEPROM_RX_OFFSET + 4 + i);
    }
    Serial.println("RXconf loaded");
  }
}

#endif

