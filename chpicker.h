//OpenLRSng adaptive channel picker

// development only...
#define CHANNELS_TO_PICK 12

void isort(uint8_t *a, uint8_t n)
{
  for (uint8_t i=1; i<n; i++) {
    for (uint8_t j = i; j> 0 && a[j] < a[j-1]; j--) {
      uint8_t v = a[j];
      a[j] = a[j-1];
      a[j-1] = v;
    }
  }
}

uint8_t chooseChannelsPerRSSI()
{
  uint8_t chRSSImax[255];
  uint8_t picked[CHANNELS_TO_PICK];
  Serial.println("Entering adaptive channel selection");
  init_rfm(0);
  rx_reset();
  for (uint8_t ch=1; ch<255; ch++) {
    uint32_t start = millis();
    rfmSetChannel(ch);
    delay(1);
    chRSSImax[ch]=0;
    while ((millis()-start) < 500) {
      uint8_t rssi = rfmGetRSSI();
      if (rssi > chRSSImax[ch]) {
        chRSSImax[ch] = rssi;
      }
    }
    if (ch&1) {
      Green_LED_OFF
      Red_LED_ON
    } else {
      Green_LED_ON
      Red_LED_OFF
    }
  }

  for (uint8_t i=0; i < CHANNELS_TO_PICK; i++) {
    uint8_t lowest, lowestRSSI=255;
    for (uint8_t ch=1; ch<255; ch++) {
      if (chRSSImax[ch] < lowestRSSI) {
        lowestRSSI = chRSSImax[ch];
        lowest = ch;
      }
    }
    picked[i] = lowest;
    chRSSImax[lowest]=255;
    if (lowest>1) {
      chRSSImax[lowest-1]=255;
    }
    if (lowest>2) {
      chRSSImax[lowest-2]=200;
    }
    if (lowest<254) {
      chRSSImax[lowest+1]=255;
    }
    if (lowest<253) {
      chRSSImax[lowest+2]=200;
    }
  }

  isort(picked,CHANNELS_TO_PICK);

  for (uint8_t i=0; i < CHANNELS_TO_PICK; i++) {
    Serial.print(picked[i]);
    Serial.print(',');
  }
  Serial.println();

  return 1;
}
