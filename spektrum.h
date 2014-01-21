#define SPKTRM_SYNC1 0x03
#define SPKTRM_SYNC2 0x01

uint32_t spektrumLast = 0;

void sendSpektrumFrame()
{
  uint32_t now = micros();
  if ((now - spektrumLast) > 20000) {
    spektrumLast = now;
    Serial.write(SPKTRM_SYNC1);
    Serial.write(SPKTRM_SYNC2);
    for (uint8_t ch = 0; ch < 7; ch++) {
      Serial.write((ch<<2) | ((PPM[ch] >> 8) & 0x03));
      Serial.write(PPM[ch] & 0xff);
    }
  }
}

