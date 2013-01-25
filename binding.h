// OpenLRSng binding

#define BINDING_POWER     0x00 // 1 mW
#define BINDING_FREQUENCY 435000000 // Hz
#define BINDING_VERSION   1

#define EEPROM_OFFSET     0x00

uint8_t bind_magic[4] = {0xDE, 0xC1, 0xBE, 0x15};
static uint8_t default_hop_list[] = {DEFAULT_HOPLIST};

struct bind_data {
  uint8_t version;
  uint32_t rf_frequency;
  uint8_t rf_magic[4];
  uint8_t rf_power;
  uint8_t hopcount;
  uint8_t hopchannel[8]; // max 8 channels
  uint8_t rc_channels; //normally 8
  uint8_t modem_params;
  uint32_t beacon_frequency;
  uint8_t beacon_interval;
  uint8_t beacon_deadtime;
} bind_data;

struct rfm22_modem_regs {
  uint32_t bps;
  uint32_t interval;
  uint8_t  flags; // 0x01 = telemetry enabled
  uint8_t  r_1c, r_1d, r_1e, r_20, r_21, r_22, r_23, r_24, r_25, r_2a, r_6e, r_6f, r_70, r_71, r_72;
} modem_params[3] = {
  { 4800,  50000, 0x00, 0x1a, 0x40, 0x0a, 0xa1, 0x20, 0x4e, 0xa5, 0x00, 0x1b, 0x1e, 0x27, 0x52, 0x2c, 0x23, 0x30 },
  { 9600,  25000, 0x00, 0x05, 0x40, 0x0a, 0xa1, 0x20, 0x4e, 0xa5, 0x00, 0x20, 0x24, 0x4e, 0xa5, 0x2c, 0x23, 0x30 },
  { 19200, 20000, 0x01, 0x06, 0x40, 0x0a, 0xd0, 0x00, 0x9d, 0x49, 0x00, 0x7b, 0x28, 0x9d, 0x49, 0x2c, 0x23, 0x30 }
};

struct rfm22_modem_regs bind_params =
  { 9600,  25000, 0x00, 0x05, 0x40, 0x0a, 0xa1, 0x20, 0x4e, 0xa5, 0x00, 0x20, 0x24, 0x4e, 0xa5, 0x2c, 0x23, 0x30 };

int16_t bindReadEeprom()
{
  for (uint8_t i = 0; i < 4; i++) {
    if (EEPROM.read(EEPROM_OFFSET + i) != bind_magic[i]) {
      return 0; // Fail
    }
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
    EEPROM.write(EEPROM_OFFSET + i, bind_magic[i]);
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

  for (uint8_t c = 0; c < 4; c++) {
    bind_data.rf_magic[c] = default_rf_magic[c];
  }

  bind_data.hopcount = sizeof(default_hop_list) / sizeof(default_hop_list[0]);

  for (uint8_t c = 0; c < 8; c++) {
    bind_data.hopchannel[c] = (c < bind_data.hopcount) ? default_hop_list[c] : 0;
  }

  bind_data.rc_channels = 8;
  bind_data.modem_params = DEFAULT_DATARATE;
#ifdef DEFAULT_FAILSAFE_BEACON_ON
  bind_data.beacon_frequency = DEFAULT_BEACON_FREQUENCY;
  bind_data.beacon_interval = DEFAULT_BEACON_INTERVAL;
  bind_data.beacon_deadtime = DEFAULT_BEACON_DEADTIME;
#else
  bind_data.beacon_frequency = 0;
  bind_data.beacon_interval = 0;
  bind_data.beacon_deadtime = 0;
#endif
}

void bindRandomize(void)
{
  for (uint8_t c = 0; c < 4; c++) {
    bind_data.rf_magic[c] = random(255);
  }

  bind_data.hopcount = 6;

  for (uint8_t c = 0; c < bind_data.hopcount; c++) {
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

void bindPrint(void)
{

  Serial.print("1) Base frequency: ");
  Serial.println(bind_data.rf_frequency);
  Serial.print("2) RF magic:       ");
  Serial.print(bind_data.rf_magic[0], 16);
  Serial.print(bind_data.rf_magic[1], 16);
  Serial.print(bind_data.rf_magic[2], 16);
  Serial.println(bind_data.rf_magic[3], 16);
  Serial.print("3) RF power (0-7): ");
  Serial.println(bind_data.rf_power);
  Serial.print("4) Number of hops: ");
  Serial.println(bind_data.hopcount);
  Serial.print("5) Hop channels:   ");

  for (uint8_t c = 0; c < bind_data.hopcount; c++) {
    Serial.print(bind_data.hopchannel[c], 16);   // max 8 channels

    if (c + 1 != bind_data.hopcount) {
      Serial.print(":");
    }
  }

  Serial.println();
//  Serial.print("NCH: ");
//  Serial.println(bind_data.rc_channels); //normally 8
  Serial.print("6) Baudrate (0-2): ");
  Serial.println(bind_data.modem_params);
  Serial.print("7) Beacon freq.:   ");
  Serial.println(bind_data.beacon_frequency);
  Serial.print("8) Beacon Interval:");
  Serial.println(bind_data.beacon_interval);
  Serial.print("9) Beacon Deadtime:");
  Serial.println(bind_data.beacon_deadtime);
}


