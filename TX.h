/****************************************************
 * OpenLRSng transmitter code
 ****************************************************/
uint8_t RF_channel = 0;

uint8_t FSstate = 0; // 1 = waiting timer, 2 = send FS, 3 sent waiting btn release
uint32_t FStime = 0;  // time when button went down...

uint32_t lastSent = 0;

uint32_t lastTelemetry = 0;

#ifdef FRSKY_EMULATION
uint32_t lastFrSky = 0;
#endif

uint8_t RSSI_rx = 0;
uint8_t RSSI_tx = 0;
uint8_t RX_ain0 = 0;
uint8_t RX_ain1 = 0;
uint32_t sampleRSSI = 0;

volatile uint8_t ppmAge = 0; // age of PPM data

volatile uint8_t ppmCounter = PPM_CHANNELS; // ignore data until first sync pulse
volatile uint8_t ppmDetecting = 1; // countter for microPPM detection
volatile uint8_t ppmMicroPPM = 0;  // status flag for 'Futaba microPPM mode'

#ifndef BZ_FREQ
#define BZ_FREQ 2000
#endif

/****************************************************
 * Interrupt Vector
 ****************************************************/

static inline void processPulse(uint16_t pulse)
{
  if (ppmDetecting) {
    if (ppmDetecting>50) {
      ppmDetecting=0;
      if (ppmMicroPPM>10) {
        ppmMicroPPM=1;
      } else {
        ppmMicroPPM=0;
      }
      // Serial.println(ppmMicroPPM?"Futaba micro mode":"Normal PPM mode");
    } else {
      if (pulse<1500) {
        ppmMicroPPM++;
      }
      ppmDetecting++;
    }
  } else {

    if (!ppmMicroPPM) {
      pulse>>=1; // divide by 2 to get servo value on normal PPM
    }

    if (pulse > 2500) {      // Verify if this is the sync pulse (2.5ms)
      ppmCounter = 0;             // -> restart the channel counter
      ppmAge = 0;                 // brand new PPM data received
    } else if ((pulse > 700) && (ppmCounter < PPM_CHANNELS)) { // extra channels will get ignored here
      PPM[ppmCounter++] = servoUs2Bits(pulse);   // Store measured pulse length (converted)
    } else {
      ppmCounter = PPM_CHANNELS; // glitch ignore rest of data
    }
  }
}

#ifdef USE_ICP1 // Use ICP1 in input capture mode
volatile uint16_t startPulse = 0;
ISR(TIMER1_CAPT_vect)
{
  uint16_t stopPulse = ICR1;
  processPulse(stopPulse - startPulse); // as top is 65535 uint16 math will take care of rollover
  startPulse = stopPulse;         // Save time at pulse start
}

void setupPPMinput()
{
  ppmDetecting = 1;
  ppmMicroPPM = 0;
  // Setup timer1 for input capture (PSC=8 -> 0.5ms precision, falling edge)
  TCCR1A = ((1 << WGM10) | (1 << WGM11));
  TCCR1B = ((1 << WGM12) | (1 << WGM13) | (1 << CS11));
  OCR1A = 65535;
  TIMSK1 |= (1 << ICIE1);   // Enable timer1 input capture interrupt
}

#else // sample PPM using pinchange interrupt
ISR(PPM_Signal_Interrupt)
{
  uint16_t pulseWidth;
  if (!PPM_Signal_Edge_Check) {   // Falling edge detected
    pulseWidth = TCNT1; // read the timer1 value
    TCNT1 = 0; // reset the timer1 value for next
    processPulse(pulseWidth);
  }
}

void setupPPMinput(void)
{
  ppmDetecting = 1;
  ppmMicroPPM = 0;
  // Setup timer1 for input capture (PSC=8 -> 0.5ms precision, top at 20ms)
  TCCR1A = ((1 << WGM10) | (1 << WGM11));
  TCCR1B = ((1 << WGM12) | (1 << WGM13) | (1 << CS11));
  OCR1A = 65535;
  TIMSK1 = 0;
  PPM_Pin_Interrupt_Setup
}
#endif

void bindMode(void)
{
  uint32_t prevsend = millis();
  uint8_t  tx_buf[sizeof(bind_data)+1];
  boolean  sendBinds = 1;

  init_rfm(1);

  while (Serial.available()) {
    Serial.read();    // flush serial
  }

  while (1) {
    if (sendBinds & (millis() - prevsend > 200)) {
      prevsend = millis();
      Green_LED_ON;
      buzzerOn(BZ_FREQ);
      tx_buf[0]='b';
      memcpy(tx_buf+1,&bind_data, sizeof(bind_data));
      tx_packet(tx_buf, sizeof(bind_data)+1);
      Green_LED_OFF;
      buzzerOff();
      RF_Mode = Receive;
      rx_reset();
      delay(50);
      if (RF_Mode == Received) {
        RF_Mode = Receive;
        spiSendAddress(0x7f);   // Send the package read command
        if ('B' == spiReadData()) {
          sendBinds=0;
        }
      }
    }

    if (!digitalRead(BTN)) {
      sendBinds=1;
    }

    while (Serial.available()) {
      switch (Serial.read()) {
      case '\n':
      case '\r':
        Serial.println(F("Enter menu..."));
        handleCLI();
        break;
      case '#':
        scannerMode();
        break;
      case 'B':
        binaryMode();
        break;
      default:
        break;
      }
    }
  }
}

void checkButton(void)
{

  uint32_t time, loop_time;

  if (digitalRead(BTN) == 0) {     // Check the button
    delay(200);   // wait for 200mS with buzzer ON
    buzzerOff();

    time = millis();  //set the current time
    loop_time = time;

    while (millis() < time + 4800) {
      if (digitalRead(BTN)) {
        goto just_bind;
      }
    }

    // Check the button again, If it is still down reinitialize
    if (0 == digitalRead(BTN)) {
      int8_t bzstate = HIGH;
      uint8_t doDefaults = 0;

      buzzerOn(bzstate?BZ_FREQ:0);
      loop_time = millis();

      while (0 == digitalRead(BTN)) {     // wait for button to release
        if (loop_time > time + 9800) {
          buzzerOn(BZ_FREQ);
          doDefaults = 1;
        } else {
          if ((millis() - loop_time) > 200) {
            loop_time = millis();
            bzstate = !bzstate;
            buzzerOn(bzstate?BZ_FREQ:0);
          }
        }
      }

      buzzerOff();
      randomSeed(micros());   // button release time in us should give us enough seed
      if (doDefaults) {
        bindInitDefaults();
      }
      bindRandomize();
      bindWriteEeprom();
      bindPrint();
    }
just_bind:
    // Enter binding mode, automatically after recoding or when pressed for shorter time.
    Serial.println("Entering binding mode\n");
    bindMode();
  }
}

void checkFS(void)
{

  switch (FSstate) {
  case 0:
    if (!digitalRead(BTN)) {
      FSstate = 1;
      FStime = millis();
    }

    break;

  case 1:
    if (!digitalRead(BTN)) {
      if ((millis() - FStime) > 1000) {
        FSstate = 2;
        buzzerOn(BZ_FREQ);
      }
    } else {
      FSstate = 0;
    }

    break;

  case 2:
    if (digitalRead(BTN)) {
      buzzerOff();
      FSstate = 0;
    }

    break;
  }
}

uint8_t tx_buf[21];
uint8_t rx_buf[9];

#define SERIAL_BUFSIZE 32
uint8_t serial_buffer[SERIAL_BUFSIZE];
uint8_t serial_resend[9];
uint8_t serial_head;
uint8_t serial_tail;
uint8_t serial_okToSend; // 2 if it is ok to send serial instead of servo

void setup(void)
{
  setupSPI();

#ifdef SDN_pin
  pinMode(SDN_pin, OUTPUT);  //SDN
  digitalWrite(SDN_pin, 0);
#endif

  //LED and other interfaces
  pinMode(Red_LED, OUTPUT);   //RED LED
  pinMode(Green_LED, OUTPUT);   //GREEN LED
  pinMode(BTN, INPUT);   //Buton

  pinMode(PPM_IN, INPUT);   //PPM from TX
  digitalWrite(PPM_IN, HIGH); // enable pullup for TX:s with open collector output

#if defined (RF_OUT_INDICATOR)
  pinMode(RF_OUT_INDICATOR, OUTPUT);
  digitalWrite(RF_OUT_INDICATOR, LOW);
#endif

  buzzerInit();

  Serial.begin(SERIAL_BAUD_RATE);

  if (bindReadEeprom()) {
    Serial.println("Loaded settings from EEPROM\n");
  } else {
    Serial.print("EEPROM data not valid, reiniting\n");
    bindInitDefaults();
    bindWriteEeprom();
    Serial.print("EEPROM data saved\n");
  }

  setupPPMinput();

  setupRfmInterrupt();

  init_rfm(0);
  rfmSetChannel(bind_data.hopchannel[RF_channel]);

  sei();

  buzzerOn(BZ_FREQ);
  digitalWrite(BTN, HIGH);
  Red_LED_ON ;
  delay(100);

  checkButton();

  Red_LED_OFF;
  buzzerOff();
#ifdef TELEMETRY_BAUD_RATE
  Serial.begin(TELEMETRY_BAUD_RATE);
#endif
  ppmAge = 255;
  rx_reset();

  serial_head=0;
  serial_tail=0;
  serial_okToSend=0;

#ifdef FRSKY_EMULATION
  FrSkyInit();
  lastFrSky = micros();
  Serial.begin(9600);
#endif

}

void loop(void)
{

  if (spiReadRegister(0x0C) == 0) {     // detect the locked module and reboot
    Serial.println("module locked?");
    Red_LED_ON;
    init_rfm(0);
    rx_reset();
    Red_LED_OFF;
  }

  while (Serial.available() && (((serial_tail + 1) % SERIAL_BUFSIZE) != serial_head)) {
    serial_buffer[serial_tail] = Serial.read();
    serial_tail = (serial_tail + 1) % SERIAL_BUFSIZE;
  }

  if (RF_Mode == Received) {
    // got telemetry packet
    lastTelemetry = micros();
    if (!lastTelemetry) {
      lastTelemetry=1;  //fixup rare case of zero
    }
    RF_Mode = Receive;
    spiSendAddress(0x7f);   // Send the package read command
    for (int16_t i = 0; i < 9; i++) {
      rx_buf[i] = spiReadData();
    }

    if ((tx_buf[0] ^ rx_buf[0]) & 0x40) {
      tx_buf[0]^=0x40; // swap sequence to ack
      if ((rx_buf[0] & 0x38) == 0x38) {
        uint8_t i;
        // transparent serial data...
        for (i=0; i<=(rx_buf[0]&7);) {
          i++;
#ifdef FRSKY_EMULATION
          FrSkyUserData(rx_buf[i]);
#else
          Serial.write(rx_buf[i]);
#endif
        }
      } else if ((rx_buf[0] & 0x3F)==0) {
        RSSI_rx = rx_buf[1];
        RX_ain0 = rx_buf[2];
        RX_ain1 = rx_buf[3];
      }
    }
    if (serial_okToSend==1) {
      serial_okToSend=2;
    }
    if (serial_okToSend==3) {
      serial_okToSend=0;
    }
  }

  uint32_t time = micros();

  if ((sampleRSSI) && ((time - sampleRSSI) >= 3000)) {
    RSSI_tx = rfmGetRSSI();
    sampleRSSI=0;
  }

  if ((time - lastSent) >= getInterval(&bind_data)) {
    lastSent = time;

    if (ppmAge < 8) {
      ppmAge++;

      if (lastTelemetry) {
        if ((time - lastTelemetry) > getInterval(&bind_data)) {
          // telemetry lost
          buzzerOn(BZ_FREQ);
          lastTelemetry=0;
        } else {
          // telemetry link re-established
          buzzerOff();
        }
      }

      // Construct packet to be sent
      tx_buf[0] &= 0xc0; //preserve seq. bits
      if ((serial_tail!=serial_head) && (serial_okToSend == 2)) {
        tx_buf[0] ^= 0x80; // signal new data on line
        uint8_t bytes=0;
        uint8_t maxbytes = 8;
        if (getPacketSize(&bind_data) < 9) {
          maxbytes = getPacketSize(&bind_data)-1;
        }
        while ((bytes<maxbytes) && (serial_head!=serial_tail)) {
          bytes++;
          tx_buf[bytes]=serial_buffer[serial_head];
          serial_resend[bytes]=serial_buffer[serial_head];
          serial_head=(serial_head + 1) % SERIAL_BUFSIZE;
        }
        tx_buf[0] |= (0x37 + bytes);
        serial_resend[0]= bytes;
        serial_okToSend = 3; // sent but not acked
      } else if (serial_okToSend == 4) {
        uint8_t i;
        for (i = 0; i < serial_resend[0]; i++) {
          tx_buf[i+1] = serial_resend[i+1];
        }
        tx_buf[0] |= (0x37 + serial_resend[0]);
        serial_okToSend = 3; // sent but not acked
      } else {
        if (FSstate == 2) {
          tx_buf[0] |= 0x01; // save failsafe
          Red_LED_ON
        } else {
          tx_buf[0] |= 0x00; // servo positions
          Red_LED_OFF
          if (serial_okToSend==0) {
            serial_okToSend = 1;
          }
          if (serial_okToSend==3) {
            serial_okToSend = 4;  // resend
          }
        }

        cli(); // disable interrupts when copying servo positions, to avoid race on 2 byte variable
        packChannels(bind_data.flags & 7, PPM, tx_buf + 1);
        sei();

      }

      //Green LED will be on during transmission
      Green_LED_ON ;

      // Send the data over RF
      rfmSetChannel(bind_data.hopchannel[RF_channel]);

      tx_packet(tx_buf, getPacketSize(&bind_data));

      //Hop to the next frequency
      RF_channel++;

      if (RF_channel >= bind_data.hopcount) {
        RF_channel = 0;
      }

      // do not switch channel as we may receive telemetry on the old channel
      if (bind_data.flags & TELEMETRY_ENABLED) {
        RF_Mode = Receive;
        rx_reset();
        // tell loop to sample downlink RSSI
        sampleRSSI=micros();
        if (sampleRSSI==0) {
          sampleRSSI=1;
        }
      }

    } else {
      if (ppmAge == 8) {
        Red_LED_ON
      }

      ppmAge = 9;
      // PPM data outdated - do not send packets
    }

  }

#ifdef FRSKY_EMULATION
  if ((micros()-lastFrSky) > FRSKY_INTERVAL) {
    lastFrSky=micros();
    FrSkySendFrame(RX_ain0,RX_ain1,lastTelemetry?RSSI_rx:0,lastTelemetry?RSSI_tx:0);
  }
#endif


  //Green LED will be OFF
  Green_LED_OFF;

  checkFS();
}

