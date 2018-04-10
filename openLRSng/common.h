#ifndef _COMMON_H_
#define _COMMON_H_

//####### COMMON FUNCTIONS #########

#define AVAILABLE    0
#define TRANSMIT    1
#define TRANSMITTED  2
#define RECEIVE    3
#define RECEIVED  4

uint8_t twoBitfy(uint16_t in);
uint8_t countSetBits(uint16_t x);
uint16_t servoUs2Bits(uint16_t x);
uint16_t servoBits2Us(uint16_t x);
uint8_t getPacketSize(struct bind_data *bd);
uint8_t getChannelCount(struct bind_data *bd);

void packChannels(uint8_t config, volatile uint16_t PPM[], uint8_t *p);
void unpackChannels(uint8_t config, volatile uint16_t PPM[], uint8_t *p);

uint32_t delayInMs(uint16_t d);
uint32_t delayInMsLong(uint8_t d);

void scannerMode(void);
void printVersion(uint16_t v);
void fatalBlink(uint8_t blinks);

void RFM22B_Int(void);
void init_rfm(uint8_t isbind);
void tx_reset(void);
void rx_reset(void);
void setHopChannel(uint8_t ch);

uint8_t tx_done(void);
void tx_packet(uint8_t* pkt, uint8_t size);
uint32_t getInterval(struct bind_data *bd);

uint32_t tx_start = 0;
volatile uint8_t RF_Mode = 0;
volatile uint32_t lastReceived = 0;
volatile uint16_t PPM[PPM_CHANNELS] = { 512, 512, 512, 512, 512, 512, 512, 512 , 512, 512, 512, 512, 512, 512, 512, 512 };
const static uint8_t pktsizes[8] = { 0, 7, 11, 12, 16, 17, 21, 0 };

void RFM22B_Int()
{
  if (RF_Mode == TRANSMIT) {
    RF_Mode = TRANSMITTED;
  } else if (RF_Mode == RECEIVE) {
    RF_Mode = RECEIVED;
    lastReceived = millis();
  }
}

uint8_t getPacketSize(struct bind_data *bd)
{
  return pktsizes[(bd->flags & 0x07)];
}

uint8_t getChannelCount(struct bind_data *bd)
{
  return (((bd->flags & 7) / 2) + 1 + (bd->flags & 1)) * 4;
}

uint32_t getInterval(struct bind_data *bd)
{
  uint32_t ret;
  // Sending an 'x' byte packet at 'y' bps takes approx. (emperical):
  // usec = (x + 15 {20 w/ diversity}) * 8200000 / bps
#define BYTES_AT_BAUD_TO_USEC(bytes, bps, div) ((uint32_t)((bytes) + (div?20:15)) * 8200000L / (uint32_t)(bps))

  ret = (BYTES_AT_BAUD_TO_USEC(getPacketSize(bd), modem_params[bd->modem_params].bps, bd->flags&DIVERSITY_ENABLED) + 2000);

  if (bd->flags & TELEMETRY_MASK) {
    ret += (BYTES_AT_BAUD_TO_USEC(TELEMETRY_PACKETSIZE, modem_params[bd->modem_params].bps, bd->flags&DIVERSITY_ENABLED) + 1000);
  }

  // round up to ms
  ret = ((ret + 999) / 1000) * 1000;

  // enable following to limit packet rate to 50Hz at most
#ifdef LIMIT_RATE_TO_50HZ
  if (ret < 20000) {
    ret = 20000;
  }
#endif

  return ret;
}

uint8_t twoBitfy(uint16_t in)
{
  if (in < 256) {
    return 0;
  } else if (in < 512) {
    return 1;
  } else if (in < 768) {
    return 2;
  } else {
    return 3;
  }
}

void packChannels(uint8_t config, volatile uint16_t PPM[], uint8_t *p)
{
  uint8_t i;
  for (i = 0; i <= (config / 2); i++) { // 4ch packed in 5 bytes
    p[0] = (PPM[0] & 0xff);
    p[1] = (PPM[1] & 0xff);
    p[2] = (PPM[2] & 0xff);
    p[3] = (PPM[3] & 0xff);
    p[4] = ((PPM[0] >> 8) & 3) | (((PPM[1] >> 8) & 3) << 2) | (((PPM[2] >> 8) & 3) << 4) | (((PPM[3] >> 8) & 3) << 6);
    p += 5;
    PPM += 4;
  }
  if (config & 1) { // 4ch packed in 1 byte;
    p[0] = (twoBitfy(PPM[0]) << 6) | (twoBitfy(PPM[1]) << 4) | (twoBitfy(PPM[2]) << 2) | twoBitfy(PPM[3]);
  }
}

void unpackChannels(uint8_t config, volatile uint16_t PPM[], uint8_t *p)
{
  uint8_t i;
  for (i=0; i<=(config/2); i++) { // 4ch packed in 5 bytes
    PPM[0] = (((uint16_t)p[4] & 0x03) << 8) + p[0];
    PPM[1] = (((uint16_t)p[4] & 0x0c) << 6) + p[1];
    PPM[2] = (((uint16_t)p[4] & 0x30) << 4) + p[2];
    PPM[3] = (((uint16_t)p[4] & 0xc0) << 2) + p[3];
    p+=5;
    PPM+=4;
  }
  if (config & 1) { // 4ch packed in 1 byte;
    PPM[0] = (((uint16_t)p[0] >> 6) & 3) * 333 + 12;
    PPM[1] = (((uint16_t)p[0] >> 4) & 3) * 333 + 12;
    PPM[2] = (((uint16_t)p[0] >> 2) & 3) * 333 + 12;
    PPM[3] = (((uint16_t)p[0] >> 0) & 3) * 333 + 12;
  }
}

uint16_t servoUs2Bits(uint16_t x)
{
// conversion between microseconds 800-2200 and value 0-1023
// 808-1000 == 0 - 11     (16us per step)
// 1000-1999 == 12 - 1011 ( 1us per step)
// 2000-2192 == 1012-1023 (16us per step)

  uint16_t ret;

  if (x < 800) {
    ret = 0;
  } else if (x < 1000) {
    ret = (x - 799) / 16;
  } else if (x < 2000) {
    ret = (x - 988);
  } else if (x < 2200) {
    ret = (x - 1992) / 16 + 1011;
  } else {
    ret = 1023;
  }

  return ret;
}

uint16_t servoBits2Us(uint16_t x)
{
  uint16_t ret;

  if (x < 12) {
    ret = 808 + x * 16;
  } else if (x < 1012) {
    ret = x + 988;
  } else if (x < 1024) {
    ret = 2000 + (x - 1011) * 16;
  } else {
    ret = 2192;
  }

  return ret;
}

uint8_t countSetBits(uint16_t x)
{
  x  = x - ((x >> 1) & 0x5555);
  x  = (x & 0x3333) + ((x >> 2) & 0x3333);
  x  = x + (x >> 4);
  x &= 0x0F0F;
  return (x * 0x0101) >> 8;
}

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

void init_rfm(uint8_t isbind)
{
  #ifdef SDN_pin
  digitalWrite(SDN_pin, 1);
  delay(50);
  digitalWrite(SDN_pin, 0);
  delay(50);
  #endif
  rfmSetReadyMode(); // turn on the XTAL and give it time to settle
  delayMicroseconds(600);
  rfmClearIntStatus();
  rfmInit(bind_data.flags&DIVERSITY_ENABLED);
  rfmSetStepSize(bind_data.rf_channel_spacing);

  uint32_t magic = isbind ? BIND_MAGIC : bind_data.rf_magic;
  for (uint8_t i = 0; i < 4; i++) {
    rfmSetHeader(i, (magic >> 24) & 0xff);
    magic = magic << 8; // advance to next byte
  }

  if (isbind) {
    rfmSetModemRegs(&bind_params);
    rfmSetPower(BINDING_POWER);
    rfmSetCarrierFrequency(BINDING_FREQUENCY);
  } else {
    rfmSetModemRegs(&modem_params[bind_data.modem_params]);
    rfmSetPower(bind_data.rf_power);
    rfmSetCarrierFrequency(bind_data.rf_frequency);
  }
}

void tx_reset(void)
{
  tx_start = micros();
  RF_Mode = TRANSMIT;
  rfmSetTX();
}

void rx_reset(void)
{
  rfmClearFIFO(bind_data.flags & DIVERSITY_ENABLED);
  rfmClearIntStatus();
  RF_Mode = RECEIVE;
  rfmSetRX();
}

void check_module(void)
{
  if (rfmGetGPIO1() == 0) {
    // detect the locked module and reboot
    Serial.println("RFM Module Locked");
    Red_LED_ON;
    init_rfm(0);
    rx_reset();
    Red_LED_OFF;
  }
}

void setHopChannel(uint8_t ch)
{
  uint8_t magicLSB = (bind_data.rf_magic & 0xFF) ^ ch;
  rfmSetHeader(3, magicLSB);
  rfmSetChannel(bind_data.hopchannel[ch]);
}

void tx_packet_async(uint8_t* pkt, uint8_t size)
{
  rfmSendPacket(pkt, size);
  rfmClearIntStatus();
  tx_reset();
}

void tx_packet(uint8_t* pkt, uint8_t size)
{
  tx_packet_async(pkt, size);
  while ((RF_Mode == TRANSMIT) && ((micros() - tx_start) < 100000));

  #ifdef TX_TIMING_DEBUG
  if (RF_Mode == TRANSMIT) {
    Serial.println("TX timeout!");
  }
  Serial.print("TX took:");
  Serial.println(micros() - tx_start);
  #endif
}

uint8_t tx_done()
{
  if (RF_Mode == TRANSMITTED) {
    #ifdef TX_TIMING_DEBUG
    Serial.print("TX took:");
    Serial.println(micros() - tx_start);
    #endif
    RF_Mode = AVAILABLE;
    return 1; // success
  } else if ((RF_Mode == TRANSMIT) && ((micros() - tx_start) > 100000)) {
    RF_Mode = AVAILABLE;
    return 2; // timeout
  }
  return 0;
}

void scannerMode(void)
{
  // Spectrum analyser 'submode'
  char c;
  uint32_t nextConfig[4] = { 0, 0, 0, 0 };
  uint32_t startFreq = MIN_RFM_FREQUENCY, endFreq = MAX_RFM_FREQUENCY, nrSamples = 500, stepSize = 50000;
  uint32_t currentFrequency = startFreq;
  uint32_t currentSamples = 0;
  uint8_t nextIndex = 0;
  uint8_t rssiMin = 0, rssiMax = 0;
  uint32_t rssiSum = 0;
  Serial.println("scanner mode");
  rx_reset();

  while (startFreq != 1000) { // if startFreq == 1000, break (used to exit scannerMode)
    while (Serial.available()) {
      c = Serial.read();

      switch (c) {
      case 'D':
        Serial.print('D');
        Serial.print(MIN_RFM_FREQUENCY);
        Serial.print(',');
        Serial.print(MAX_RFM_FREQUENCY);
        Serial.println(',');
        break;

      case 'S':
        currentFrequency = startFreq;
        currentSamples = 0;
        break;

      case '#':
        nextIndex = 0;
        nextConfig[0] = 0;
        nextConfig[1] = 0;
        nextConfig[2] = 0;
        nextConfig[3] = 0;
        break;

      case ',':
        nextIndex++;

        if (nextIndex == 4) {
          nextIndex = 0;
          startFreq = nextConfig[0] * 1000UL; // kHz -> Hz
          endFreq   = nextConfig[1] * 1000UL; // kHz -> Hz
          nrSamples = nextConfig[2]; // count
          stepSize  = nextConfig[3] * 1000UL;   // kHz -> Hz

          // set IF filtter BW (kha)
          if (stepSize < 20000) {
            spiWriteRegister(RFM22B_IFBW, 0x32);   // 10.6kHz
          } else if (stepSize < 30000) {
            spiWriteRegister(RFM22B_IFBW, 0x22);   // 21.0kHz
          } else if (stepSize < 40000) {
            spiWriteRegister(RFM22B_IFBW, 0x26);   // 32.2kHz
          } else if (stepSize < 50000) {
            spiWriteRegister(RFM22B_IFBW, 0x12);   // 41.7kHz
          } else if (stepSize < 60000) {
            spiWriteRegister(RFM22B_IFBW, 0x15);   // 56.2kHz
          } else if (stepSize < 70000) {
            spiWriteRegister(RFM22B_IFBW, 0x01);   // 75.2kHz
          } else if (stepSize < 100000) {
            spiWriteRegister(RFM22B_IFBW, 0x03);   // 90.0kHz
          } else {
            spiWriteRegister(RFM22B_IFBW, 0x05);   // 112.1kHz
          }
        }

        break;

      default:
        if ((c >= '0') && (c <= '9')) {
          c -= '0';
          nextConfig[nextIndex] = nextConfig[nextIndex] * 10 + c;
        }
      }
    }

    if (currentSamples == 0) {
      // retune base
      rfmSetCarrierFrequency(currentFrequency);
      rssiMax = 0;
      rssiMin = 255;
      rssiSum = 0;
      delay(1);
    }

    if (currentSamples < nrSamples) {
      uint8_t val = rfmGetRSSI();
      rssiSum += val;

      if (val > rssiMax) {
        rssiMax = val;
      }

      if (val < rssiMin) {
        rssiMin = val;
      }

      currentSamples++;
    } else {
      Serial.print(currentFrequency / 1000UL);
      Serial.print(',');
      Serial.print(rssiMax);
      Serial.print(',');
      Serial.print(rssiSum / currentSamples);
      Serial.print(',');
      Serial.print(rssiMin);
      Serial.println(',');
      currentFrequency += stepSize;

      if (currentFrequency > endFreq) {
        currentFrequency = startFreq;
      }

      currentSamples = 0;
    }
  }
}

// Print version, either x.y or x.y.z (if z != 0)
#ifdef USE_CONSOLE_SERIAL
void printVersion(uint16_t v, Serial_ *serial)
#else
void printVersion(uint16_t v, HardwareSerial *serial)
#endif
{
  if (serial) {
    serial->print(v >> 8);
    serial->print('.');
    serial->print((v >> 4) & 0x0f);
    if (version & 0x0f) {
      serial->print('.');
      serial->print(v & 0x0f);
    }
  }
}

void printVersion(uint16_t v)
{
  printVersion(v, &Serial);
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

#endif
