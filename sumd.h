#define SUMD_HEAD 0xa8
uint32_t sumdLast = 0;
uint16_t sumdCRC;

void sumdWriteCRC(uint8_t c)
{
  uint8_t i;
  Serial.write(c);
  sumdCRC ^= (uint16_t)c<<8;
  for (i = 0; i < 8; i++) {
    if (sumdCRC & 0x8000) {
      sumdCRC = (sumdCRC << 1) ^ 0x1021;
    } else {
      sumdCRC = (sumdCRC << 1);
    }
  }
}

void sendSUMDFrame(uint8_t failsafe)
{
  uint32_t now = micros();
  if ((now - sumdLast) > 10000) {
    sumdLast = now;
    sumdCRC = 0;
    sumdWriteCRC(SUMD_HEAD);
    sumdWriteCRC(failsafe ? 0x81 : 0x01);
    sumdWriteCRC(16);
    for (uint8_t i = 0; i < 16; i++) {
      uint16_t val = servoBits2Us(PPM[i]) << 3;
      sumdWriteCRC(val >> 8);
      sumdWriteCRC(val & 0xff);
    }
    Serial.write(sumdCRC >> 8);
    Serial.write(sumdCRC & 0xff);
  }
}
