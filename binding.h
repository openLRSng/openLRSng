// OpenLRSng binding

#define BINDING_POWER     0x00 // 1 mW
#define BINDING_FREQUENCY 435000000 // Hz
#define BINDING_VERSION   0

#define EEPROM_OFFSET 0x20

unsigned char bind_magic[4] = {0xDE,0xC1,0xBE,0x15};
static unsigned char default_hop_list[] = {DEFAULT_HOPLIST};

struct bind_data {
  unsigned char bind_version;
  unsigned long rf_frequency;
  unsigned char rf_magic[4];
  unsigned char hopcount;
  unsigned char hopchannel[8]; // max 8 channels
  unsigned char rc_channels; //normally 8
  unsigned char modem_params; 
  unsigned long beacon_frequency;
  unsigned char beacon_interval;
  unsigned char beacon_deadtime;
} bind_data;

int bind_read_eeprom() {
  for (unsigned char i=0; i<4; i++) {
    if (EEPROM.read(EEPROM_OFFSET+i) != bind_magic[i]) {
      return 0; // Fail
    }
  }
  for (unsigned char i=0; i<sizeof(bind_data); i++) {
    *((unsigned char *)bind_data + i) = EEPROM.read(EEPROM_OFFSET+4+i); 
  }
  if (bind_data.bind_version != BINDING_VERSION) {
    return 0;
  }
  return 1;
}

void bind_write_eeprom() {
  for (unsigned char i=0; i<4; i++) {
    EEPROM.write(EEPROM_OFFSET+i, bind_magic[i]);
  }

  for (unsigned char i=0; i<sizeof(bind_data); i++) {
    EEPROM.write(EEPROM_OFFSET+4+i, *((unsigned char *)bind_data + i)); 
  }
}

void bind_init_defaults() {
  bind_data.bind_version = BINDING_VERSION;
  bind_data.rf_frequency = DEFAULT_CARRIER_FREQUENCY;
  bind_data.rf_magic = {DEFAULT_HEADER};
  bind_data.hopcount = sizeof(default_hop_list)/sizeof(default_hop_list[0]);
  for (unsigned char c=0; c<8; c++) {
    bind_data.hopchannel[c] = (c<bind_data.hopcount) ? default_hop_list[c] : 0;
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
  

