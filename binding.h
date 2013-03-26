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

//  0 -- 4800bps, best range, 20Hz update rate
//  1 -- 9600bps, medium range, 40Hz update rate
//  2 -- 19200bps, medium range, 50Hz update rate + telemetry backlink
#define DEFAULT_DATARATE 1

// helpper macro for European PMR channels
#define EU_PMR_CH(x) (445993750L + 12500L * (x)) // valid for ch1-ch8

// helpper macro for US FRS channels 1-7
#define US_FRS_CH(x) (462537500L + 25000L * (x)) // valid for ch1-ch7

#define DEFAULT_BEACON_FREQUENCY 0 // disable beacon
#define DEFAULT_BEACON_DEADTIME 30 // time to wait until go into beacon mode (s)
#define DEFAULT_BEACON_INTERVAL 10 // interval between beacon transmits (s)

#define BINDING_POWER     0x00 // 1 mW
#define BINDING_VERSION   2

#define EEPROM_OFFSET     0x00

#define BIND_MAGIC (0xDEC1BE15 + BINDING_VERSION)
static uint8_t default_hop_list[] = {DEFAULT_HOPLIST};

// HW frequency limits
#ifdef RFMXX_868
#  define MIN_RFM_FREQUENCY 848000000
#  define MAX_RFM_FREQUENCY 488000000
#  define DEFAULT_CARRIER_FREQUENCY 868000000  // Hz  (ch 0)
#  define BINDING_FREQUENCY 868000000 // Hz
#else
#  define MIN_RFM_FREQUENCY 413000000
#  define MAX_RFM_FREQUENCY 463000000
#  define DEFAULT_CARRIER_FREQUENCY 435000000  // Hz  (ch 0)
#  define BINDING_FREQUENCY 435000000 // Hz
#endif


struct bind_data {
  uint8_t version;
  uint32_t rf_frequency;
  uint32_t rf_magic;
  uint8_t rf_power;
  uint8_t hopcount;
  uint8_t rf_channel_spacing;
  uint8_t hopchannel[8];
  uint8_t modem_params;
  uint32_t beacon_frequency;
  uint8_t beacon_interval;
  uint8_t beacon_deadtime;
} bind_data;

#define TELEMETRY_ENABLED 0x01

struct rfm22_modem_regs {
  uint32_t bps;
  uint32_t interval;
  uint8_t  flags;
  uint8_t  r_1c, r_1d, r_1e, r_20, r_21, r_22, r_23, r_24, r_25, r_2a, r_6e, r_6f, r_70, r_71, r_72;
} modem_params[] = {
  { 4800,  50000, 0x00, 0x1a, 0x40, 0x0a, 0xa1, 0x20, 0x4e, 0xa5, 0x00, 0x1b, 0x1e, 0x27, 0x52, 0x2c, 0x23, 0x30 },
  { 9600,  25000, 0x00, 0x05, 0x40, 0x0a, 0xa1, 0x20, 0x4e, 0xa5, 0x00, 0x20, 0x24, 0x4e, 0xa5, 0x2c, 0x23, 0x30 },
  { 19200, 25000, 0x01, 0x06, 0x40, 0x0a, 0xd0, 0x00, 0x9d, 0x49, 0x00, 0x7b, 0x28, 0x9d, 0x49, 0x2c, 0x23, 0x30 }
};

#define DATARATE_COUNT (sizeof(modem_params)/sizeof(modem_params[0]))

struct rfm22_modem_regs bind_params =
{ 9600,  25000, 0x00, 0x05, 0x40, 0x0a, 0xa1, 0x20, 0x4e, 0xa5, 0x00, 0x20, 0x24, 0x4e, 0xa5, 0x2c, 0x23, 0x30 };

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
    EEPROM.write(EEPROM_OFFSET + i, (BIND_MAGIC >> ((3-i) * 8))& 0xff);
  }

  for (uint8_t i = 0; i < sizeof(bind_data); i++) {
    EEPROM.write(EEPROM_OFFSET + 4 + i, *((uint8_t*)&bind_data + i));
  }
}

void bindInitDefaults(void)
{
  bind_data.version = BINDING_VERSION;
  bind_data.rf_power = DEFAULT_RF_POWER;
  bind_data.rf_frequency = DEFAULT_CARRIER_FREQUENCY;
  bind_data.rf_channel_spacing = DEFAULT_CHANNEL_SPACING;

  bind_data.rf_magic = DEFAULT_RF_MAGIC;

  bind_data.hopcount = sizeof(default_hop_list) / sizeof(default_hop_list[0]);

  for (uint8_t c = 0; c < 8; c++) {
    bind_data.hopchannel[c] = (c < bind_data.hopcount) ? default_hop_list[c] : 0;
  }

  bind_data.modem_params = DEFAULT_DATARATE;
  bind_data.beacon_frequency = DEFAULT_BEACON_FREQUENCY;
  bind_data.beacon_interval = DEFAULT_BEACON_INTERVAL;
  bind_data.beacon_deadtime = DEFAULT_BEACON_DEADTIME;
}

void bindRandomize(void)
{
  uint8_t c;

  bind_data.rf_magic = 0;
  for (c = 0; c < 4; c++) {
    bind_data.rf_magic = (bind_data.rf_magic << 8) + random(255);
  }

  for (c = 0; c < bind_data.hopcount; c++) {
again:
    uint8_t ch = random(50);

    // don't allow same channel twice
    for (uint8_t i = 0; i < c; i++) {
      if (bind_data.hopchannel[i] == ch) {
        goto again;
      }
    }

    bind_data.hopchannel[c] = ch;
  }
}


