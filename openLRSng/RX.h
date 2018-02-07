#ifndef _RX_H_
#define _RX_H_

/****************************************************
 * OpenLRSng receiver code
 ****************************************************/

#include <avr/eeprom.h>
#define STR(S) #S
#define XSTR(S) STR(S)

//#define DEBUG_DUMP_PPM
#define SERIAL_BUFSIZE 32

#if (F_CPU == 16000000)
#define PWM_MULTIPLIER 2
#define PPM_PULSELEN   600
#define PWM_DEJITTER   32
#define PPM_FRAMELEN   40000
#elif (F_CPU == 8000000)
#define PWM_MULTIPLIER 1
#define PPM_PULSELEN   300
#define PWM_DEJITTER   16
#define PPM_FRAMELEN   20000
#else
#error F_CPU not supported
#endif

volatile uint16_t nextICR1;
uint8_t RF_channel = 0;
uint32_t lastPacketTimeUs = 0;
uint32_t lastRSSITimeUs = 0;
uint32_t linkLossTimeMs;
uint32_t hopInterval = 0;
uint32_t hopTimeout = 0;
uint32_t hopTimeoutSlow = 0;
uint32_t RSSI_timeout = 0;
uint32_t pktTimeDelta = 0;
uint32_t nextBeaconTimeMs;
uint32_t timeUs = 0;
uint32_t timeMs= 0;
uint16_t beaconRSSIavg = 255;
uint8_t  RSSI_count = 0;
uint16_t RSSI_sum = 0;
uint8_t  lastRSSIvalue = 0;
uint8_t  smoothRSSI = 0;
uint8_t  compositeRSSI = 0;
uint16_t lastAFCCvalue = 0;

uint16_t linkQuality = 0;
uint8_t  linkQ;

uint8_t linkAcquired = 0;
uint8_t numberOfLostPackets = 0;

uint8_t  ppmCountter = 0;
uint16_t ppmSync = 40000;
uint8_t  ppmChannels = 8;

volatile uint8_t disablePWM = 0;
volatile uint8_t disablePPM = 0;
uint8_t failsafeActive = 0;

static const uint16_t switchThresholds[3] = { 178, 500, 844 };

uint16_t failsafePPM[PPM_CHANNELS];
uint8_t serial_buffer[SERIAL_BUFSIZE];
uint8_t serial_head;
uint8_t serial_tail;

uint8_t hopcount;
bool willhop = 0;
bool fs_saved = 0;

pinMask_t chToMask[PPM_CHANNELS];
pinMask_t clearMask;

uint8_t rx_buf[21]; // RX buffer (uplink)
// First byte of RX buffer is metadata
// MSB..LSB [1bit uplink seqno.] [1bit downlink seqno] [6bits type]
// type 0x38..0x3f uplinked serial data
// type 0x00 normal servo, 0x01 failsafe set

uint8_t tx_buf[9]; // TX buffer (downlink)(type plus 8 x data)
// First byte of TX buffer is metadata
// MSB..LSB [1 bit uplink seq] [1bit downlink seqno] [6b telemtype]
// 0x00 link info [RSSI] [AFCC]*2 etc...
// type 0x38-0x3f downlink serial data 1-8 bytes

void set_PPM_rssi(void);
void set_RSSI_output(void);
void updateSwitches(void);
void failsafeApply(void);
void setupOutputs(void);
void checkSerial(void);
void outputUp(uint8_t no);
void outputDownAll(void);
void processPacketRC(void);
void processPacketData(void);
void handlePacketTelem(void);
void handlePacketRX(void);
void checkLinkState(void);
void updateHopChannel(void);
void updateSerialRC(void);
void handleFailsafe(void);
void checkFailsafeButton(void);
void handleBeacon(void);
void checkRSSI(void);
void updateLBeep(bool packetLost);
uint8_t bindReceive(uint32_t timeout);
uint8_t checkIfConnected(uint8_t pin1, uint8_t pin2);
uint16_t RSSI2Bits(uint8_t rssi);

#ifdef DEBUG_DUMP_PPM
void dumpPPM(void);
#endif

#ifdef CONFIGURATOR
void checkBinaryMode(void);
#endif

void setup(void)
{
  watchdogConfig(WATCHDOG_OFF);

  //LEDs
  pinMode(Green_LED, OUTPUT);
  pinMode(Red_LED, OUTPUT);

  setupSPI();

#ifdef SDN_pin
  pinMode(SDN_pin, OUTPUT);  //SDN
  digitalWrite(SDN_pin, 0);
#endif

  pinMode(0, INPUT);   // Serial RX
  pinMode(1, OUTPUT);  // Serial TX

  Serial.begin(115200);
  rxReadEeprom();
  failsafeLoad();

  Serial.print("OpenLRSng RX starting ");
  printVersion(version);
  Serial.println(" on HW " XSTR(BOARD_TYPE));

#ifdef CONFIGURATOR
  delay(100);
  checkBinaryMode();
#endif

  setupRfmInterrupt();

  sei();
  Red_LED_ON;

  if (checkIfConnected(OUTPUT_PIN[0], OUTPUT_PIN[1]) || (!bindReadEeprom())) {
    Serial.print("EEPROM data not valid or bind jumpper set, forcing bind\n");
    if (bindReceive(0)) {
      bindWriteEeprom();
      Serial.println("Saved bind data to EEPROM\n");
      Green_LED_ON;
    }
    setupOutputs();
  } else {
    setupOutputs();

    if ((rx_config.pinMapping[SDA_OUTPUT] != PINMAP_SDA) ||
        (rx_config.pinMapping[SCL_OUTPUT] != PINMAP_SCL)) {
#ifdef ENABLE_SLAVE_MODE
      rx_config.flags &= ~SLAVE_MODE;
#endif
    }

    if ((rx_config.flags & ALWAYS_BIND) && (!(rx_config.flags & SLAVE_MODE))) {
      if (bindReceive(500)) {
        bindWriteEeprom();
        Serial.println("Saved bind data to EEPROM\n");
        setupOutputs(); // parameters may have changed
        Green_LED_ON;
      }
    }
  }

  if ((rx_config.pinMapping[SDA_OUTPUT] == PINMAP_SDA) &&
      (rx_config.pinMapping[SCL_OUTPUT] == PINMAP_SCL)) {
    myI2C_init(1);
#ifdef ENABLE_SLAVE_MODE
    if (rx_config.flags & SLAVE_MODE) {
      Serial.println("I am slave");
      slaveLoop();
    } else {
      uint8_t ret,buf;
      delay(20);
      ret = myI2C_readFrom(32, &buf, 1, MYI2C_WAIT);
      if (ret==0) {
        slaveState = 1;
      }
    }
#endif
  }

  Serial.print("Entering normal mode");
  watchdogConfig(WATCHDOG_2S);

  // Count hopchannels as we need it later
  hopcount=0;
  while ((hopcount < MAXHOPS) && (bind_data.hopchannel[hopcount] != 0)) {
    hopcount++;
  }

  //################### RX SYNC AT STARTUP #################
  init_rfm(0);   // Configure the RFM22B's registers for normal operation
  RF_channel = 0;
  setHopChannel(RF_channel);
  rx_reset();

#ifdef ENABLE_SLAVE_MODE
  checkSlaveState();
#endif

  if ((rx_config.pinMapping[TXD_OUTPUT] == PINMAP_SPKTRM) ||
      (rx_config.pinMapping[TXD_OUTPUT] == PINMAP_SUMD)) {
    Serial.begin(115200);
  } else if (rx_config.pinMapping[TXD_OUTPUT] == PINMAP_SBUS) {
    Serial.begin(100000, SERIAL_8E2); // Even parity, two stop bits
  } else if ((bind_data.flags & TELEMETRY_MASK) == TELEMETRY_FRSKY) {
    Serial.begin(9600);
  } else {
    if (bind_data.serial_baudrate < 10) {
      Serial.begin(9600);
    } else {
      Serial.begin(bind_data.serial_baudrate);
    }
  }

  while (Serial.available()) {
    Serial.read();
  }

  if (rx_config.pinMapping[RXD_OUTPUT] != PINMAP_RXD) {
    UCSR0B &= 0xEF; //disable serial RXD
  }

  if ((rx_config.pinMapping[TXD_OUTPUT] != PINMAP_TXD) &&
      (rx_config.pinMapping[TXD_OUTPUT] != PINMAP_SUMD) &&
      (rx_config.pinMapping[TXD_OUTPUT] != PINMAP_SBUS) &&
      (rx_config.pinMapping[TXD_OUTPUT] != PINMAP_SPKTRM)) {
    UCSR0B &= 0xF7; //disable serial TXD
  }

  serial_head = 0;
  serial_tail = 0;
  linkAcquired = 0;
  hopInterval = getInterval(&bind_data);
  hopTimeout = hopInterval + 1000;
  hopTimeoutSlow = hopInterval * hopcount;
  RSSI_timeout = hopInterval - 1500;
  lastPacketTimeUs = micros();
}

ISR(TIMER1_OVF_vect)
{
  if (ppmCountter < ppmChannels) {
    ICR1 = nextICR1;
    nextICR1 = servoBits2Us(PPM[ppmCountter]) * PWM_MULTIPLIER;
    ppmSync -= nextICR1;
    if (ppmSync < (rx_config.minsync * PWM_MULTIPLIER)) {
      ppmSync = rx_config.minsync * PWM_MULTIPLIER;
    }
    if ((disablePPM) || ((rx_config.flags & PPM_MAX_8CH) && (ppmCountter >= 8))) {
#ifdef USE_OCR1B
      OCR1B = 65535; //do not generate a pulse
#else
      OCR1A = 65535; //do not generate a pulse
#endif
    } else {
#ifdef USE_OCR1B
      OCR1B = nextICR1 - PPM_PULSELEN;
#else
      OCR1A = nextICR1 - PPM_PULSELEN;
#endif
    }

    while (TCNT1 < PWM_DEJITTER);
    outputDownAll();
    if ((!disablePWM) && (ppmCountter > 0)) {
      outputUp(ppmCountter - 1);
    }
    ppmCountter++;
  } else {
    ICR1 = nextICR1;
    nextICR1 = ppmSync;
    if (disablePPM) {
#ifdef USE_OCR1B
      OCR1B = 65535; //do not generate a pulse
#else
      OCR1A = 65535; //do not generate a pulse
#endif
    } else {
#ifdef USE_OCR1B
      OCR1B = nextICR1 - PPM_PULSELEN;
#else
      OCR1A = nextICR1 - PPM_PULSELEN;
#endif
    }
    ppmSync = PPM_FRAMELEN;

    while (TCNT1 < PWM_DEJITTER);
    outputDownAll();
    if (!disablePWM) {
      outputUp(ppmChannels - 1);
    }
    ppmCountter = 0 ;
  }
}

//############ MAIN LOOP ##############
void loop(void)
{
  watchdogReset();
  check_module();
  checkSerial();

  handlePacketRX();

  timeUs = micros();
  timeMs = millis();
  pktTimeDelta = (timeUs - lastPacketTimeUs);  

  checkRSSI();
  
  if (linkAcquired) {
    // check RC link status after initial 'lock'
    checkLinkState();
  } else if (pktTimeDelta > hopTimeoutSlow) {
    // Still waiting for first packet, so hop slowly
    lastPacketTimeUs = timeUs;
    willhop = 1;
  }

  updateSerialRC();
  updateHopChannel();

#ifdef ENABLE_SLAVE_MODE
  checkSlaveStatus();
#endif
}

void updateSerialRC(void)
{
  if (!disablePPM) {
    if (rx_config.pinMapping[TXD_OUTPUT] == PINMAP_SPKTRM) {
      sendSpektrumFrame();
    } else if (rx_config.pinMapping[TXD_OUTPUT] == PINMAP_SBUS) {
      sendSBUSFrame(failsafeActive, numberOfLostPackets);
    } else if (rx_config.pinMapping[TXD_OUTPUT] == PINMAP_SUMD) {
      sendSUMDFrame(failsafeActive);
    }
  }
}

void handlePacketRX(void)
{
  uint8_t slaveReceived = 0;

#ifdef ENABLE_SLAVE_MODE
  if (readSlaveState() == 5) {
    slaveReceived = 1;
  }

retry:
#endif

  if ((RF_Mode == RECEIVED) || (slaveReceived)) {
    uint32_t timeTemp = micros();
#ifdef ENABLE_SLAVE_MODE
    if (RF_Mode != RECEIVED) {
      uint8_t ret;
      uint8_t slave_buf[22];
      ret = myI2C_readFrom(32, slave_buf, getPacketSize(&bind_data) + 1, MYI2C_WAIT);
      if (ret) {
        slaveState = 255;
        slaveFailedMs = millis();
        slaveReceived = 0;
        goto retry; //slave failed when reading packet...
      } else {
        memcpy(rx_buf, (slave_buf + 1), getPacketSize(&bind_data));
      }
    } else {
#endif

      rfmGetPacket(rx_buf, getPacketSize(&bind_data));
      lastAFCCvalue = rfmGetAFCC();
      Green_LED_ON;

      lastPacketTimeUs = timeTemp;
      numberOfLostPackets = 0;
      nextBeaconTimeMs = 0;
      linkQuality <<= 1;
      linkQuality |= 1;
      Red_LED_OFF;
      updateLBeep(false);

#if defined(ENABLE_SLAVE_MODE) && defined(SLAVE_STATISTICS)
      if (readSlaveState() == 5) {
        if (RF_Mode == RECEIVED) {
          rxBoth++;
        } else {
          rxSlave++;
        }
      } else {
        rxMaster++;
      }
#endif

      if ((rx_buf[0] & 0x3e) == 0x00) {
        processPacketRC();
      } else if ((rx_buf[0] & 0x38) == 0x38) {
        processPacketData();
      }

      linkAcquired = 1;
      failsafeActive = 0;
      disablePWM = 0;
      disablePPM = 0;

      if (bind_data.flags & TELEMETRY_MASK) {
        handlePacketTelem();
      }

#ifdef TEST_HALT_RX_BY_CH2
      if (PPM[1]>1013) {
        fatalBlink(3);
      }
#endif

      updateSwitches();
      willhop = 1;
      rx_reset();
      Green_LED_OFF;

#ifdef ENABLE_SLAVE_MODE
    }
#endif
  }
}

void processPacketRC(void)
{
  // process RC data packet
  cli();
  unpackChannels(bind_data.flags & 7, PPM, rx_buf + 1);
  set_PPM_rssi();
  sei();
#ifdef DEBUG_DUMP_PPM
  dumpPPM();
#endif
  checkFailsafeButton();
}

void processPacketData(void)
{
  // process serial / data up-link packet 
  if ((rx_buf[0] ^ tx_buf[0]) & 0x80) {
    // We got new data... (not retransmission)
    uint8_t i;
    tx_buf[0] ^= 0x80; // signal that we got it
    if (rx_config.pinMapping[TXD_OUTPUT] == PINMAP_TXD) {
      for (i = 0; i <= (rx_buf[0] & 7);) {
        i++;
        Serial.write(rx_buf[i]);
      }
    }
  }
}

void handlePacketTelem(void)
{
  if ((tx_buf[0] ^ rx_buf[0]) & 0x40) {
    // resend last message
    // does this currently do anything? -csurf
  } else {
    tx_buf[0] &= 0xc0;
    tx_buf[0] ^= 0x40; // swap sequence as we have new data
    if (serial_head != serial_tail) {
      uint8_t bytes = 0;
      while ((bytes < 8) && (serial_head != serial_tail)) {
        bytes++;
        tx_buf[bytes] = serial_buffer[serial_head];
        serial_head = (serial_head + 1) % SERIAL_BUFSIZE;
      }
      tx_buf[0] |= (0x37 + bytes);
    } else {
      // tx_buf[0] lowest 6 bits left at 0
      tx_buf[1] = lastRSSIvalue;

      if (rx_config.pinMapping[ANALOG0_OUTPUT] == PINMAP_ANALOG) {
        tx_buf[2] = analogRead(OUTPUT_PIN[ANALOG0_OUTPUT]) >> 2;
#ifdef ANALOG0_OUTPUT_ALT
      } else if (rx_config.pinMapping[ANALOG0_OUTPUT_ALT] == PINMAP_ANALOG) {
        tx_buf[2] = analogRead(OUTPUT_PIN[ANALOG0_OUTPUT_ALT]) >> 2;
#endif
      } else {
        tx_buf[2] = 0;
      }

      if (rx_config.pinMapping[ANALOG1_OUTPUT] == PINMAP_ANALOG) {
        tx_buf[3] = analogRead(OUTPUT_PIN[ANALOG1_OUTPUT]) >> 2;
#ifdef ANALOG1_OUTPUT_ALT
      } else if (rx_config.pinMapping[ANALOG1_OUTPUT_ALT] == PINMAP_ANALOG) {
        tx_buf[3] = analogRead(OUTPUT_PIN[ANALOG1_OUTPUT_ALT]) >> 2;
#endif
      } else {
        tx_buf[3] = 0;
      }
      tx_buf[4] = (lastAFCCvalue >> 8);
      tx_buf[5] = lastAFCCvalue & 0xff;
      tx_buf[6] = countSetBits(linkQuality & 0x7fff);
    }
  }
#ifdef TEST_NO_ACK_BY_CH1
  if (PPM[0]<900) {
    tx_packet_async(tx_buf, 9);
    while(!tx_done()) {
      checkSerial();
    }
  }
#else
  tx_packet_async(tx_buf, 9);
  while(!tx_done()) {
    checkSerial();
  }
#endif
}

void outputUp(uint8_t no)
{
  PORTB |= chToMask[no].B;
  PORTC |= chToMask[no].C;
  PORTD |= chToMask[no].D;
}

void outputDownAll(void)
{
  PORTB &= clearMask.B;
  PORTC &= clearMask.C;
  PORTD &= clearMask.D;
}

uint16_t RSSI2Bits(uint8_t rssi)
{
  uint16_t ret = (uint16_t) rssi << 2;
  if (ret < 12) {
    ret = 12;
  } else if (ret > 1012) {
    ret = 1012;
  }
  return ret;
}

void set_PPM_rssi(void)
{
  if (rx_config.RSSIpwm < 48) {
    uint8_t out;
    switch (rx_config.RSSIpwm & 0x30) {
    case 0x00:
      out = compositeRSSI;
      break;
    case 0x10:
      out = (linkQ << 4);
      break;
    default:
      out = smoothRSSI;
      break;
    }
    PPM[rx_config.RSSIpwm & 0x0f] = RSSI2Bits(out);
  } else if (rx_config.RSSIpwm < 63) {
    PPM[(rx_config.RSSIpwm & 0x0f)] = RSSI2Bits(linkQ << 4);
    PPM[(rx_config.RSSIpwm & 0x0f)+1] = RSSI2Bits(smoothRSSI);
  }
}

void set_RSSI_output(void)
{
  linkQ = countSetBits(linkQuality & 0x7fff);
  if (linkQ == 15) {
    // RSSI 0 - 255 mapped to 192 - ((255>>2)+192) == 192-255
    compositeRSSI = (smoothRSSI >> 1) + 128;
  } else {
    // linkquality gives 0 to 14*9 == 126
    compositeRSSI = linkQ * 9;
  }

  cli();
  set_PPM_rssi();
  sei();

  if (rx_config.pinMapping[RSSI_OUTPUT] == PINMAP_RSSI) {
    if ((compositeRSSI == 0) || (compositeRSSI == 255)) {
      TCCR2A &= ~(1 << COM2B1); // disable RSSI PWM output
      digitalWrite(OUTPUT_PIN[RSSI_OUTPUT], (compositeRSSI == 0) ? LOW : HIGH);
    } else {
      OCR2B = compositeRSSI;
      TCCR2A |= (1 << COM2B1); // enable RSSI PWM output
    }
  }
}

void updateSwitches(void)
{
  uint8_t i;
  for (i = 0; i < OUTPUTS; i++) {
    uint8_t map = rx_config.pinMapping[i];
    if ((map & 0xf0) == 0x10) { // 16-31
      digitalWrite(OUTPUT_PIN[i], (PPM[map & 0x0f] > switchThresholds[i%3]) ? HIGH : LOW);
    }
  }
}

void failsafeApply(void)
{
  for (uint8_t i = 0; i < PPM_CHANNELS; i++) {
    if ((i == (rx_config.RSSIpwm & 0x0f)) ||
        ((i == (rx_config.RSSIpwm & 0x0f) + 1) && (rx_config.RSSIpwm > 47))) {
      continue;
    }
    if (failsafePPM[i] & 0xfff) {
      cli();
      PPM[i] = servoUs2Bits(failsafePPM[i] & 0xfff);
      sei();
    }
    updateSwitches();
  }
}

void setupOutputs(void)
{
  uint8_t i;

  ppmChannels = getChannelCount(&bind_data);
  if ((rx_config.RSSIpwm & 0x0f) == ppmChannels) {
    ppmChannels += 1;
  }
  if ((rx_config.RSSIpwm > 47) && (rx_config.RSSIpwm < 63) &&
      ((rx_config.RSSIpwm & 0x0f) == ppmChannels-1)) {
    ppmChannels += 1;
  }

  if (ppmChannels > 16) {
    ppmChannels=16;
  }

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
      pinMode(OUTPUT_PIN[i], OUTPUT); //PPM,PWM,RSSI,LBEEP
      break;
    }
  }

  if (rx_config.pinMapping[PPM_OUTPUT] == PINMAP_PPM) {
    digitalWrite(OUTPUT_PIN[PPM_OUTPUT], HIGH);
#ifdef USE_OCR1B
    TCCR1A = (1 << WGM11) | (1 << COM1B1);
    if(rx_config.flags & INVERTED_PPMOUT) {
      TCCR1A |= (1 << COM1B0);
    }
#else
    TCCR1A = (1 << WGM11) | (1 << COM1A1);
    if(rx_config.flags & INVERTED_PPMOUT) {
      TCCR1A |= (1 << COM1A0);
    }
#endif
  } else {
    TCCR1A = (1 << WGM11);
  }

  disablePWM = 1;
  disablePPM = 1;

  if ((rx_config.pinMapping[RSSI_OUTPUT] == PINMAP_RSSI) ||
      (rx_config.pinMapping[RSSI_OUTPUT] == PINMAP_LBEEP)) {
    pinMode(OUTPUT_PIN[RSSI_OUTPUT], OUTPUT);
    digitalWrite(OUTPUT_PIN[RSSI_OUTPUT], LOW);
    if (rx_config.pinMapping[RSSI_OUTPUT] == PINMAP_RSSI) {
      TCCR2A = (1 << WGM20);
      TCCR2B = (1 << CS20);
    } else { // LBEEP
      TCCR2A = (1 << WGM21); // mode=CTC
#if (F_CPU == 16000000)
      TCCR2B = (1 << CS22) | (1 << CS20); // prescaler = 128
#elif (F_CPU == 8000000)
      TCCR2B = (1 << CS22); // prescaler = 64
#else
#error F_CPU not supported
#endif
      OCR2A = 62; // 1KHz
    }
  }

  TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
#ifdef USE_OCR1B
  OCR1B = 65535;  // no pulse =)
#else
  OCR1A = 65535;  // no pulse =)
#endif
  ICR1 = 2000; // just initial value, will be constantly updated
  ppmSync = PPM_FRAMELEN;
  nextICR1 = PPM_FRAMELEN;
  ppmCountter = 0;
  TIMSK1 |= (1 << TOIE1);

  if ((rx_config.flags & IMMEDIATE_OUTPUT) && (failsafePPM[0] & 0xfff)) {
    failsafeApply();
    disablePPM=0;
    disablePWM=0;
  }
}

void updateLBeep(bool packetLost)
{
#if defined(LLIND_OUTPUT)
  if (rx_config.pinMapping[LLIND_OUTPUT] == PINMAP_LLIND) {
    digitalWrite(OUTPUT_PIN[LLIND_OUTPUT], packetLost);
  }
#endif
  if (rx_config.pinMapping[RSSI_OUTPUT] == PINMAP_LBEEP) {
    if (packetLost) {
      TCCR2A |= (1 << COM2B0); // enable tone
    } else {
      TCCR2A &= ~(1 << COM2B0); // disable tone
    }
  }
}

void checkRSSI(void)
{
  // sample RSSI when packet is in the 'air'
  if ((numberOfLostPackets < 2) && (lastRSSITimeUs != lastPacketTimeUs) && (pktTimeDelta > RSSI_timeout)) {
    lastRSSITimeUs = lastPacketTimeUs;
    lastRSSIvalue = rfmGetRSSI(); // Read the RSSI value
    RSSI_sum += lastRSSIvalue;    // tally up for average
    RSSI_count++;

    if (RSSI_count > 8) {
      RSSI_sum /= RSSI_count;
      smoothRSSI = (((uint16_t)smoothRSSI * 3 + (uint16_t)RSSI_sum * 1) / 4);
      set_RSSI_output();
      RSSI_sum = 0;
      RSSI_count = 0;
    }
  }
}

void checkLinkState(void)
{
  if ((numberOfLostPackets < hopcount) && (pktTimeDelta > hopTimeout)) {
    // we lost a packet, so hop to next channel
    linkQuality <<= 1;
    willhop = 1;
    if (numberOfLostPackets == 0) {
      linkLossTimeMs = timeMs;
      nextBeaconTimeMs = 0;
    }
    numberOfLostPackets++;
    lastPacketTimeUs += hopInterval;
    willhop = 1;
    Red_LED_ON;
    updateLBeep(true);
    set_RSSI_output();
  } else if ((numberOfLostPackets == hopcount) && (pktTimeDelta > hopTimeoutSlow)) {
    // hop slowly to allow re-sync with TX
    linkQuality = 0;
    willhop = 1;
    smoothRSSI = 0;
    set_RSSI_output();
    lastPacketTimeUs = timeUs;
  }

  if (numberOfLostPackets) {
    handleFailsafe();
  }
}

void checkFailsafeButton(void)
{
  if (rx_buf[0] & 0x01) {
    if (!fs_saved) {
      for (int16_t i = 0; i < PPM_CHANNELS; i++) {
        if (!(failsafePPM[i] & 0x1000)) {
          failsafePPM[i] = servoBits2Us(PPM[i]);
        }
      }
      failsafeSave();
      fs_saved = 1;
    }
  } else if (fs_saved) {
    fs_saved = 0;
  }
}

void handleFailsafe(void)
{
  uint32_t timeMsDelta = (timeMs - linkLossTimeMs);
  if (rx_config.failsafeDelay && (!failsafeActive) && (timeMsDelta > delayInMs(rx_config.failsafeDelay))) {
    failsafeActive = 1;
    failsafeApply();
    nextBeaconTimeMs = (timeMs + delayInMsLong(rx_config.beacon_deadtime)) | 1; //beacon activating...
  }
  if (rx_config.pwmStopDelay && (!disablePWM) && (timeMsDelta > delayInMs(rx_config.pwmStopDelay))) {
    disablePWM = 1;
  }
  if (rx_config.ppmStopDelay && (!disablePPM) && (timeMsDelta > delayInMs(rx_config.ppmStopDelay))) {
    disablePPM = 1;
  }
  if ((rx_config.beacon_frequency) && (nextBeaconTimeMs) && ((timeMs - nextBeaconTimeMs) < 0x80000000)) {
    beacon_send((rx_config.flags & STATIC_BEACON));
    rx_reset();
    nextBeaconTimeMs = (millis() +  (1000UL * rx_config.beacon_interval)) | 1; // avoid 0 in time
  }
}

void updateHopChannel(void)
{
  if (willhop == 1) {
    RF_channel++;
    if ((RF_channel == MAXHOPS) || (bind_data.hopchannel[RF_channel] == 0)) {
      RF_channel = 0;
    }
    handleBeacon();
    setHopChannel(RF_channel);
#ifdef ENABLE_SLAVE_MODE
    slaveHop();
#endif
    willhop = 0;
  }
}

uint8_t bindReceive(uint32_t timeout)
{
  uint32_t start = millis();
  uint8_t rxc_buf[33];
  uint8_t len;
  init_rfm(1);
  rx_reset();
  Serial.println("Waiting bind\n");
  while ((!timeout) || ((millis() - start) < timeout)) {
    if (RF_Mode == RECEIVED) {
      Serial.println("Got pkt");
      len=rfmGetPacketLength();
      rfmGetPacket(rxc_buf, len);
      switch((char) rxc_buf[0]) {
      case 'b': { // GET bind_data
        memcpy(&bind_data, (rxc_buf + 1), sizeof(bind_data));
        if (bind_data.version == BINDING_VERSION) {
          Serial.println("data good");
          rxc_buf[0] = 'B';
          tx_packet(rxc_buf, 1); // ACK that we got bound
          Green_LED_ON; //signal we got bound on LED:s
          return 1;
        }
      }
      break;
      case 'p': // GET rx_config
      case 'i': { // GET & reset rx_config
        if (rxc_buf[0] == 'p') {
          rxc_buf[0] = 'P';
          timeout = 0;
        } else {
          rxInitDefaults(1);
          rxc_buf[0] = 'I';
        }
        if (watchdogUsed) {
          rx_config.flags |= WATCHDOG_USED;
        } else {
          rx_config.flags &=~ WATCHDOG_USED;
        }
        memcpy((rxc_buf + 1), &rx_config, sizeof(rx_config));
        tx_packet(rxc_buf, sizeof(rx_config) + 1);
      }
      break;
      case 't': { // GET version & outputs/pins info
        timeout = 0;
        rxc_buf[0] = 'T';
        rxc_buf[1] = (version >> 8);
        rxc_buf[2] = (version & 0xff);
        rxc_buf[3] = OUTPUTS;
        rxc_buf[4] = sizeof(rxSpecialPins) / sizeof(rxSpecialPins[0]);
        memcpy((rxc_buf + 5), &rxSpecialPins, sizeof(rxSpecialPins));
        tx_packet(rxc_buf, sizeof(rxSpecialPins) + 5);
      }
      break;
      case 'u': { // SET save rx_config to eeprom
        memcpy(&rx_config, (rxc_buf + 1), sizeof(rx_config));
        accessEEPROM(0, true);
        rxc_buf[0] = 'U';
        tx_packet(rxc_buf, 1);
      }
      break;
      case 'f': { // GET failsafe channel values
        rxc_buf[0]='F';
        memcpy((rxc_buf + 1), failsafePPM, sizeof(failsafePPM));
		tx_packet(rxc_buf, (sizeof(failsafePPM) + 1));
      }
      break;
      case 'g': { // SET failsafe channel values
        memcpy(failsafePPM, (rxc_buf + 1), sizeof(failsafePPM));
        failsafeSave();
        rxc_buf[0] = 'G';
        tx_packet(rxc_buf, 1);
      }
      break;
      case 'G': { // SET reset & save failsafe channel values
        memset(failsafePPM, 0, sizeof(failsafePPM));
        failsafeSave();
        rxc_buf[0] = 'G';
        tx_packet(rxc_buf, 1);
      }
      break;
      }
      rx_reset();
    }
  }
  return 0;
}

uint8_t checkIfConnected(uint8_t pin1, uint8_t pin2)
{
  uint8_t ret = 0;
  pinMode(pin1, OUTPUT);
  digitalWrite(pin1, 1);
  digitalWrite(pin2, 1);
  delayMicroseconds(10);
  if (digitalRead(pin2)) {
    digitalWrite(pin1, 0);
    delayMicroseconds(10);
    if (!digitalRead(pin2)) {
      ret = 1;
    }
  }
  pinMode(pin1, INPUT);
  digitalWrite(pin1, 0);
  digitalWrite(pin2, 0);
  return ret;
}

void handleBeacon(void)
{
  if ((rx_config.beacon_frequency) && (nextBeaconTimeMs)) {
    // Listen for RSSI on beacon channel briefly for 'trigger'
    uint8_t brssi = beaconGetRSSI();
    if (brssi > ((beaconRSSIavg>>2) + 20)) {
      nextBeaconTimeMs = millis() + 1000L;
    }
    beaconRSSIavg = (beaconRSSIavg * 3 + brssi * 4) >> 2;
    rfmSetCarrierFrequency(bind_data.rf_frequency);
  }
}

void checkSerial(void)
{
  while (Serial.available() && (((serial_tail + 1) % SERIAL_BUFSIZE) != serial_head)) {
    serial_buffer[serial_tail] = Serial.read();
    serial_tail = (serial_tail + 1) % SERIAL_BUFSIZE;
  }
}

#ifdef DEBUG_DUMP_PPM
void dumpPPM(void)
{
  for (uint8_t i = 0; i < PPM_CHANNELS; i++) {
    Serial.print(PPM[i]);
    Serial.print(',');
  }
  Serial.println();
}
#endif

#ifdef CONFIGURATOR
void checkBinaryMode(void)
{
  if ((Serial.available() > 3) &&
      (Serial.read() == 'B') && (Serial.read() == 'N') &&
      (Serial.read() == 'D') && (Serial.read() == '!')) {
    delay(250);
    while(Serial.available()) {
      if(Serial.read() == 'B') {
        binaryMode();
      }
    }
  }
}
#endif

#endif
