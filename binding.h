// OpenLRSng binding

#define BINDING_POWER     0x00 // 1 mW
#define BINDING_FREQUENCY 435000000 // Hz
#define BINDING_VERSION   1

#define EEPROM_OFFSET     0x00

unsigned char bind_magic[4] = {0xDE,0xC1,0xBE,0x15};
static unsigned char default_hop_list[] = {DEFAULT_HOPLIST};

struct bind_data {
  unsigned char version;
  unsigned long rf_frequency;
  unsigned char rf_magic[4];
  unsigned char rf_power;
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
    *((unsigned char *)&bind_data + i) = EEPROM.read(EEPROM_OFFSET+4+i); 
  }
  if (bind_data.version != BINDING_VERSION) {
    return 0;
  }
  return 1;
}

void bind_write_eeprom() {
  for (unsigned char i=0; i<4; i++) {
    EEPROM.write(EEPROM_OFFSET+i, bind_magic[i]);
  }

  for (unsigned char i=0; i<sizeof(bind_data); i++) {
    EEPROM.write(EEPROM_OFFSET+4+i, *((unsigned char *)&bind_data + i)); 
  }
}

void bind_init_defaults() {
  bind_data.version = BINDING_VERSION;
  bind_data.rf_power = DEFAULT_RF_POWER;
  bind_data.rf_frequency = DEFAULT_CARRIER_FREQUENCY;
  for (unsigned char c=0; c<4; c++) {
    bind_data.rf_magic[c] = default_rf_magic[c];
  }
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

void print_bind_data() {

  Serial.print("V:   ");
  Serial.println(bind_data.version);
  Serial.print("RFF: ");
  Serial.println(bind_data.rf_frequency);
  Serial.print("RFM: ");
  Serial.print(bind_data.rf_magic[0],16);
  Serial.print(bind_data.rf_magic[1],16);
  Serial.print(bind_data.rf_magic[2],16);
  Serial.println(bind_data.rf_magic[3],16);
  Serial.print("RFP: ");
  Serial.println(bind_data.rf_power);
  Serial.print("RFH: ");
  Serial.println(bind_data.hopcount);
  Serial.print("CHA: ");
  for (unsigned char c = 0; c<8; c++) {
    Serial.print(bind_data.hopchannel[c],16); // max 8 channels
    Serial.print(":");
  }
  Serial.println();
  Serial.print("NCH: ");
  Serial.println(bind_data.rc_channels); //normally 8
  Serial.print("RMP: ");
  Serial.println(bind_data.modem_params); 
  Serial.print("BFR: ");
  Serial.println(bind_data.beacon_frequency);
  Serial.print("BIN: ");
  Serial.println(bind_data.beacon_interval);
  Serial.print("BDT: ");
  Serial.println(bind_data.beacon_deadtime);
}
  

