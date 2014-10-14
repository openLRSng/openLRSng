/*
  RX connection handling
*/

#define RXC_MAX_SPECIAL_PINS 16
struct rxSpecialPinMap rxcSpecialPins[RXC_MAX_SPECIAL_PINS];
uint8_t rxcSpecialPinCount;
uint8_t rxcNumberOfOutputs;
uint16_t rxcVersion;

uint8_t rxcConnect()
{
  uint8_t tx_buf[1 + sizeof(rx_config)];
  uint32_t last_time = micros();

  init_rfm(1);
  do {
    tx_buf[0] = 't';
    tx_packet(tx_buf, 1);
    RF_Mode = Receive;
    rx_reset();
    delay(250);
  } while ((RF_Mode == Receive) && (!Serial.available()) && ((micros() - last_time) < 30000000));

  if (RF_Mode == Receive) {
    return 2;
  }

  spiSendAddress(0x7f);   // Send the package read command
  tx_buf[0] = spiReadData();
  if (tx_buf[0] != 'T') {
    return 3;
  }

  rxcVersion = (uint16_t)spiReadData() * 256;
  rxcVersion += spiReadData();

  rxcNumberOfOutputs = spiReadData();
  rxcSpecialPinCount = spiReadData();
  if (rxcSpecialPinCount > RXC_MAX_SPECIAL_PINS) {
    return 3;
  }

  for (uint8_t i = 0; i < sizeof(struct rxSpecialPinMap) * rxcSpecialPinCount; i++) {
    *(((uint8_t*)&rxcSpecialPins) + i) = spiReadData();
  }

  tx_buf[0] = 'p'; // ask for config dump
  tx_packet(tx_buf, 1);
  RF_Mode = Receive;
  rx_reset();
  delay(50);

  if (RF_Mode == Receive) {
    return 2;
  }
  spiSendAddress(0x7f);   // Send the package read command
  tx_buf[0] = spiReadData();
  if (tx_buf[0] != 'P') {
    return 3;
  }

  for (uint8_t i = 0; i < sizeof(rx_config); i++) {
    *(((uint8_t*)&rx_config) + i) = spiReadData();
  }
  return 1;
}

