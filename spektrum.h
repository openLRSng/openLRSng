#define SPKTRM_SYNC1 0x03
#define SPKTRM_SYNC2 0x01

uint32_t spektrumLast = 0;
uint8_t  spektrumSendHi = 0;

void sendSpektrumFrame()
{
  uint32_t now = micros();
  if ((now - spektrumLast) > 10000) {
    uint8_t  ch = ((spektrumSendHi++) & 1) ? 7 : 0;
    spektrumLast = now;
    Serial.write(SPKTRM_SYNC1);
    Serial.write(SPKTRM_SYNC2);
    for (uint8_t i = 0; i < 7; i++) {
      Serial.write((ch << 2) | ((PPM[ch] >> 8) & 0x03));
      Serial.write(PPM[ch] & 0xff);
      ch++;
    }
  }
}

