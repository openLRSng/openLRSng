/*
    Implementation of PSP (Phoenix Serial Protocol)

    Protocol data structure:
    [SYNC1][SYNC2][CODE][LENGTH_L][LENGTH_H][DATA/DATA ARRAY][CRC]
*/

bool binary_mode_active = false;

#define PSP_SYNC1 0xB5
#define PSP_SYNC2 0x62

#define PSP_REQ_BIND_DATA               1
#define PSP_REQ_RX_CONFIG               2
#define PSP_REQ_RX_JOIN_CONFIGURATION   3
#define PSP_REQ_SCANNER_MODE            4
#define PSP_REQ_SPECIAL_PINS            5
#define PSP_REQ_FW_VERSION              6
#define PSP_REQ_NUMBER_OF_RX_OUTPUTS    7
#define PSP_REQ_ACTIVE_PROFILE          8
#define PSP_REQ_RX_FAILSAFE             9
#define PSP_REQ_TX_CONFIG               10
#define PSP_REQ_PPM_IN                  11
#define PSP_REQ_DEFAULT_PROFILE         12

#define PSP_SET_BIND_DATA               101
#define PSP_SET_RX_CONFIG               102
#define PSP_SET_TX_SAVE_EEPROM          103
#define PSP_SET_RX_SAVE_EEPROM          104
#define PSP_SET_TX_RESTORE_DEFAULT      105
#define PSP_SET_RX_RESTORE_DEFAULT      106
#define PSP_SET_ACTIVE_PROFILE          107
#define PSP_SET_RX_FAILSAFE             108
#define PSP_SET_TX_CONFIG               109
#define PSP_SET_DEFAULT_PROFILE         110

#define PSP_SET_EXIT                    199

#define PSP_INF_ACK                     201
#define PSP_INF_REFUSED                 202
#define PSP_INF_CRC_FAIL                203
#define PSP_INF_DATA_TOO_LONG           204

extern volatile uint8_t ppmAge;
extern struct rxSpecialPinMap rxcSpecialPins[];
extern uint8_t rxcSpecialPinCount;
extern uint8_t rxcNumberOfOutputs;
extern uint16_t rxcVersion;
uint8_t rxcConnect();
uint16_t getChannel(uint8_t ch);
uint8_t PSP_crc;

#define AS_U8ARRAY(x) ((uint8_t *)(x))

void PSP_serialize_uint8(uint8_t data)
{
  Serial.write(data);
  PSP_crc ^= data;
}

void PSP_serialize_uint16(uint16_t data)
{
  PSP_serialize_uint8(lowByte(data));
  PSP_serialize_uint8(highByte(data));
}

void PSP_serialize_uint32(uint32_t data)
{
  for (uint8_t i = 0; i < 4; i++) {
    PSP_serialize_uint8((uint8_t) (data >> (i * 8)));
  }
}

void PSP_serialize_uint64(uint64_t data)
{
  for (uint8_t i = 0; i < 8; i++) {
    PSP_serialize_uint8((uint8_t) (data >> (i * 8)));
  }
}

void PSP_serialize_float32(float f)
{
  uint8_t *b = (uint8_t*) & f;

  for (uint8_t i = 0; i < sizeof(f); i++) {
    PSP_serialize_uint8(b[i]);
  }
}

void PSP_protocol_head(uint8_t code, uint16_t length)
{
  PSP_crc = 0; // reset crc

  Serial.write(PSP_SYNC1);
  Serial.write(PSP_SYNC2);

  PSP_serialize_uint8(code);
  PSP_serialize_uint16(length);
}

void PSP_protocol_tail()
{
  Serial.write(PSP_crc);
}

void PSP_ACK()
{
  PSP_protocol_head(PSP_INF_ACK, 1);

  PSP_serialize_uint8(0x01);
}

void PSP_process_data(uint8_t code, uint16_t payload_length_received, uint8_t data_buffer[])
{
  switch (code) {
  case PSP_REQ_BIND_DATA:
    PSP_protocol_head(PSP_REQ_BIND_DATA, sizeof(bind_data));
    {
      for (uint16_t i = 0; i < sizeof(bind_data); i++) {
        PSP_serialize_uint8(AS_U8ARRAY(&bind_data)[i]);
      }
    }
    break;
  case PSP_REQ_RX_CONFIG:
    PSP_protocol_head(PSP_REQ_RX_CONFIG, sizeof(rx_config));
    {
      for (uint16_t i = 0; i < sizeof(rx_config); i++) {
        PSP_serialize_uint8(AS_U8ARRAY(&rx_config)[i]);
      }
    }
    break;
  case PSP_REQ_RX_JOIN_CONFIGURATION:
    PSP_protocol_head(PSP_REQ_RX_JOIN_CONFIGURATION, 1);
    // 1 success, 2 timeout, 3 failed response

    PSP_serialize_uint8(rxcConnect());
    break;
  case PSP_REQ_SCANNER_MODE:
    PSP_protocol_head(PSP_REQ_SCANNER_MODE, 1);
    PSP_serialize_uint8(0x01);
    PSP_protocol_tail();

    scannerMode();

    return;
    break;
  case PSP_REQ_SPECIAL_PINS:
    PSP_protocol_head(PSP_REQ_SPECIAL_PINS, sizeof(struct rxSpecialPinMap) * rxcSpecialPinCount);
    {
      for (uint16_t i = 0; i < sizeof(struct rxSpecialPinMap) * rxcSpecialPinCount; i++) {
        PSP_serialize_uint8(AS_U8ARRAY(&rxcSpecialPins)[i]);
      }
    }
    break;
  case PSP_REQ_FW_VERSION:
    PSP_protocol_head(PSP_REQ_FW_VERSION, sizeof(version));
    {
      PSP_serialize_uint16(version);
    }
    break;
  case PSP_REQ_NUMBER_OF_RX_OUTPUTS:
    PSP_protocol_head(PSP_REQ_NUMBER_OF_RX_OUTPUTS, 1);
    {
      PSP_serialize_uint8(rxcNumberOfOutputs);
    }
    break;
  case PSP_REQ_ACTIVE_PROFILE:
    PSP_protocol_head(PSP_REQ_ACTIVE_PROFILE, 1);
    {
      PSP_serialize_uint8(activeProfile);
    }
    break;
  case PSP_REQ_RX_FAILSAFE: {
    uint8_t rxtx_buf;
    rxtx_buf = 'f';
    tx_packet(&rxtx_buf, 1);
    rx_reset();
    RF_Mode = Receive;
    delay(200);

    if (RF_Mode == Received) {
      spiSendAddress(0x7f);
      rxtx_buf = spiReadData();
      if (rxtx_buf == 'F') {
        PSP_protocol_head(PSP_REQ_RX_FAILSAFE, 32);
        for (uint8_t i = 0; i < 32; i++) {
          PSP_serialize_uint8(spiReadData()); // failsafe data
        }
      } else {
        PSP_protocol_head(PSP_REQ_RX_FAILSAFE, 1);
        PSP_serialize_uint8(0x01); // failsafe not set
      }
    } else {
      PSP_protocol_head(PSP_REQ_RX_FAILSAFE, 1);
      PSP_serialize_uint8(0x00); // fail
    }
  }
  break;
  case PSP_REQ_TX_CONFIG: {
    PSP_protocol_head(PSP_REQ_TX_CONFIG, sizeof(tx_config));
    {
      // Force correct TX type based on firmware
#if (RFMTYPE == 868)
      tx_config.rfm_type = 1;
#elif (RFMTYPE == 915)
      tx_config.rfm_type = 2;
#else
      tx_config.rfm_type = 0;
#endif
      // Set current watchdog status
      if (watchdogUsed) {
        tx_config.flags |= WATCHDOG_USED;
      } else {
        tx_config.flags &=~ WATCHDOG_USED;
      }

      for (uint16_t i = 0; i < sizeof(tx_config); i++) {
        PSP_serialize_uint8(AS_U8ARRAY(&tx_config)[i]);
      }
    }
  }
  break;
  case PSP_REQ_PPM_IN: {
    PSP_protocol_head(PSP_REQ_PPM_IN, 33);

    PSP_serialize_uint8((ppmAge < 255) ? ppmAge++ : ppmAge);
    for (uint8_t i = 0; i < 16; i++) {
      PSP_serialize_uint16(servoBits2Us(getChannel(i)));
    }
  }
  break;
  case PSP_REQ_DEFAULT_PROFILE:
    PSP_protocol_head(PSP_REQ_DEFAULT_PROFILE, 1);
    {
      PSP_serialize_uint8(defaultProfile);
    }
    break;
    // SET
  case PSP_SET_BIND_DATA:
    PSP_protocol_head(PSP_SET_BIND_DATA, 1);

    if (payload_length_received == sizeof(bind_data)) {

      for (uint16_t i = 0; i < sizeof(bind_data); i++) {
        AS_U8ARRAY(&bind_data)[i] = data_buffer[i];
      }

      PSP_serialize_uint8(0x01);
    } else {
      PSP_serialize_uint8(0x00);
    }
    break;
  case PSP_SET_RX_CONFIG:
    PSP_protocol_head(PSP_SET_RX_CONFIG, 1);

    if (payload_length_received == sizeof(rx_config)) {
      for (uint16_t i = 0; i < sizeof(rx_config); i++) {
        AS_U8ARRAY(&rx_config)[i] = data_buffer[i];
      }

      PSP_serialize_uint8(0x01);
    } else {
      PSP_serialize_uint8(0x00);
    }
    break;
  case PSP_SET_TX_SAVE_EEPROM:
    PSP_protocol_head(PSP_SET_TX_SAVE_EEPROM, 1);
    txWriteEeprom();
    PSP_serialize_uint8(0x01);
    break;
  case PSP_SET_RX_SAVE_EEPROM:
    PSP_protocol_head(PSP_SET_RX_SAVE_EEPROM, 1);

    {
      uint8_t tx_buf[1 + sizeof(rx_config)];
      tx_buf[0] = 'u';
      memcpy(tx_buf + 1, &rx_config, sizeof(rx_config));
      tx_packet(tx_buf, sizeof(rx_config) + 1);
      rx_reset();
      RF_Mode = Receive;
      delay(800);

      if (RF_Mode == Received) {
        spiSendAddress(0x7f); // Send the package read command
        tx_buf[0] = spiReadData();
        if (tx_buf[0]=='U') {
          PSP_serialize_uint8(0x01); // success
        } else {
          PSP_serialize_uint8(0x00); // fail
        }
      } else {
        PSP_serialize_uint8(0x00); // fail
      }
    }
    break;
  case PSP_SET_TX_RESTORE_DEFAULT:
    PSP_protocol_head(PSP_SET_TX_RESTORE_DEFAULT, 1);

    bindInitDefaults();
    txInitDefaults();

    PSP_serialize_uint8(0x01); // done
    break;
  case PSP_SET_RX_RESTORE_DEFAULT:
    PSP_protocol_head(PSP_SET_RX_RESTORE_DEFAULT, 1);

    uint8_t tx_buf[1 + sizeof(rx_config)];
    tx_buf[0] = 'i';
    tx_packet(tx_buf,1);
    rx_reset();
    RF_Mode = Receive;
    delay(800);

    if (RF_Mode == Received) {
      spiSendAddress(0x7f);   // Send the package read command
      tx_buf[0] = spiReadData();

      for (uint8_t i = 0; i < sizeof(rx_config); i++) {
        tx_buf[i + 1] = spiReadData();
      }

      memcpy(&rx_config, tx_buf + 1, sizeof(rx_config));

      if (tx_buf[0] == 'I') {
        PSP_serialize_uint8(0x01); // success
      } else {
        PSP_serialize_uint8(0x00); // fail
      }
    } else {
      PSP_serialize_uint8(0x00); // fail
    }
    break;
  case PSP_SET_ACTIVE_PROFILE:
    PSP_protocol_head(PSP_SET_ACTIVE_PROFILE, 1);

    activeProfile=data_buffer[0];
    txReadEeprom();
    PSP_serialize_uint8(0x01); // done
    break;
  case PSP_SET_RX_FAILSAFE:
    PSP_protocol_head(PSP_SET_RX_FAILSAFE, 1);
    {
      uint8_t rxtx_buf[33];
      if (payload_length_received == 32) {
        rxtx_buf[0] = 'g';
        memcpy(rxtx_buf + 1, data_buffer, 32);
        tx_packet(rxtx_buf, 33);
      } else {
        rxtx_buf[0] = 'G';
        tx_packet(rxtx_buf, 1);
      }
      rx_reset();
      RF_Mode = Receive;
      delay(1100);

      if (RF_Mode == Received) {
        spiSendAddress(0x7f);
        if (spiReadData() == 'G') {
          PSP_serialize_uint8(0x01);
        } else {
          PSP_serialize_uint8(0x00);
        }
      } else {
        PSP_serialize_uint8(0x00);
      }
    }
    break;
  case PSP_SET_TX_CONFIG:
    PSP_protocol_head(PSP_SET_TX_CONFIG, 1);

    if (payload_length_received == sizeof(tx_config)) {
      for (uint16_t i = 0; i < sizeof(tx_config); i++) {
        AS_U8ARRAY(&tx_config)[i] = data_buffer[i];
      }
      PSP_serialize_uint8(0x01);
    } else {
      PSP_serialize_uint8(0x00);
    }
    break;
  case PSP_SET_DEFAULT_PROFILE:
    PSP_protocol_head(PSP_SET_DEFAULT_PROFILE, 1);

    setDefaultProfile(data_buffer[0]);
    PSP_serialize_uint8(0x01); // done
    break;
  case PSP_SET_EXIT:
    PSP_protocol_head(PSP_SET_EXIT, 1);
    PSP_serialize_uint8(0x01);
    PSP_protocol_tail();

    binary_mode_active = false;

    return;
    break;
  default: // Unrecognized code
    PSP_protocol_head(PSP_INF_REFUSED, 1);

    PSP_serialize_uint8(0x00);
  }

  // send over crc
  PSP_protocol_tail();
}

void PSP_read(void)
{
  static uint8_t data;
  static uint8_t state;
  static uint8_t code;
  static uint8_t message_crc;
  static uint16_t payload_length_expected;
  static uint16_t payload_length_received;
  static uint8_t data_buffer[100];

  while (Serial.available()) {
    data = Serial.read();

    switch (state) {
    case 0:
      if (data == PSP_SYNC1) {
        state++;
      }
      break;
    case 1:
      if (data == PSP_SYNC2) {
        state++;
      } else {
        state = 0; // Restart and try again
      }
      break;
    case 2:
      code = data;
      message_crc = data;

      state++;
      break;
    case 3: // LSB
      payload_length_expected = data;
      message_crc ^= data;

      state++;
      break;
    case 4: // MSB
      payload_length_expected |= data << 8;
      message_crc ^= data;

      state++;

      if (payload_length_expected > sizeof(data_buffer)) {
        // Message too long, we won't accept
        PSP_protocol_head(PSP_INF_DATA_TOO_LONG, 1);
        PSP_serialize_uint8(0x01);
        PSP_protocol_tail();

        state = 0; // Restart
      }
      break;
    case 5:
      data_buffer[payload_length_received] = data;
      message_crc ^= data;
      payload_length_received++;

      if (payload_length_received >= payload_length_expected) {
        state++;
      }
      break;
    case 6:
      if (message_crc == data) {
        // CRC is ok, process data
        PSP_process_data(code, payload_length_received, data_buffer);
      } else {
        // respond that CRC failed
        PSP_protocol_head(PSP_INF_CRC_FAIL, 2);

        PSP_serialize_uint8(code);
        PSP_serialize_uint8(PSP_crc);
      }

      // reset variables
      memset(data_buffer, 0, sizeof(data_buffer));

      payload_length_received = 0;
      state = 0;
      break;
    }
  }
}

void binaryMode()
{
  // Just entered binary mode, flip the bool
  binary_mode_active = true;

  while (binary_mode_active == true) { // LOCK user here until exit command is received
    if (Serial.available()) {
      PSP_read();
    }
  }
}
