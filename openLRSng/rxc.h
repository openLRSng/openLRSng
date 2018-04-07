#ifndef _RXC_H_
#define _RXC_H_

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
  uint8_t tx_buf[sizeof(rxcSpecialPins) + 5];
  uint32_t last_time = micros();
  tx_buf[0] = 't';
  init_rfm(1);

  do {
    tx_packet(tx_buf, 1);
    rx_reset();
    delay(250);
  } while ((RF_Mode == RECEIVE) && (!Serial.available()) && ((micros() - last_time) < 30000000));

  if (RF_Mode == RECEIVE) {
    return 2; // timeout
  }

  rfmGetPacket(tx_buf, rfmGetPacketLength());

  if (tx_buf[0] != 'T') {
    return 3; // error, bad data
  }

  rxcVersion = ( ((uint16_t) tx_buf[1] << 8) +  tx_buf[2] );
  rxcNumberOfOutputs = tx_buf[3];
  rxcSpecialPinCount = tx_buf[4];

  if (rxcSpecialPinCount > RXC_MAX_SPECIAL_PINS) {
    return 3;
  }

  memcpy(rxcSpecialPins, tx_buf + 5, sizeof(rxSpecialPinMap) * rxcSpecialPinCount);

  tx_buf[0] = 'p'; // ask for config dump
  tx_packet(tx_buf, 1);
  rx_reset();
  delay(50);

  if (RF_Mode == RECEIVE) {
    return 2; //timeout
  }

  rfmGetPacket(tx_buf, sizeof(rx_config) + 1);

  if (tx_buf[0] != 'P') {
    return 3; // error, bad data
  }

  memcpy(&rx_config, tx_buf + 1, sizeof(rx_config));

  return 1; // ok,
}

#endif
