/****************************************************
 * OpenLRSng receiver code
 ****************************************************/

volatile unsigned char RF_Mode = 0;

#define Available 0
#define Transmit 1
#define Transmitted 2
#define Receive 3
#define Received 4

unsigned char RF_channel = 0;

unsigned long time;
unsigned long last_pack_time = 0;
unsigned long last_rssi_time = 0;
unsigned long fs_time; // time when failsafe activated

unsigned long last_beacon;

unsigned char  RSSI_count = 0;
unsigned short RSSI_sum = 0;

int ppmCountter = 0;
int ppmTotal = 0;

boolean PWM_output = 1; // set if parallel PWM output is desired

short firstpack = 0;
short lostpack = 0;

boolean willhop = 0,fs_saved = 0;

void RFM22B_Int()
{
 if (RF_Mode == Transmit)
    {
     RF_Mode = Transmitted;
    }
 if (RF_Mode == Receive)
    {
     RF_Mode = Received;
    }
}

ISR(TIMER1_OVF_vect) {
  if (ppmCountter >= PPM_CHANNELS) {
    ICR1 = 40000 - ppmTotal; // 20ms total frame
    ppmCountter = 0;
    ppmTotal = 0;
    if (PWM_output) { // clear all bits
      PORTB &= ~PWM_MASK_PORTB(PWM_ALL_MASK);
      PORTD &= ~PWM_MASK_PORTD(PWM_ALL_MASK);
    }
  } else {
    int  ppmOut = servoBits2Us(PPM[ppmCountter]) * 2;
    ppmTotal += ppmOut;
    ICR1 = ppmOut;
    if (PWM_output) {
      PORTB &= ~PWM_MASK_PORTB(PWM_ALL_MASK);
      PORTD &= ~PWM_MASK_PORTD(PWM_ALL_MASK);
      PORTB |= PWM_MASK_PORTB(PWM_MASK[ppmCountter]);
      PORTD |= PWM_MASK_PORTD(PWM_MASK[ppmCountter]);
    }
    ppmCountter++;
  }
}

void setupPPMout() {

  if (PWM_output) {
    TCCR1A = (1<<WGM11);
  } else {
    TCCR1A = (1<<WGM11)|(1<<COM1A1)|(1<<COM1A0);
  }
  TCCR1B = (1<<WGM13)|(1<<WGM12)|(1<<CS11);
  ICR1 = 40000; // just initial value, will be constantly updated
  OCR1A = 600;  // 0.3ms pulse
  TIMSK1 |= (1<<TOIE1);

  if (PWM_output) {
    pinMode(PWM_1, OUTPUT);
    pinMode(PWM_2, OUTPUT);
    pinMode(PWM_3, OUTPUT);
    pinMode(PWM_4, OUTPUT);
    pinMode(PWM_5, OUTPUT);
    pinMode(PWM_6, OUTPUT);
    pinMode(PWM_7, OUTPUT);
    pinMode(PWM_8, OUTPUT);
  } else {
    pinMode(PPM_OUT, OUTPUT);
  }
}

#define FAILSAFE_OFFSET 0x80

void save_failsafe_values(void){

  EEPROM.write(FAILSAFE_OFFSET+0,(PPM[0] & 0xff));
  EEPROM.write(FAILSAFE_OFFSET+1,(PPM[1] & 0xff));
  EEPROM.write(FAILSAFE_OFFSET+2,(PPM[2] & 0xff));
  EEPROM.write(FAILSAFE_OFFSET+3,(PPM[3] & 0xff));
  EEPROM.write(FAILSAFE_OFFSET+4,(((PPM[0] >> 8) & 3) | (((PPM[1] >> 8) & 3)<<2) | (((PPM[2] >> 8) & 3)<<4) | (((PPM[3] >> 8) & 3)<<6)));
  EEPROM.write(FAILSAFE_OFFSET+5,(PPM[4] & 0xff));
  EEPROM.write(FAILSAFE_OFFSET+6,(PPM[5] & 0xff));
  EEPROM.write(FAILSAFE_OFFSET+7,(PPM[6] & 0xff));
  EEPROM.write(FAILSAFE_OFFSET+8,(PPM[7] & 0xff));
  EEPROM.write(FAILSAFE_OFFSET+9,(((PPM[4] >> 8) & 3) | (((PPM[5] >> 8) & 3)<<2) | (((PPM[6] >> 8) & 3)<<4) | (((PPM[7] >> 8) & 3)<<6)));
}

void load_failsafe_values(void){

  unsigned char ee_buf[10];
  for (int i=0; i<10; i++) {
    ee_buf[i]=EEPROM.read(FAILSAFE_OFFSET+i);
  }
  PPM[0]= ee_buf[0] + ((ee_buf[4] & 0x03) << 8);
  PPM[1]= ee_buf[1] + ((ee_buf[4] & 0x0c) << 6);
  PPM[2]= ee_buf[2] + ((ee_buf[4] & 0x30) << 4);
  PPM[3]= ee_buf[3] + ((ee_buf[4] & 0xc0) << 2);
  PPM[4]= ee_buf[5] + ((ee_buf[9] & 0x03) << 8);
  PPM[5]= ee_buf[6] + ((ee_buf[9] & 0x0c) << 6);
  PPM[6]= ee_buf[7] + ((ee_buf[9] & 0x30) << 4);
  PPM[7]= ee_buf[8] + ((ee_buf[9] & 0xc0) << 2);
}

int bindReceive(unsigned long timeout) {
  unsigned long start = millis();
  init_rfm(1);
  RF_Mode = Receive;
  to_rx_mode();
  Serial.println("Waiting bind\n");
  while ((!timeout) || ((millis() - start) < timeout)) {
    if(RF_Mode == Received) {  // RFM22B INT pin Enabled by received Data
      Serial.println("Got pkt\n");
      RF_Mode=Receive;
      spiSendAddress(0x7f); // Send the package read command
      for (unsigned char i=0; i < sizeof(bind_data); i++) {
        *(((unsigned char*)&bind_data)+i) = spiReadData();
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

int checkJumpper(unsigned char pin1,unsigned char pin2) {
  int ret=0;
  pinMode(pin1,OUTPUT);
  digitalWrite(pin1, 1);
  digitalWrite(pin2, 1); // enable pullup
  delay(10);
  if (digitalRead(pin2)) {
    digitalWrite(pin1, 0);
    delay(10);
    if (!digitalRead(pin2)) {
      ret=1;
    }
  }
  pinMode(pin1,INPUT);
  digitalWrite(pin1, 0);
  digitalWrite(pin2, 0);
  return ret;
}

void setup() {
  //LEDs
  pinMode(Green_LED, OUTPUT);
  pinMode(Red_LED, OUTPUT);

  //RF module pins
  pinMode(SDO_pin, INPUT); //SDO
  pinMode(SDI_pin, OUTPUT); //SDI
  pinMode(SCLK_pin, OUTPUT); //SCLK
  pinMode(IRQ_pin, INPUT); //IRQ
  pinMode(nSel_pin, OUTPUT); //nSEL

  pinMode(0, INPUT); // Serial Rx
  pinMode(1, OUTPUT);// Serial Tx

  pinMode(RSSI_OUT,OUTPUT);

  Serial.begin(SERIAL_BAUD_RATE); //Serial Transmission
  
  attachInterrupt(IRQ_interrupt,RFM22B_Int,FALLING);
  
  sei();
  Red_LED_ON;

  if (checkJumpper(PWM_7,PWM_8) || (!bindReadEeprom())) {
    Serial.print("EEPROM data not valid or bind jumpper set, forcing bind\n");
    if (bindReceive(0)) {
      bindWriteEeprom();
      Serial.println("Saved bind data to EEPROM\n");
      Green_LED_ON;
    }
  } else {
    #if 1 //ALWAYS_BIND
      if (bindReceive(500)) {
        bindWriteEeprom();
        Serial.println("Saved bind data to EEPROM\n");
        Green_LED_ON;
      }
    #endif
  }

  Serial.println("Entering normal mode\n");

  init_rfm(0); // Configure the RFM22B's registers for normal operation
  RF_channel=0;
  rfmSetChannel(bind_data.hopchannel[RF_channel]);

  // Check for jumpper on ch1 - ch2 (PPM enable).
  if (checkJumpper(PWM_1,PWM_2)) {
    PWM_output=0;
  } else {
    PWM_output=1;
  }
    
  setupPPMout();

  //################### RX SYNC AT STARTUP #################
  RF_Mode = Receive;
  to_rx_mode();

  firstpack =0;

}

//############ MAIN LOOP ##############
void loop() {
  unsigned long time;
  if (spiReadRegister(0x0C)==0) { // detect the locked module and reboot
    Serial.println("RX hang");
    init_rfm(0);
    to_rx_mode();
  }

  time = micros();
  
  if(RF_Mode == Received) {  // RFM22B INT pin Enabled by received Data

    RF_Mode = Receive;

    last_pack_time = micros(); // record last package time
    lostpack=0;

    if (firstpack ==0)  firstpack =1;

    Red_LED_OFF;
    Green_LED_ON;

    spiSendAddress(0x7f); // Send the package read command

    for (int i=0; i<11; i++) {
      rx_buf[i] = spiReadData();
    }

    if ((rx_buf[0] == 0x5E) || (rx_buf[0] == 0xF5)) {
      cli();
      PPM[0]= rx_buf[1] + ((rx_buf[5] & 0x03) << 8);
      PPM[1]= rx_buf[2] + ((rx_buf[5] & 0x0c) << 6);
      PPM[2]= rx_buf[3] + ((rx_buf[5] & 0x30) << 4);
      PPM[3]= rx_buf[4] + ((rx_buf[5] & 0xc0) << 2);
      PPM[4]= rx_buf[6] + ((rx_buf[10] & 0x03) << 8);
      PPM[5]= rx_buf[7] + ((rx_buf[10] & 0x0c) << 6);
      PPM[6]= rx_buf[8] + ((rx_buf[10] & 0x30) << 4);
      PPM[7]= rx_buf[9] + ((rx_buf[10] & 0xc0) << 2);
      sei();
    }
    
    if (rx_buf[0] == 0xF5) {
      if (!fs_saved) {
        save_failsafe_values();
        fs_saved=1;
      }
    } else if (fs_saved) {
      fs_saved=0;
    }
      
    rx_reset();

    willhop =1;

    Green_LED_OFF;
  }

  time = micros();

  // sample RSSI when packet is in the 'air'
  if ((lostpack < 2) && (last_rssi_time!=last_pack_time) &&
      (time - last_pack_time) > (modem_params[bind_data.modem_params].interval - 1500)) {
    last_rssi_time=last_pack_time;
    RSSI_sum += rfmGetRSSI(); // Read the RSSI value
    RSSI_count++;
    if (RSSI_count > 20) {
      RSSI_sum /= RSSI_count;
      analogWrite(RSSI_OUT,map(constrain(RSSI_sum,45,200),40,200,0,255));
      RSSI_sum = 0;
      RSSI_count = 0;
    }
  }

  time = micros();
  if (firstpack) {
    if ((!lostpack) && (time - last_pack_time) > (modem_params[bind_data.modem_params].interval+1000)) {
      // we missed one packet, hop to next channel
      lostpack = 1;
      last_pack_time += modem_params[bind_data.modem_params].interval;
      willhop = 1;
    } else if ((lostpack==1) && (time - last_pack_time) > (modem_params[bind_data.modem_params].interval+1000)) {
      // we lost second packet in row, hop and signal trouble
      lostpack=2;
      last_pack_time += modem_params[bind_data.modem_params].interval;
      willhop = 1;
      Red_LED_ON;
      analogWrite(RSSI_OUT,0);
    } else if ((time - last_pack_time) > 200000L) {
      // hop slowly to allow resync with TX
      last_pack_time = time;
      if (lostpack < 10) {
        lostpack++;
      } else if (lostpack == 10) {
        lostpack=11;
        // Serious trouble, apply failsafe
        load_failsafe_values();
        fs_time=time;
      }
      else if (bind_data.beacon_interval && bind_data.beacon_deadtime &&
               bind_data.beacon_frequency) {
        if (lostpack == 11) { // failsafes set....
          if ((time - fs_time) > (bind_data.beacon_deadtime * 1000000UL)) {
            lostpack = 12;
            last_beacon = time;
          }
        } else if (lostpack == 12) { // beacon mode active
          if ((time - last_beacon) > (bind_data.beacon_interval * 1000000UL)) {
            last_beacon=time;
            beacon_send();
            init_rfm(0); // go back to normal RX 
            rx_reset();
          }
        }
      }
      willhop = 1;
    } 
  }

  if (willhop==1) {
    RF_channel++;
    if ( RF_channel >= bind_data.hopcount ) RF_channel = 0;
    rfmSetChannel(bind_data.hopchannel[RF_channel]);
    willhop =0;
  }
}
