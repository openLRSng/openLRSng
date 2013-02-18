//####### COMMON FUNCTIONS #########
volatile uint8_t RF_Mode = 0;

#define Available 0
#define Transmit 1
#define Transmitted 2
#define Receive 3
#define Received 4

void rfmSetCarrierFrequency(uint32_t f);
uint8_t rfmGetRSSI(void);
void RF22B_init_parameter(void);
uint8_t spiReadRegister(uint8_t address);
void spiWriteRegister(uint8_t address, uint8_t data);
void tx_packet(uint8_t* pkt, uint8_t size);
void to_rx_mode(void);
volatile uint8_t rx_buf[11]; // RX buffer

#define PPM_CHANNELS 8
volatile uint16_t PPM[PPM_CHANNELS] = { 512, 512, 512, 512, 512, 512, 512, 512 };

// conversion between microseconds 800-2200 and value 0-1023
// 808-1000 == 0 - 11     (16us per step)
// 1000-1999 == 12 - 1011 ( 1us per step)
// 2000-2192 == 1012-1023 (16us per step)

uint16_t servoUs2Bits(uint16_t x)
{
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

// Spectrum analyser 'submode'
void scannerMode(void)
{
  char c;
  uint32_t nextConfig[4] = {0, 0, 0, 0};
  uint32_t startFreq = 430000000, endFreq = 440000000, nrSamples = 500, stepSize = 50000;
  uint32_t currentFrequency = startFreq;
  uint32_t currentSamples = 0;
  uint8_t nextIndex = 0;
  uint8_t rssiMin = 0, rssiMax = 0;
  uint32_t rssiSum = 0;
  Red_LED_OFF;
  Green_LED_OFF;
  Serial.println("scanner mode");
  to_rx_mode();

  while (1) {
    while (Serial.available()) {
      c = Serial.read();

      switch (c) {
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
          startFreq = nextConfig[0] * 1000000UL; // MHz
          endFreq   = nextConfig[1] * 1000000UL; // MHz
          nrSamples = nextConfig[2]; // count
          stepSize  = nextConfig[3] * 10000UL;   // 10kHz
          currentFrequency = startFreq;
          currentSamples = 0;

          // set IF filtter BW (kha)
          if (stepSize < 20000) {
            spiWriteRegister(0x1c, 0x32);   // 10.6kHz
          } else if (stepSize < 30000) {
            spiWriteRegister(0x1c, 0x22);   // 21.0kHz
          } else if (stepSize < 40000) {
            spiWriteRegister(0x1c, 0x26);   // 32.2kHz
          } else if (stepSize < 50000) {
            spiWriteRegister(0x1c, 0x12);   // 41.7kHz
          } else if (stepSize < 60000) {
            spiWriteRegister(0x1c, 0x15);   // 56.2kHz
          } else if (stepSize < 70000) {
            spiWriteRegister(0x1c, 0x01);   // 75.2kHz
          } else if (stepSize < 100000) {
            spiWriteRegister(0x1c, 0x03);   // 90.0kHz
          } else {
            spiWriteRegister(0x1c, 0x05);   // 112.1kHz
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
      Serial.print(currentFrequency / 10000UL);
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

  //never exit!!
}


void RFM22B_Int()
{
  if (RF_Mode == Transmit) {
    RF_Mode = Transmitted;
  }

  if (RF_Mode == Receive) {
    RF_Mode = Received;
  }
}

#define NOP() __asm__ __volatile__("nop")

#define RF22B_PWRSTATE_POWERDOWN    0x00
#define RF22B_PWRSTATE_READY        0x01
#define RF22B_PACKET_SENT_INTERRUPT 0x04
#define RF22B_PWRSTATE_RX           0x05
#define RF22B_PWRSTATE_TX           0x09

#define RF22B_Rx_packet_received_interrupt   0x02

uint8_t ItStatus1, ItStatus2;

void spiWriteBit(uint8_t b);

void spiSendCommand(uint8_t command);
void spiSendAddress(uint8_t i);
uint8_t spiReadData(void);
void spiWriteData(uint8_t i);


void to_sleep_mode(void);
void rx_reset(void);

// **** SPI bit banging functions

void spiWriteBit(uint8_t b)
{
  if (b) {
    SCK_off;
    NOP();
    SDI_on;
    NOP();
    SCK_on;
    NOP();
  } else {
    SCK_off;
    NOP();
    SDI_off;
    NOP();
    SCK_on;
    NOP();
  }
}

uint8_t spiReadBit(void)
{
  uint8_t r = 0;
  SCK_on;
  NOP();

  if (SDO_1) {
    r = 1;
  }

  SCK_off;
  NOP();
  return r;
}

void spiSendCommand(uint8_t command)
{
  nSEL_on;
  SCK_off;
  nSEL_off;

  for (uint8_t n = 0; n < 8 ; n++) {
    spiWriteBit(command & 0x80);
    command = command << 1;
  }

  SCK_off;
}

void spiSendAddress(uint8_t i)
{
  spiSendCommand(i & 0x7f);
}

void spiWriteData(uint8_t i)
{
  for (uint8_t n = 0; n < 8; n++) {
    spiWriteBit(i & 0x80);
    i = i << 1;
  }

  SCK_off;
}

uint8_t spiReadData(void)
{
  uint8_t Result = 0;
  SCK_off;

  for (uint8_t i = 0; i < 8; i++) {   //read fifo data byte
    Result = (Result << 1) + spiReadBit();
  }

  return(Result);
}

uint8_t spiReadRegister(uint8_t address)
{
  uint8_t result;
  spiSendAddress(address);
  result = spiReadData();
  nSEL_on;
  return(result);
}

void spiWriteRegister(uint8_t address, uint8_t data)
{
  address |= 0x80; //
  spiSendCommand(address);
  spiWriteData(data);
  nSEL_on;
}

// **** RFM22 access functions

void rfmSetChannel(uint8_t ch)
{
  spiWriteRegister(0x79, ch);
}

uint8_t rfmGetRSSI(void)
{
  return spiReadRegister(0x26);
}

void setModemRegs(struct rfm22_modem_regs* r)
{

  spiWriteRegister(0x1c, r->r_1c);
  spiWriteRegister(0x1d, r->r_1d);
  spiWriteRegister(0x1e, r->r_1e);
  spiWriteRegister(0x20, r->r_20);
  spiWriteRegister(0x21, r->r_21);
  spiWriteRegister(0x22, r->r_22);
  spiWriteRegister(0x23, r->r_23);
  spiWriteRegister(0x24, r->r_24);
  spiWriteRegister(0x25, r->r_25);
  spiWriteRegister(0x2a, r->r_2a);
  spiWriteRegister(0x6e, r->r_6e);
  spiWriteRegister(0x6f, r->r_6f);
  spiWriteRegister(0x70, r->r_70);
  spiWriteRegister(0x71, r->r_71);
  spiWriteRegister(0x72, r->r_72);
}

void rfmSetCarrierFrequency(uint32_t f)
{
  uint16_t fb,fc;
  fb = f / 10000000 - 24;
  fc = (f - (fb + 24) * 10000000) * 4 / 625;

  spiWriteRegister(0x75, 0x40 + (fb & 0x1f));     // sbsel=1 lower 5 bits is band
  spiWriteRegister(0x76, (fc >> 8));
  spiWriteRegister(0x77, (fc & 0xff));
}

void init_rfm(uint8_t isbind)
{
  ItStatus1 = spiReadRegister(0x03);   // read status, clear interrupt
  ItStatus2 = spiReadRegister(0x04);
  spiWriteRegister(0x06, 0x00);    // disable interrupts
  spiWriteRegister(0x07, RF22B_PWRSTATE_READY); // disable lbd, wakeup timer, use internal 32768,xton = 1; in ready mode
  spiWriteRegister(0x09, 0x7f);   // c = 12.5p
  spiWriteRegister(0x0a, 0x05);
  spiWriteRegister(0x0b, 0x12);    // gpio0 TX State
  spiWriteRegister(0x0c, 0x15);    // gpio1 RX State
  spiWriteRegister(0x0d, 0xfd);    // gpio 2 micro-controller clk output
  spiWriteRegister(0x0e, 0x00);    // gpio    0, 1,2 NO OTHER FUNCTION.

  if (isbind) {
    setModemRegs(&bind_params);
  } else {
    setModemRegs(&modem_params[bind_data.modem_params]);
  }

  // Packet settings
  spiWriteRegister(0x30, 0x8c);    // enable packet handler, msb first, enable crc,
  spiWriteRegister(0x32, 0x0f);    // no broadcast, check header bytes 3,2,1,0
  spiWriteRegister(0x33, 0x42);    // 4 byte header, 2 byte synch, variable pkt size
  spiWriteRegister(0x34, 0x0a);    // 10 nibbles (40 bit preamble)
  spiWriteRegister(0x35, 0x2a);    // preath = 5 (20bits), rssioff = 2
  spiWriteRegister(0x36, 0x2d);    // synchronize word 3
  spiWriteRegister(0x37, 0xd4);    // synchronize word 2
  spiWriteRegister(0x38, 0x00);    // synch word 1 (not used)
  spiWriteRegister(0x39, 0x00);    // synch word 0 (not used)

  if (isbind) {
    spiWriteRegister(0x3a, bind_magic[0]);   // tx header
    spiWriteRegister(0x3b, bind_magic[1]);
    spiWriteRegister(0x3c, bind_magic[2]);
    spiWriteRegister(0x3d, bind_magic[3]);
    spiWriteRegister(0x3f, bind_magic[0]);   // verify header
    spiWriteRegister(0x40, bind_magic[1]);
    spiWriteRegister(0x41, bind_magic[2]);
    spiWriteRegister(0x42, bind_magic[3]);
  } else {
    spiWriteRegister(0x3a, bind_data.rf_magic[0]);   // tx header
    spiWriteRegister(0x3b, bind_data.rf_magic[1]);
    spiWriteRegister(0x3c, bind_data.rf_magic[2]);
    spiWriteRegister(0x3d, bind_data.rf_magic[3]);
    spiWriteRegister(0x3f, bind_data.rf_magic[0]);   // verify header
    spiWriteRegister(0x40, bind_data.rf_magic[1]);
    spiWriteRegister(0x41, bind_data.rf_magic[2]);
    spiWriteRegister(0x42, bind_data.rf_magic[3]);
  }

  spiWriteRegister(0x43, 0xff);    // all the bit to be checked
  spiWriteRegister(0x44, 0xff);    // all the bit to be checked
  spiWriteRegister(0x45, 0xff);    // all the bit to be checked
  spiWriteRegister(0x46, 0xff);    // all the bit to be checked

  if (isbind) {
    spiWriteRegister(0x6d, BINDING_POWER);   // set power
  } else {
    spiWriteRegister(0x6d, bind_data.rf_power);   // 7 set power max power
  }

  spiWriteRegister(0x79, 0);

  spiWriteRegister(0x7a, 0x06);   // 60kHz channel spacing

  spiWriteRegister(0x73, 0x00);
  spiWriteRegister(0x74, 0x00);    // no offset

  rfmSetCarrierFrequency(isbind ? BINDING_FREQUENCY : bind_data.rf_frequency);

}

void to_rx_mode(void)
{
  ItStatus1 = spiReadRegister(0x03);
  ItStatus2 = spiReadRegister(0x04);
  spiWriteRegister(0x07, RF22B_PWRSTATE_READY);
  delay(10);
  rx_reset();
  NOP();
}

void rx_reset(void)
{
  spiWriteRegister(0x07, RF22B_PWRSTATE_READY);
  spiWriteRegister(0x7e, 36);    // threshold for rx almost full, interrupt when 1 byte received
  spiWriteRegister(0x08, 0x03);    //clear fifo disable multi packet
  spiWriteRegister(0x08, 0x00);    // clear fifo, disable multi packet
  spiWriteRegister(0x07, RF22B_PWRSTATE_RX);   // to rx mode
  spiWriteRegister(0x05, RF22B_Rx_packet_received_interrupt);
  ItStatus1 = spiReadRegister(0x03);   //read the Interrupt Status1 register
  ItStatus2 = spiReadRegister(0x04);
}

void tx_packet(uint8_t* pkt, uint8_t size)
{

  spiWriteRegister(0x3e, size);   // total tx size

  for (uint8_t i = 0; i < size; i++) {
    spiWriteRegister(0x7f, pkt[i]);
  }

  spiWriteRegister(0x05, RF22B_PACKET_SENT_INTERRUPT);
  ItStatus1 = spiReadRegister(0x03);      //read the Interrupt Status1 register
  ItStatus2 = spiReadRegister(0x04);
#ifdef TX_TIMING
  uint32_t tx_start = micros();
#endif
  spiWriteRegister(0x07, RF22B_PWRSTATE_TX);    // to tx mode

  while (nIRQ_1);

#ifdef TX_TIMING
  Serial.print("TX took:");
  Serial.println(micros() - tx_start);
#endif
}

void beacon_tone(int16_t hz, int16_t len)
{
  int16_t d = 500 / hz; // somewhat limited resolution ;)

  if (d < 1) {
    d = 1;
  }

  int16_t cycles = (len * 1000 / d);

  for (int16_t i = 0; i < cycles; i++) {
    SDI_on;
    delay(d);
    SDI_off;
    delay(d);
  }
}

void beacon_send(void)
{
  Green_LED_ON
  ItStatus1 = spiReadRegister(0x03);   // read status, clear interrupt
  ItStatus2 = spiReadRegister(0x04);
  spiWriteRegister(0x06, 0x00);    // no wakeup up, lbd,
  spiWriteRegister(0x07, RF22B_PWRSTATE_READY);      // disable lbd, wakeup timer, use internal 32768,xton = 1; in ready mode
  spiWriteRegister(0x09, 0x7f);  // (default) c = 12.5p
  spiWriteRegister(0x0a, 0x05);
  spiWriteRegister(0x0b, 0x12);    // gpio0 TX State
  spiWriteRegister(0x0c, 0x15);    // gpio1 RX State
  spiWriteRegister(0x0d, 0xfd);    // gpio 2 micro-controller clk output
  spiWriteRegister(0x0e, 0x00);    // gpio    0, 1,2 NO OTHER FUNCTION.

  spiWriteRegister(0x70, 0x2C);    // disable manchest

  spiWriteRegister(0x30, 0x00);    //disable packet handling

  spiWriteRegister(0x79, 0);    // start channel

  spiWriteRegister(0x7a, 0x05);   // 50khz step size (10khz x value) // no hopping

  spiWriteRegister(0x71, 0x12);   // trclk=[00] no clock, dtmod=[01] direct using SPI, fd8=0 eninv=0 modtyp=[10] FSK
  spiWriteRegister(0x72, 0x02);   // fd (frequency deviation) 2*625Hz == 1.25kHz

  spiWriteRegister(0x73, 0x00);
  spiWriteRegister(0x74, 0x00);    // no offset

  rfmSetCarrierFrequency(bind_data.beacon_frequency);

  spiWriteRegister(0x6d, 0x07);   // 7 set max power 100mW

  delay(10);
  spiWriteRegister(0x07, RF22B_PWRSTATE_TX);    // to tx mode
  delay(10);
  beacon_tone(500, 1);

  spiWriteRegister(0x6d, 0x04);   // 4 set mid power 15mW
  delay(10);
  beacon_tone(250, 1);

  spiWriteRegister(0x6d, 0x00);   // 0 set min power 1mW
  delay(10);
  beacon_tone(160, 1);

  spiWriteRegister(0x07, RF22B_PWRSTATE_READY);
  Green_LED_OFF
}
