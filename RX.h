/****************************************************
 * OpenLRSng receiver code
 ****************************************************/

uint8_t RF_channel = 0;

uint32_t time;
uint32_t last_pack_time = 0;
uint32_t last_rssi_time = 0;
uint32_t fs_time; // time when failsafe activated

uint32_t last_beacon;

uint8_t  RSSI_count = 0;
uint16_t RSSI_sum = 0;
uint8_t  last_rssi_value = 0;
uint8_t  smoothRSSI = 0;
uint16_t last_afcc_value = 0;

uint8_t  ppmCountter = 0;
uint16_t ppmSync = 40000;
uint8_t  ppmChannels = 8;

volatile uint8_t disablePWM = 0;
volatile uint8_t disablePPM = 0;

uint8_t firstpack = 0;
uint8_t lostpack = 0;

boolean willhop = 0, fs_saved = 0;

pinMask_t chToMask[PPM_CHANNELS];
pinMask_t clearMask;

void outputUp(uint8_t no)
{
  PORTB |= chToMask[no].B;
  PORTC |= chToMask[no].C;
  PORTD |= chToMask[no].D;
}

void outputDownAll()
{
  PORTB &= clearMask.B;
  PORTC &= clearMask.C;
  PORTD &= clearMask.D;
}

ISR(TIMER1_OVF_vect)
{
  if (ppmCountter >= ppmChannels) {
    ICR1 = ppmSync;
    ppmCountter = 0;
    ppmSync = 40000;

    while (TCNT1<32);
    outputDownAll();
    if (rx_config.pinMapping[PPM_OUTPUT] == PINMAP_PPM) {
      if (disablePPM) {
        TCCR1A &= ~((1 << COM1A1) | (1 << COM1A0));
      } else {
        TCCR1A |= ((1 << COM1A1) | (1 << COM1A0));
      }
    }
  } else {
    uint16_t ppmOut = servoBits2Us(PPM[ppmCountter]) * 2;
    ppmSync -= ppmOut;
    if (ppmSync < (rx_config.minsync * 2)) {
      ppmSync = rx_config.minsync * 2;
    }
    ICR1 = ppmOut;

    if ((rx_config.flags & PPM_MAX_8CH) && (ppmCountter == 8)) {
      TCCR1A &= ~((1 << COM1A1) | (1 << COM1A0));
    }

    while (TCNT1<32);
    outputDownAll();
    if (!disablePWM) {
      outputUp(ppmCountter);
    }

    ppmCountter++;
  }
}


void set_RSSI_output( uint8_t val )
{
  if (rx_config.RSSIpwm < 16) {
    cli();
    PPM[rx_config.RSSIpwm] = smoothRSSI << 2;
    sei();
  }
  if (rx_config.pinMapping[RSSI_OUTPUT] == PINMAP_RSSI) {
    if ((val == 0) || (val == 255)) {
      TCCR2A &= ~(1<<COM2B1); // disable RSSI PWM output
      digitalWrite(OUTPUT_PIN[RSSI_OUTPUT], (val == 0) ? LOW : HIGH);
    } else {
      OCR2B = val;
      TCCR2A |= (1<<COM2B1); // enable RSSI PWM output
    }
  }
}

void setupOutputs()
{
  uint8_t i;

  for (i = 0; i < OUTPUTS; i++) {
    chToMask[i].B = 0;
    chToMask[i].C = 0;
    chToMask[i].D = 0;
  }
  clearMask.B = 0xff;
  clearMask.C = 0xff;
  clearMask.D = 0xff;
  for (i = 0; i < OUTPUTS; i++) {
    if (rx_config.pinMapping[i] < PPM_CHANNELS) {
      chToMask[rx_config.pinMapping[i]].B |= OUTPUT_MASKS[i].B;
      chToMask[rx_config.pinMapping[i]].C |= OUTPUT_MASKS[i].C;
      chToMask[rx_config.pinMapping[i]].D |= OUTPUT_MASKS[i].D;
      clearMask.B &= ~OUTPUT_MASKS[i].B;
      clearMask.C &= ~OUTPUT_MASKS[i].C;
      clearMask.D &= ~OUTPUT_MASKS[i].D;
    }
  }

  for (i = 0; i < OUTPUTS; i++) {
    switch (rx_config.pinMapping[i]) {
    case PINMAP_ANALOG:
      pinMode(OUTPUT_PIN[i], INPUT);
      break;
    case PINMAP_TXD:
    case PINMAP_RXD:
    case PINMAP_SDA:
    case PINMAP_SCL:
      break; //ignore serial/I2C for now
    default:
      if (i == RXD_OUTPUT) {
        UCSR0B &= 0xEF; //disable serial RXD
      }
      if (i == TXD_OUTPUT) {
        UCSR0B &= 0xF7; //disable serial TXD
      }
      pinMode(OUTPUT_PIN[i], OUTPUT); //PPM,PWM,RSSI
      break;
    }
  }

  if (rx_config.pinMapping[PPM_OUTPUT] == PINMAP_PPM) {
    digitalWrite(OUTPUT_PIN[PPM_OUTPUT], HIGH);
    TCCR1A = (1 << WGM11) | (1 << COM1A1) | (1 << COM1A0);
  } else {
    TCCR1A = (1 << WGM11);
  }

  disablePWM = 0;
  disablePPM = 0;

  if (rx_config.pinMapping[RSSI_OUTPUT] == PINMAP_RSSI) {
    pinMode(OUTPUT_PIN[RSSI_OUTPUT], OUTPUT);
    digitalWrite(OUTPUT_PIN[RSSI_OUTPUT], LOW);
    TCCR2B = (1<<CS20);
    TCCR2A = (1<<WGM20);
  }

  TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
  ICR1 = 40000; // just initial value, will be constantly updated
  OCR1A = 600;  // 0.3ms pulse
  TIMSK1 |= (1 << TOIE1);

}

#define FAILSAFE_OFFSET 0x80

void save_failsafe_values(void)
{
  uint8_t ee_buf[20];

  packChannels(6, PPM, ee_buf);
  for (int16_t i = 0; i < 20; i++) {
    EEPROM.write(FAILSAFE_OFFSET + 4 +i, ee_buf[i]);
  }

  ee_buf[0]=0xFA;
  ee_buf[1]=0x11;
  ee_buf[2]=0x5A;
  ee_buf[3]=0xFE;
  for (int16_t i = 0; i < 4; i++) {
    EEPROM.write(FAILSAFE_OFFSET + i, ee_buf[i]);
  }
}

void load_failsafe_values(void)
{
  uint8_t ee_buf[20];

  for (int16_t i = 0; i < 4; i++) {
    ee_buf[i] = EEPROM.read(FAILSAFE_OFFSET + i);
  }

  if ((ee_buf[0]==0xFA) && (ee_buf[1]==0x11) && (ee_buf[2]==0x5A) && (ee_buf[3]==0xFE)) {
    for (int16_t i = 0; i < 20; i++) {
      ee_buf[i] = EEPROM.read(FAILSAFE_OFFSET + 4 +i);
    }
    cli();
    unpackChannels(6, PPM, ee_buf);
    sei();
  }
}

uint8_t bindReceive(uint32_t timeout)
{
  uint32_t start = millis();
  uint8_t  rxb;
  init_rfm(1);
  RF_Mode = Receive;
  to_rx_mode();
  Serial.println("Waiting bind\n");

  while ((!timeout) || ((millis() - start) < timeout)) {
    if (RF_Mode == Received) {
      Serial.println("Got pkt\n");
      spiSendAddress(0x7f);   // Send the package read command
      rxb=spiReadData();
      if (rxb=='b') {
        for (uint8_t i = 0; i < sizeof(bind_data); i++) {
          *(((uint8_t*)&bind_data) + i) = spiReadData();
        }

        if (bind_data.version == BINDING_VERSION) {
          Serial.println("data good\n");
          rxb='B';
          tx_packet(&rxb,1); // ACK that we got bound
          Green_LED_ON; //signal we got bound on LED:s
          return 1;
        }
      } else if ((rxb=='p') || (rxb=='i')) {
        uint8_t rxc_buf[sizeof(rx_config)+1];
        if (rxb=='p') {
          Serial.println(F("Sending RX config"));
          rxc_buf[0]='P';
          timeout=0;
        } else {
          Serial.println(F("Reinit RX config"));
          rxInitDefaults();
          rxWriteEeprom();
          rxc_buf[0]='I';
        }
        memcpy(rxc_buf+1, &rx_config, sizeof(rx_config));
        tx_packet(rxc_buf,sizeof(rx_config)+1);
      } else if (rxb=='u') {
        for (uint8_t i = 0; i < sizeof(rx_config); i++) {
          *(((uint8_t*)&rx_config) + i) = spiReadData();
        }
        rxWriteEeprom();
        rxb='U';
        tx_packet(&rxb,1); // ACK that we updated settings
      }
      RF_Mode = Receive;
      rx_reset();

    }
  }
  return 0;
}

uint8_t checkIfGrounded(uint8_t pin)
{
  int8_t ret = 0;

  pinMode(pin, INPUT);
  digitalWrite(pin, 1);
  delay(10);

  if (!digitalRead(pin)) {
    ret = 1;
  }

  digitalWrite(pin, 0);

  return ret;
}

int8_t checkIfConnected(uint8_t pin1, uint8_t pin2)
{
  int8_t ret = 0;
  pinMode(pin1, OUTPUT);
  digitalWrite(pin1, 1);
  digitalWrite(pin2, 1);
  delay(10);

  if (digitalRead(pin2)) {
    digitalWrite(pin1, 0);
    delay(10);

    if (!digitalRead(pin2)) {
      ret = 1;
    }
  }

  pinMode(pin1, INPUT);
  digitalWrite(pin1, 0);
  digitalWrite(pin2, 0);
  return ret;
}

uint8_t rx_buf[21]; // RX buffer (uplink)
// First byte of RX buf is
// MSB..LSB [1bit uplink seqno.] [1bit downlink seqno] [6bits type)
// type 0x00 normal servo, 0x01 failsafe set
// type 0x38..0x3f uplinkked serial data

uint8_t tx_buf[9]; // TX buffer (downlink)(type plus 8 x data)
// First byte is meta
// MSB..LSB [1 bit uplink seq] [1bit downlink seqno] [6b telemtype]
// 0x00 link info [RSSI] [AFCC]*2 etc...
// type 0x38-0x3f downlink serial data 1-8 bytes

#define SERIAL_BUFSIZE 32
uint8_t serial_buffer[SERIAL_BUFSIZE];
uint8_t serial_head;
uint8_t serial_tail;

uint8_t hopcount;

void setup()
{
  //LEDs
  pinMode(Green_LED, OUTPUT);
  pinMode(Red_LED, OUTPUT);

  setupSPI();

#ifdef SDN_pin
  pinMode(SDN_pin, OUTPUT);  //SDN
  digitalWrite(SDN_pin, 0);
#endif

  pinMode(0, INPUT);   // Serial Rx
  pinMode(1, OUTPUT);  // Serial Tx

  Serial.begin(SERIAL_BAUD_RATE);   //Serial Transmission
  rxReadEeprom();

  setupRfmInterrupt();

  sei();
  Red_LED_ON;

  if (checkIfConnected(OUTPUT_PIN[2],OUTPUT_PIN[3])) { // ch1 - ch2 --> force scannerMode
    scannerMode();
  }

  if (checkIfConnected(OUTPUT_PIN[0],OUTPUT_PIN[1]) || (!bindReadEeprom())) {
    Serial.print("EEPROM data not valid or bind jumpper set, forcing bind\n");

    if (bindReceive(0)) {
      bindWriteEeprom();
      Serial.println("Saved bind data to EEPROM\n");
      Green_LED_ON;
    }
  } else {
    if (rx_config.flags & ALWAYS_BIND) {
      if (bindReceive(500)) {
        bindWriteEeprom();
        Serial.println("Saved bind data to EEPROM\n");
        Green_LED_ON;
      }
    }
  }

  ppmChannels = getChannelCount(&bind_data);
  if (rx_config.RSSIpwm == ppmChannels) {
    ppmChannels+=1;
  }

  Serial.print("Entering normal mode with PPM ");
  Serial.print((rx_config.pinMapping[PPM_OUTPUT] == PINMAP_PPM)?"Enabled":"Disabled");
  Serial.print(" CHs=");
  Serial.println(ppmChannels);
  Serial.println(bind_data.flags,16);
  init_rfm(0);   // Configure the RFM22B's registers for normal operation
  RF_channel = 0;
  rfmSetChannel(bind_data.hopchannel[RF_channel]);

  // Count hopchannels as we need it later
  hopcount=0;
  while ((hopcount < MAXHOPS) && (bind_data.hopchannel[hopcount] != 0)) {
    hopcount++;
  }


  //################### RX SYNC AT STARTUP #################
  RF_Mode = Receive;
  to_rx_mode();
  
  if (bind_data.flags & TELEMETRY_ENABLED) {
    Serial.begin((bind_data.flags & FRSKY_ENABLED)? 9600 : bind_data.serial_baudrate);
    while (Serial.available()) {
      Serial.read();
    }
  }
  serial_head=0;
  serial_tail=0;
  firstpack = 0;
  last_pack_time = micros();

}

void checkSerial()
{
  while (Serial.available() && (((serial_tail + 1) % SERIAL_BUFSIZE) != serial_head)) {
    serial_buffer[serial_tail] = Serial.read();
    serial_tail = (serial_tail + 1) % SERIAL_BUFSIZE;
  }
}

//############ MAIN LOOP ##############
void loop()
{
  uint32_t time;

  if (spiReadRegister(0x0C) == 0) {     // detect the locked module and reboot
    Serial.println("RX hang");
    init_rfm(0);
    to_rx_mode();
  }

  checkSerial();

  time = micros();

  if (RF_Mode == Received) {   // RFM22B int16_t pin Enabled by received Data

    last_pack_time = micros(); // record last package time
    lostpack = 0;

    Red_LED_OFF;
    Green_LED_ON;

    spiSendAddress(0x7f);   // Send the package read command

    for (int16_t i = 0; i < getPacketSize(&bind_data); i++) {
      rx_buf[i] = spiReadData();
    }

    last_afcc_value = rfmGetAFCC();

    if ((rx_buf[0]&0x3e) == 0x00) {
      cli();
      unpackChannels(bind_data.flags & 7, PPM, rx_buf + 1);
      if (rx_config.RSSIpwm < 16) {
        PPM[rx_config.RSSIpwm] = smoothRSSI << 2;
      }
      sei();
      if (rx_buf[0] & 0x01) {
        if (!fs_saved) {
          save_failsafe_values();
          fs_saved = 1;
        }
      } else if (fs_saved) {
        fs_saved = 0;
      }
    } else {
      // something else than servo data...
      if ((rx_buf[0] & 0x38) == 0x38) {
        if ((rx_buf[0] ^ tx_buf[0]) & 0x80) {
          // We got new data... (not retransmission)
          uint8_t i;
          tx_buf[0] ^= 0x80; // signal that we got it
          for (i=0; i <= (rx_buf[0]&7);) {
            i++;
            Serial.write(rx_buf[i]);
          }
        }
      }
    }

    if (firstpack == 0) {
      firstpack = 1;
      setupOutputs();
    } else {
      disablePWM = 0;
      disablePPM = 0;
    }

    if (bind_data.flags & TELEMETRY_ENABLED) {
      if ((tx_buf[0] ^ rx_buf[0]) & 0x40) {
        // resend last message
      } else {
        tx_buf[0] &= 0xc0;
        tx_buf[0] ^= 0x40; // swap sequence as we have new data
        if (serial_head!=serial_tail) {
          uint8_t bytes=0;
          while ((bytes<8) && (serial_head!=serial_tail)) {
            bytes++;
            tx_buf[bytes]=serial_buffer[serial_head];
            serial_head=(serial_head + 1) % SERIAL_BUFSIZE;
          }
          tx_buf[0] |= (0x37 + bytes);
        } else {
          // tx_buf[0] lowest 6 bits left at 0
          tx_buf[1] = last_rssi_value;
          if (rx_config.pinMapping[ANALOG0_OUTPUT] == PINMAP_ANALOG) {
            tx_buf[2] = analogRead(OUTPUT_PIN[ANALOG0_OUTPUT])>>2;
          } else {
            tx_buf[2] = 0;
          }
          if (rx_config.pinMapping[ANALOG1_OUTPUT] == PINMAP_ANALOG) {
            tx_buf[3] = analogRead(OUTPUT_PIN[ANALOG1_OUTPUT])>>2;
          } else {
            tx_buf[3] = 0;
          }
          tx_buf[4] = (last_afcc_value >> 8);
          tx_buf[5] = last_afcc_value & 0xff;
        }
      }
      tx_packet_async(tx_buf, 9);

      while(!tx_done()) {
        checkSerial();
      }
    }

    RF_Mode = Receive;
    rx_reset();

    willhop = 1;

    Green_LED_OFF;
  }

  time = micros();

  // sample RSSI when packet is in the 'air'
  if ((lostpack < 2) && (last_rssi_time != last_pack_time) &&
      (time - last_pack_time) > (getInterval(&bind_data) - 1500)) {
    last_rssi_time = last_pack_time;
    last_rssi_value = rfmGetRSSI(); // Read the RSSI value
    RSSI_sum += last_rssi_value;    // tally up for average
    RSSI_count++;

    if (RSSI_count > 20) {
      RSSI_sum /= RSSI_count;
      RSSI_sum = map(constrain(RSSI_sum, 45, 200), 40, 200, 0, 255);
      smoothRSSI = (uint8_t)(((uint16_t)smoothRSSI * 6 + (uint16_t)RSSI_sum * 2)/8);
      set_RSSI_output(smoothRSSI);
      RSSI_sum = 0;
      RSSI_count = 0;
    }
  }

  if (firstpack) {
    if ((lostpack < hopcount) && ((time - last_pack_time) > (getInterval(&bind_data) + 1000))) {
      // we lost packet, hop to next channel
      willhop = 1;
      if (lostpack==0) {
        fs_time = time;
        last_beacon = 0;
      }
      lostpack++;
      last_pack_time += getInterval(&bind_data);
      willhop = 1;
      Red_LED_ON;
      if (smoothRSSI>30) {
        smoothRSSI-=30;
      } else {
        smoothRSSI=0;
      }
      set_RSSI_output(smoothRSSI);
    } else if ((lostpack == hopcount) && ((time - last_pack_time) > (getInterval(&bind_data) * hopcount))) {
      // hop slowly to allow resync with TX
      willhop = 1;
      smoothRSSI=0;
      set_RSSI_output(smoothRSSI);
      last_pack_time = time;
    }

    if (lostpack) {
      if ((fs_time) && ((time - fs_time) > FAILSAFE_TIME(rx_config))) {
        load_failsafe_values();
        if (rx_config.flags & FAILSAFE_NOPWM) {
          disablePWM = 1;
        }
        if (rx_config.flags & FAILSAFE_NOPPM) {
          disablePPM = 1;
        }
        fs_time=0;
        last_beacon = (time + ((uint32_t)rx_config.beacon_deadtime * 1000000UL)) | 1; //beacon activating...
      }

      if ((rx_config.beacon_frequency) && (last_beacon)) {
        if (((time - last_beacon) < 0x80000000) && // last beacon is future during deadtime
            (time - last_beacon) > ((uint32_t)rx_config.beacon_interval * 1000000UL)) {
          beacon_send();
          init_rfm(0);   // go back to normal RX
          rx_reset();
          last_beacon = micros() | 1; // avoid 0 in time
        }
      }
    }
  } else {
    // Waiting for first packet, hop slowly
    if ((time - last_pack_time) > (getInterval(&bind_data) * hopcount)) {
      last_pack_time = time;
      willhop = 1;
    }
  }

  if (willhop == 1) {
    RF_channel++;

    if ((RF_channel == MAXHOPS) || (bind_data.hopchannel[RF_channel] == 0)) {
      RF_channel = 0;
    }
    rfmSetChannel(bind_data.hopchannel[RF_channel]);
    willhop = 0;
  }

}
