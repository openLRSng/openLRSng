#ifndef _SERIALPPM_H_
#define _SERIALPPM_H_

#define SBUS_SYNC 0x0f
#define SBUS_TAIL 0x00
#define SPKTRM_SYNC1 0x03
#define SPKTRM_SYNC2 0x01
#define SUMD_HEAD 0xa8

uint32_t sOutLast = 0;
uint8_t  spektrumSendHi = 0;

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

uint16_t PPMtoSBUS(uint16_t input);
void sumdWriteCRC(uint8_t c);
void sendSBUSFrame(uint8_t failsafe, uint8_t lostpack);
void sendSpektrumFrame(void);
void sendSUMDFrame(uint8_t failsafe);

uint16_t PPMtoSBUS(uint16_t input)
{
  uint16_t value = servoBits2Us(input);
  value = (((value - 880) << 3) / 5);
  return value;
}

void sumdWriteCRC(uint8_t c)
{
  Serial.write(c);
  CRC16_add(c);
}

void sendSBUSFrame(uint8_t failsafe, uint8_t lostpack)
{
  uint32_t now = micros();
  if ((now - sOutLast) > 10000) {
    sOutLast = now;
    sbus.msg.ch0 = PPMtoSBUS(PPM[0]);
    sbus.msg.ch1 = PPMtoSBUS(PPM[1]);
    sbus.msg.ch2 = PPMtoSBUS(PPM[2]);
    sbus.msg.ch3 = PPMtoSBUS(PPM[3]);
    sbus.msg.ch4 = PPMtoSBUS(PPM[4]);
    sbus.msg.ch5 = PPMtoSBUS(PPM[5]);
    sbus.msg.ch6 = PPMtoSBUS(PPM[6]);
    sbus.msg.ch7 = PPMtoSBUS(PPM[7]);
    sbus.msg.ch8 = PPMtoSBUS(PPM[8]);
    sbus.msg.ch9 = PPMtoSBUS(PPM[9]);
    sbus.msg.ch10 = PPMtoSBUS(PPM[10]);
    sbus.msg.ch11 = PPMtoSBUS(PPM[11]);
    sbus.msg.ch12 = PPMtoSBUS(PPM[12]);
    sbus.msg.ch13 = PPMtoSBUS(PPM[13]);
    sbus.msg.ch14 = PPMtoSBUS(PPM[14]);
    sbus.msg.ch15 = PPMtoSBUS(PPM[15]);
    sbus.msg.status = (failsafe ? 0x08 : 0) | (lostpack ? 0x04: 0);
    Serial.write(SBUS_SYNC);
    for (uint8_t i = 0; i<23; i++) {
      Serial.write(sbus.bytes[i]);
    }
    Serial.write(SBUS_TAIL);
  }
}

void sendSpektrumFrame(void)
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

#endif
