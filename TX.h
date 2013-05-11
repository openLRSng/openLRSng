/****************************************************
 * OpenLRSng transmitter code
 ****************************************************/
uint8_t RF_channel = 0;

uint8_t FSstate = 0; // 1 = waiting timer, 2 = send FS, 3 sent waiting btn release
uint32_t FStime = 0;  // time when button went down...

uint32_t lastSent = 0;

uint32_t lastTelemetry = 0;

volatile uint8_t ppmAge = 0; // age of PPM data


volatile uint16_t startPulse = 0;
volatile uint8_t  ppmCounter = PPM_CHANNELS; // ignore data until first sync pulse

#define TIMER1_FREQUENCY_HZ 50
#define TIMER1_PRESCALER    8
#define TIMER1_PERIOD       (F_CPU/TIMER1_PRESCALER/TIMER1_FREQUENCY_HZ)

#define BZ_FREQ 2000

#ifdef USE_ICP1 // Use ICP1 in input capture mode
/****************************************************
 * Interrupt Vector
 ****************************************************/
ISR(TIMER1_CAPT_vect)
{
  uint16_t stopPulse = ICR1;

  // Compensate for timer overflow if needed
  uint16_t pulseWidth = ((startPulse > stopPulse) ? TIMER1_PERIOD : 0) + stopPulse - startPulse;

  if (pulseWidth > 5000) {      // Verify if this is the sync pulse (2.5ms)
    ppmCounter = 0;             // -> restart the channel counter
    ppmAge = 0;                 // brand new PPM data received
  } else if ((pulseWidth > 1400) && (ppmCounter < PPM_CHANNELS)) {       // extra channels will get ignored here
    PPM[ppmCounter] = servoUs2Bits(pulseWidth / 2);   // Store measured pulse length (converted)
    ppmCounter++;                     // Advance to next channel
  } else {
    ppmCounter = PPM_CHANNELS; // glitch ignore rest of data
  }

  startPulse = stopPulse;         // Save time at pulse start
}

void setupPPMinput()
{
  // Setup timer1 for input capture (PSC=8 -> 0.5ms precision, top at 20ms)
  TCCR1A = ((1 << WGM10) | (1 << WGM11));
  TCCR1B = ((1 << WGM12) | (1 << WGM13) | (1 << CS11) | (1 << ICES1));
  OCR1A = TIMER1_PERIOD;
  TIMSK1 |= (1 << ICIE1);   // Enable timer1 input capture interrupt
}
#else // sample PPM using pinchange interrupt
ISR(PPM_Signal_Interrupt)
{
  uint16_t time_temp;

  if (PPM_Signal_Edge_Check) {   // Only works with rising edge of the signal
    time_temp = TCNT1; // read the timer1 value
    TCNT1 = 0; // reset the timer1 value for next

    if (time_temp > 5000) {   // new frame detection (>2.5ms)
      ppmCounter = 0;             // -> restart the channel counter
      ppmAge = 0;                 // brand new PPM data received
    } else if ((time_temp > 1400) && (ppmCounter < PPM_CHANNELS)) {
      PPM[ppmCounter] = servoUs2Bits(time_temp / 2);   // Store measured pulse length (converted)
      ppmCounter++;                     // Advance to next channel
    } else {
      ppmCounter = PPM_CHANNELS; // glitch ignore rest of data
    }
  }
}

void setupPPMinput(void)
{
  // Setup timer1 for input capture (PSC=8 -> 0.5ms precision, top at 20ms)
  TCCR1A = ((1 << WGM10) | (1 << WGM11));
  TCCR1B = ((1 << WGM12) | (1 << WGM13) | (1 << CS11));
  OCR1A = TIMER1_PERIOD;
  TIMSK1 = 0;
  PPM_Pin_Interrupt_Setup
}
#endif

void bindMode(void)
{
  uint32_t prevsend = millis();
  init_rfm(1);

  while (Serial.available()) {
    Serial.read();    // flush serial
  }

  while (1) {
    if (millis() - prevsend > 200) {
      prevsend = millis();
      Green_LED_ON;
      buzzerOn(BZ_FREQ);
      tx_packet((uint8_t*)&bind_data, sizeof(bind_data));
      Green_LED_OFF;
      buzzerOff();
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
    delay(200);   // wait for 200mS when buzzer ON
    buzzerOff();

    time = millis();  //set the current time
    loop_time = time;

    while ((digitalRead(BTN) == 0) && (loop_time < time + 4800)) {
      // wait for button reelase if it is already pressed.
      loop_time = millis();
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

void setup(void)
{

  //RF module pins
  pinMode(SDO_pin, INPUT);   //SDO
  pinMode(SDI_pin, OUTPUT);   //SDI
  pinMode(SCLK_pin, OUTPUT);   //SCLK
  pinMode(IRQ_pin, INPUT);   //IRQ
  pinMode(nSel_pin, OUTPUT);   //nSEL

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

  attachInterrupt(IRQ_interrupt, RFM22B_Int, FALLING);

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

  ppmAge = 255;
  rx_reset();
  
  Serial.println("pkt size & interval");
  Serial.print(getPacketSize(&bind_data));
  Serial.print(',');
  Serial.println(getInterval(&bind_data));
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

  if (RF_Mode == Received) {
    uint8_t rx_buf[4];
    // got telemetry packet

    lastTelemetry = micros();
    RF_Mode = Receive;
    spiSendAddress(0x7f);   // Send the package read command
    for (int16_t i = 0; i < 4; i++) {
      rx_buf[i] = spiReadData();
    }
    // Serial.println(rx_buf[0]); // print rssi value
  }

  uint32_t time = micros();

  if ((time - lastSent) >= getInterval(&bind_data)) {
    lastSent = time;

    if (ppmAge < 8) {
      uint8_t tx_buf[21];
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
      if (FSstate == 2) {
        tx_buf[0] = 0xF5; // save failsafe
        Red_LED_ON
      } else {
        tx_buf[0] = 0x5E; // servo positions
        Red_LED_OFF

      }

      cli(); // disable interrupts when copying servo positions, to avoid race on 2 byte variable
      packChannels(&bind_data, PPM, tx_buf + 1);
      sei();
      for (int i=0; i < getPacketSize(&bind_data) ; i++) {
        Serial.print(tx_buf[i],16);
        Serial.print(',');
      }
      Serial.println();

      //Green LED will be on during transmission
      Green_LED_ON ;

      // Send the data over RF
      rfmSetChannel(bind_data.hopchannel[RF_channel]);

      {
        uint32_t start=micros();
      tx_packet(tx_buf, getPacketSize(&bind_data));
        Serial.println(micros()-start);
      }

      //Hop to the next frequency
      RF_channel++;

      if (RF_channel >= bind_data.hopcount) {
        RF_channel = 0;
      }

      // do not switch channel as we may receive telemetry on the old channel
      if (bind_data.flags & TELEMETRY_ENABLED) {
        RF_Mode = Receive;
        rx_reset();
      }

    } else {
      if (ppmAge == 8) {
        Red_LED_ON
      }

      ppmAge = 9;
      // PPM data outdated - do not send packets
    }

  }

  //Green LED will be OFF
  Green_LED_OFF;

  checkFS();
}

