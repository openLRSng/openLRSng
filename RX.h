/****************************************************
 * OpenLRSng receiver code
 ****************************************************/

uint8_t RF_channel = 0;

uint32_t time;
uint32_t last_pack_time = 0;
uint32_t last_rssi_time = 0;
uint32_t fs_time; // time when failsafe activated

uint32_t last_beacon;

uint8_t  RSSI_count = 0;
uint16_t RSSI_sum = 0;
uint8_t  last_rssi_value = 0;

uint8_t  ppmCountter = 0;
uint16_t ppmSync = 40000;
uint8_t  ppmChannels = 8;

boolean PPM_output = 0; // set if PPM output is desired

uint8_t firstpack = 0;
uint8_t lostpack = 0;

boolean willhop = 0, fs_saved = 0;

ISR(TIMER1_OVF_vect)
{
  if (ppmCountter >= ppmChannels) {
    ICR1 = ppmSync;
    ppmCountter = 0;
    ppmSync = 40000;

    if (PPM_output) {   // clear all bits
      PORTB &= ~PWM_MASK_PORTB(PWM_WITHPPM_MASK);
      PORTD &= ~PWM_MASK_PORTD(PWM_WITHPPM_MASK);
    } else {
      PORTB &= ~PWM_MASK_PORTB(PWM_ALL_MASK);
      PORTD &= ~PWM_MASK_PORTD(PWM_ALL_MASK);
    }
  } else {
    uint16_t ppmOut = servoBits2Us(PPM[ppmCountter]) * 2;
    ppmSync -= ppmOut;
    if (ppmSync<5000) ppmSync=5000;
    ICR1 = ppmOut;

    if (PPM_output) {
      PORTB &= ~PWM_MASK_PORTB(PWM_WITHPPM_MASK);
      PORTD &= ~PWM_MASK_PORTD(PWM_WITHPPM_MASK);
      if (ppmCountter < 7) { // only 6 (7 if defined by FORCED_PPM_OUTPUT) channels available in PPM mode
        // shift channels over the PPM pin
        uint8_t pin = (ppmCountter >= PPM_CH) ? (ppmCountter + 1) : ppmCountter;
        PORTB |= PWM_MASK_PORTB(PWM_MASK[pin]);
        PORTD |= PWM_MASK_PORTD(PWM_MASK[pin]);
      }
    } else {
      PORTB &= ~PWM_MASK_PORTB(PWM_ALL_MASK);
      PORTD &= ~PWM_MASK_PORTD(PWM_ALL_MASK);
      if (ppmCountter < 8) {
        PORTB |= PWM_MASK_PORTB(PWM_MASK[ppmCountter]);
        PORTD |= PWM_MASK_PORTD(PWM_MASK[ppmCountter]);
      }
    }

    ppmCountter++;
  }
}

void setupPPMout()
{
  if (PPM_output) {
    TCCR1A = (1 << WGM11) | (1 << COM1A1) | (1 << COM1A0);
  } else {
    TCCR1A = (1 << WGM11);
  }

  TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
  ICR1 = 40000; // just initial value, will be constantly updated
  OCR1A = 600;  // 0.3ms pulse
  TIMSK1 |= (1 << TOIE1);

  pinMode(PWM_1, OUTPUT);
  pinMode(PWM_2, OUTPUT);
  pinMode(PWM_3, OUTPUT);
  pinMode(PWM_4, OUTPUT);
  pinMode(PWM_5, OUTPUT);
  pinMode(PWM_6, OUTPUT);
  pinMode(PWM_7, OUTPUT);
#ifdef FORCED_PPM_OUTPUT
  pinMode(PWM_8, OUTPUT); // if PPM defined at compile time CH8 outputs channel 7 data
#else
  if (!PPM_output) {
    pinMode(PWM_8, OUTPUT); // leave ch8 as input as it is connected to 7 (which can be used as servo too)
  }
#endif
  pinMode(PPM_OUT, OUTPUT);
}

#define FAILSAFE_OFFSET 0x80

void save_failsafe_values(void)
{
  EEPROM.write(FAILSAFE_OFFSET + 0, (PPM[0] & 0xff));
  EEPROM.write(FAILSAFE_OFFSET + 1, (PPM[1] & 0xff));
  EEPROM.write(FAILSAFE_OFFSET + 2, (PPM[2] & 0xff));
  EEPROM.write(FAILSAFE_OFFSET + 3, (PPM[3] & 0xff));
  EEPROM.write(FAILSAFE_OFFSET + 4, (((PPM[0] >> 8) & 3) | (((PPM[1] >> 8) & 3) << 2) | (((PPM[2] >> 8) & 3) << 4) | (((PPM[3] >> 8) & 3) << 6)));
  EEPROM.write(FAILSAFE_OFFSET + 5, (PPM[4] & 0xff));
  EEPROM.write(FAILSAFE_OFFSET + 6, (PPM[5] & 0xff));
  EEPROM.write(FAILSAFE_OFFSET + 7, (PPM[6] & 0xff));
  EEPROM.write(FAILSAFE_OFFSET + 8, (PPM[7] & 0xff));
  EEPROM.write(FAILSAFE_OFFSET + 9, (((PPM[4] >> 8) & 3) | (((PPM[5] >> 8) & 3) << 2) | (((PPM[6] >> 8) & 3) << 4) | (((PPM[7] >> 8) & 3) << 6)));
}

void load_failsafe_values(void)
{
  uint8_t ee_buf[10];

  for (int16_t i = 0; i < 10; i++) {
    ee_buf[i] = EEPROM.read(FAILSAFE_OFFSET + i);
  }

  PPM[0] = ee_buf[0] + ((ee_buf[4] & 0x03) << 8);
  PPM[1] = ee_buf[1] + ((ee_buf[4] & 0x0c) << 6);
  PPM[2] = ee_buf[2] + ((ee_buf[4] & 0x30) << 4);
  PPM[3] = ee_buf[3] + ((ee_buf[4] & 0xc0) << 2);
  PPM[4] = ee_buf[5] + ((ee_buf[9] & 0x03) << 8);
  PPM[5] = ee_buf[6] + ((ee_buf[9] & 0x0c) << 6);
  PPM[6] = ee_buf[7] + ((ee_buf[9] & 0x30) << 4);
  PPM[7] = ee_buf[8] + ((ee_buf[9] & 0xc0) << 2);
}

uint8_t bindReceive(uint32_t timeout)
{
  uint32_t start = millis();
  init_rfm(1);
  RF_Mode = Receive;
  to_rx_mode();
  Serial.println("Waiting bind\n");

  while ((!timeout) || ((millis() - start) < timeout)) {
    if (RF_Mode == Received) {   // RFM22B int16_t pin Enabled by received Data
      Serial.println("Got pkt\n");
      RF_Mode = Receive;
      spiSendAddress(0x7f);   // Send the package read command

      for (uint8_t i = 0; i < sizeof(bind_data); i++) {
        *(((uint8_t*)&bind_data) + i) = spiReadData();
      }

      if (bind_data.version == BINDING_VERSION) {
        Serial.println("data good\n");
        return 1;
      } else {
        rx_reset();
      }
    }
  }
  return 0;
}

uint8_t checkIfGrounded(uint8_t pin)
{
  int8_t ret = 0;

  pinMode(pin, INPUT);
  digitalWrite(pin, 1);
  delay(10);

  if (!digitalRead(pin)) {
    ret = 1;
  }

  digitalWrite(pin, 0);

  return ret;
}

int8_t checkIfConnected(uint8_t pin1, uint8_t pin2)
{
  int8_t ret = 0;
  pinMode(pin1, OUTPUT);
  digitalWrite(pin1, 1);
  digitalWrite(pin2, 1);
  delay(10);

  if (digitalRead(pin2)) {
    digitalWrite(pin1, 0);
    delay(10);

    if (!digitalRead(pin2)) {
      ret = 1;
    }
  }

  pinMode(pin1, INPUT);
  digitalWrite(pin1, 0);
  digitalWrite(pin2, 0);
  return ret;
}

void setup()
{
  //LEDs
  pinMode(Green_LED, OUTPUT);
  pinMode(Red_LED, OUTPUT);

  //RF module pins
  pinMode(SDO_pin, INPUT);   //SDO
  pinMode(SDI_pin, OUTPUT);   //SDI
  pinMode(SCLK_pin, OUTPUT);   //SCLK
  pinMode(IRQ_pin, INPUT);   //IRQ
  pinMode(nSel_pin, OUTPUT);   //nSEL

  pinMode(0, INPUT);   // Serial Rx
  pinMode(1, OUTPUT);   // Serial Tx

  setup_RSSI_output();

  Serial.begin(SERIAL_BAUD_RATE);   //Serial Transmission

  attachInterrupt(IRQ_interrupt, RFM22B_Int, FALLING);

  sei();
  Red_LED_ON;

  if (checkIfConnected(PWM_3,PWM_4)) { // ch1 - ch2 --> force scannerMode
    scannerMode();
  }

  if (checkIfConnected(PWM_1,PWM_2) || (!bindReadEeprom())) {
    Serial.print("EEPROM data not valid or bind jumpper set, forcing bind\n");

    if (bindReceive(0)) {
      bindWriteEeprom();
      Serial.println("Saved bind data to EEPROM\n");
      Green_LED_ON;
    }
  } else {
#ifdef RX_ALWAYS_BIND
    if (bindReceive(500)) {
      bindWriteEeprom();
      Serial.println("Saved bind data to EEPROM\n");
      Green_LED_ON;
    }
#endif
  }

  // Check for bind plug on ch8 (PPM enable).
  if (checkIfConnected(PWM_7,PWM_8)) {
    PPM_output = 1;
  } else {
    PPM_output = 0;
  }

#ifdef FORCED_PPM_OUTPUT
  PPM_output = 1;
#endif

  Serial.print("Entering normal mode with PPM=");
  Serial.println(PPM_output);
  init_rfm(0);   // Configure the RFM22B's registers for normal operation
  RF_channel = 0;
  rfmSetChannel(bind_data.hopchannel[RF_channel]);

  //################### RX SYNC AT STARTUP #################
  RF_Mode = Receive;
  to_rx_mode();

  firstpack = 0;

}

uint8_t rx_buf[21]; // RX buffer

//############ MAIN LOOP ##############
void loop()
{
  uint32_t time;

  if (spiReadRegister(0x0C) == 0) {     // detect the locked module and reboot
    Serial.println("RX hang");
    init_rfm(0);
    to_rx_mode();
  }

  time = micros();

  if (RF_Mode == Received) {   // RFM22B int16_t pin Enabled by received Data

    last_pack_time = micros(); // record last package time
    lostpack = 0;

    Red_LED_OFF;
    Green_LED_ON;

    spiSendAddress(0x7f);   // Send the package read command

    for (int16_t i = 0; i < 11; i++) {
      rx_buf[i] = spiReadData();
    }

    if ((rx_buf[0] == 0x5E) || (rx_buf[0] == 0xF5)) {
      cli();
      unpackChannels(&bind_data, PPM, rx_buf + 1);
      sei();
    }

    if (firstpack == 0) {
      firstpack = 1;
      setupPPMout();
    }

    if (rx_buf[0] == 0xF5) {
      if (!fs_saved) {
        save_failsafe_values();
        fs_saved = 1;
      }
    } else if (fs_saved) {
      fs_saved = 0;
    }

    if (bind_data.flags & TELEMETRY_ENABLED) {
      // reply with telemetry
      uint8_t telemetry_packet[4];
      telemetry_packet[0] = last_rssi_value;
      tx_packet(telemetry_packet, 4);
    }

    RF_Mode = Receive;
    rx_reset();

    willhop = 1;

    Green_LED_OFF;
  }

  time = micros();

  // sample RSSI when packet is in the 'air'
  if ((lostpack < 2) && (last_rssi_time != last_pack_time) &&
      (time - last_pack_time) > (getInterval(&bind_data) - 1500)) {
    last_rssi_time = last_pack_time;
    last_rssi_value = rfmGetRSSI(); // Read the RSSI value
    RSSI_sum += last_rssi_value;    // tally up for average
    RSSI_count++;

    if (RSSI_count > 20) {
      RSSI_sum /= RSSI_count;
      set_RSSI_output(map(constrain(RSSI_sum, 45, 200), 40, 200, 0, 255));
      RSSI_sum = 0;
      RSSI_count = 0;
    }
  }

  time = micros();

  if (firstpack) {
    if ((lostpack < 5) && (time - last_pack_time) > (getInterval(&bind_data) + 1000)) {
      // we packet, hop to next channel
      lostpack++;
      last_pack_time += getInterval(&bind_data);
      willhop = 1;
      Red_LED_ON;
      set_RSSI_output(0);
    } else if ((time - last_pack_time) > (getInterval(&bind_data) * bind_data.hopcount)) {
      // hop slowly to allow resync with TX
      last_pack_time = time;

      if (lostpack < 10) {
        lostpack++;
      } else if (lostpack == 10) {
        lostpack = 11;
        // Serious trouble, apply failsafe
        load_failsafe_values();
        fs_time = time;
      } else if (bind_data.beacon_interval && bind_data.beacon_deadtime &&
                 bind_data.beacon_frequency) {
        if (lostpack == 11) {   // failsafes set....
          if ((time - fs_time) > (bind_data.beacon_deadtime * 1000000UL)) {
            lostpack = 12;
            last_beacon = time;
          }
        } else if (lostpack == 12) {   // beacon mode active
          if ((time - last_beacon) > (bind_data.beacon_interval * 1000000UL)) {
            last_beacon = time;
            beacon_send();
            init_rfm(0);   // go back to normal RX
            rx_reset();
          }
        }
      }

      willhop = 1;
    }
  }

  if (willhop == 1) {
    RF_channel++;

    if (RF_channel >= bind_data.hopcount) {
      RF_channel = 0;
    }

    rfmSetChannel(bind_data.hopchannel[RF_channel]);
    willhop = 0;
  }
}
