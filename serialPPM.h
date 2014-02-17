#define SPKTRM_SYNC1 0x03
#define SPKTRM_SYNC2 0x01

uint32_t sOutLast = 0;
uint8_t  spektrumSendHi = 0;

void sendSpektrumFrame()
{
  uint32_t now = micros();
  if ((now - sOutLast) > 10000) {
    uint8_t  ch = ((spektrumSendHi++) & 1) ? 7 : 0;
    sOutLast = now;
    Serial.write(SPKTRM_SYNC1);
    Serial.write(SPKTRM_SYNC2);
    for (uint8_t i = 0; i < 7; i++) {
      Serial.write((ch << 2) | ((PPM[ch] >> 8) & 0x03));
      Serial.write(PPM[ch] & 0xff);
      ch++;
    }
  }
}

#define SBUS_SYNC 0x0f
#define SBUS_TAIL 0x00
struct sbus_dat {
  uint16_t ch0 : 11;
  uint16_t ch1 : 11;
  uint16_t ch2 : 11;
  uint16_t ch3 : 11;
  uint16_t ch4 : 11;
  uint16_t ch5 : 11;
  uint16_t ch6 : 11;
  uint16_t ch7 : 11;
  uint16_t ch8 : 11;
  uint16_t ch9 : 11;
  uint16_t ch10 : 11;
  uint16_t ch11 : 11;
  uint16_t ch12 : 11;
  uint16_t ch13 : 11;
  uint16_t ch14 : 11;
  uint16_t ch15 : 11;
  uint8_t  status;
} __attribute__ ((__packed__));

union sbus_msg {
  uint8_t bytes[23];
  struct sbus_dat msg;
} sbus;

void sendSBUSFrame(uint8_t failsafe, uint8_t lostpack)
{
  uint32_t now = micros();
  if ((now - sOutLast) > 10000) {
    sOutLast = now;
    sbus.msg.ch0 = PPM[0]<<1;
    sbus.msg.ch1 = PPM[1]<<1;
    sbus.msg.ch2 = PPM[2]<<1;
    sbus.msg.ch3 = PPM[3]<<1;
    sbus.msg.ch4 = PPM[4]<<1;
    sbus.msg.ch5 = PPM[5]<<1;
    sbus.msg.ch6 = PPM[6]<<1;
    sbus.msg.ch7 = PPM[7]<<1;
    sbus.msg.ch8 = PPM[8]<<1;
    sbus.msg.ch9 = PPM[9]<<1;
    sbus.msg.ch10 = PPM[10]<<1;
    sbus.msg.ch11 = PPM[11]<<1;
    sbus.msg.ch12 = PPM[12]<<1;
    sbus.msg.ch13 = PPM[13]<<1;
    sbus.msg.ch14 = PPM[14]<<1;
    sbus.msg.ch15 = PPM[15]<<1;
    sbus.msg.status = (failsafe ? 0x08 : 0) | (lostpack ? 0x04: 0);
    Serial.write(SBUS_SYNC);
    for (uint8_t i = 0; i<23; i++) {
      Serial.write(sbus.bytes[i]);
    }
    Serial.write(SBUS_TAIL);
  }
}

#define SUMD_HEAD 0xa8

void sumdWriteCRC(uint8_t c)
{
  Serial.write(c);
  CRC16_add(c);
}

void sendSUMDFrame(uint8_t failsafe)
{
  uint32_t now = micros();
  if ((now - sOutLast) > 10000) {
    sOutLast = now;
    CRC16_reset();
    sumdWriteCRC(SUMD_HEAD);
    sumdWriteCRC(failsafe ? 0x81 : 0x01);
    sumdWriteCRC(16);
    for (uint8_t i = 0; i < 16; i++) {
      uint16_t val = servoBits2Us(PPM[i]) << 3;
      sumdWriteCRC(val >> 8);
      sumdWriteCRC(val & 0xff);
    }
    Serial.write(CRC16_value >> 8);
    Serial.write(CRC16_value & 0xff);
  }
}
