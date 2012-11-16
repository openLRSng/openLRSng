/****************************************************
 * OpenLRSng receiver code
 ****************************************************/

volatile unsigned char RF_Mode = 0;
#define Available 0
#define Transmit 1
#define Transmitted 2
#define Receive 3
#define Received 4

unsigned long time;
unsigned long last_pack_time = 0;
unsigned long fs_time; // time when failsafe activated

#ifdef FAILSAFE_BEACON
unsigned long last_beacon;
#endif

unsigned char  RSSI_count = 0;
unsigned short RSSI_sum = 0;

int ppmCountter = 0;
int ppmTotal = 0;

unsigned long pwmLastFrame = 0;

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
  } else {
    int  ppmOut = 1976 + PPM[ppmCountter++] * 2; // 0-1023 -> 1976 - 4023 (0.988 - 2.012ms)
    ppmTotal += ppmOut;
    ICR1 = ppmOut;
  }
}

void setupPPMout() {

  TCCR1A = (1<<WGM11)|(1<<COM1A1)|(1<<COM1A0);
  TCCR1B = (1<<WGM13)|(1<<WGM12)|(1<<CS11);
  ICR1 = 40000; // just initial value, will be constantly updated
  OCR1A = 600;  // 0.3ms pulse
  TIMSK1 |= (1<<TOIE1);

  pinMode(PPM_OUT, OUTPUT);
}

void setupPWMout() {

  // Timer mode 0 , counts 0-65535, no output on pins, don't enable interrupts
  TCCR1A = 0;
  TCCR1B = (1<<CS11);

  pinMode(PWM_1, OUTPUT);
  pinMode(PWM_2, OUTPUT);
  pinMode(PWM_3, OUTPUT);
  pinMode(PWM_4, OUTPUT);
  pinMode(PWM_5, OUTPUT);
  pinMode(PWM_6, OUTPUT);
  pinMode(PWM_7, OUTPUT);
  pinMode(PWM_8, OUTPUT);

}

struct pwmstep {
  unsigned short time;
  unsigned short mask;
};

void pulsePWM() {

  struct pwmstep pwmstep[8];
  int steps = 0;
  int done = -1;
  for (int i=0; i<8; i++) {
    int smallest = 5000;
    unsigned short mask = 0;
    for (int j=0; j<8; j++) {
      if ((PPM[j] > done) && (PPM[j] < smallest)) {
        smallest = PPM[j];
        mask = PWM_MASK[j];
      } else if (PPM[j] == smallest) {
        mask |= PWM_MASK[j];
      }
    }
    if (smallest != 5000) {
      done = smallest;
      pwmstep[steps].mask = mask;
      pwmstep[steps].time = 1976 + smallest * 2;
      steps++;
    } else {
      break;
    }
  }

  cli();
  int step = 0;
  TCNT1 = 0;
  PORTB |= PWM_MASK_PORTB(PWM_ALL_MASK);
  PORTD |= PWM_MASK_PORTD(PWM_ALL_MASK);
  while (step < steps) {
    while (TCNT1 < pwmstep[step].time);
    PORTB &= ~PWM_MASK_PORTB(pwmstep[step].mask);
    PORTD &= ~PWM_MASK_PORTD(pwmstep[step].mask);
    step++;
  }
  sei();
}

void save_failsafe_values(void){

  EEPROM.write(11,(PPM[0] & 0xff));
  EEPROM.write(12,(PPM[1] & 0xff));
  EEPROM.write(13,(PPM[2] & 0xff));
  EEPROM.write(14,(PPM[3] & 0xff));
  EEPROM.write(15,(((PPM[0] >> 8) & 3) | (((PPM[1] >> 8) & 3)<<2) | (((PPM[2] >> 8) & 3)<<4) | (((PPM[3] >> 8) & 3)<<6)));
  EEPROM.write(16,(PPM[4] & 0xff));
  EEPROM.write(17,(PPM[5] & 0xff));
  EEPROM.write(18,(PPM[6] & 0xff));
  EEPROM.write(19,(PPM[7] & 0xff));
  EEPROM.write(20,(((PPM[4] >> 8) & 3) | (((PPM[5] >> 8) & 3)<<2) | (((PPM[6] >> 8) & 3)<<4) | (((PPM[7] >> 8) & 3)<<6)));
}

void load_failsafe_values(void){

  unsigned char ee_buf[10];
  for (int i=0; i<10; i++) {
    ee_buf[i]=EEPROM.read(11+i);
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

  // Check for jumpper on ch1 - ch2 (PPM enable).
  PWM_output=1;
  pinMode(PWM_1,OUTPUT);
  digitalWrite(PWM_1, 1);
  digitalWrite(PWM_2, 1); // enable pullup
  delay(10);
  if (digitalRead(PWM_2)) {
    digitalWrite(PWM_1, 0);
    delay(10);
    if (!digitalRead(PWM_2)) {
      PWM_output=0;
    }
  }
  pinMode(PWM_1,INPUT);
  digitalWrite(PWM_1, 0);
  digitalWrite(PWM_2, 0);
  
  if (PWM_output) {
    setupPWMout();
  } else {
    setupPPMout();
  }
  
  attachInterrupt(IRQ_interrupt,RFM22B_Int,FALLING);

//  receiver_mode = check_modes(); // Check the possible jumper positions for changing the receiver mode.

//  load_failsafe_values();   // Load failsafe values on startup

  Red_LED_ON;

  RF22B_init_parameter(); // Configure the RFM22B's registers

  sei();

  //################### RX SYNC AT STARTUP #################
  RF_Mode = Receive;
  to_rx_mode();

  firstpack =0;

}

//############ MAIN LOOP ##############
void loop() {

  if (spiReadRegister(0x0C)==0) { // detect the locked module and reboot
    Serial.println("RX hang");
    RF22B_init_parameter();
    to_rx_mode();
  }

  if (PWM_output) {
    unsigned long time = micros();
    if ((time - pwmLastFrame) >= 20000) {
      pwmLastFrame=time;
      pulsePWM();
    }
  }
  
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
      
    RSSI_sum += spiReadRegister(0x26); // Read the RSSI value
    RSSI_count++;
    rx_reset();

    if (RSSI_count > 20) {
      RSSI_sum /= RSSI_count;
      analogWrite(RSSI_OUT,map(constrain(RSSI_sum,45,120),40,120,0,255));
      RSSI_sum = 0;
      RSSI_count = 0;
    }

    willhop =1;

    Green_LED_OFF;
  }

  if (firstpack) {
    unsigned long time = micros();
    if ((!lostpack) && (time - last_pack_time) > (PACKET_INTERVAL+1000)) {
      // we missed one packet, hop to next channel
      lostpack = 1;
      last_pack_time += PACKET_INTERVAL;
      willhop = 1;
    } else if ((lostpack==1) && (time - last_pack_time) > (PACKET_INTERVAL+1000)) {
      // we lost second packet in row, hop and signal trouble
      lostpack=2;
      last_pack_time += PACKET_INTERVAL;
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
#ifdef FAILSAFE_BEACON
      else if (lostpack == 11) { // failsafes set....
        if ((time - fs_time) > 300000000L) { // 300s == 5 minutes
          lostpack = 12;
          last_beacon = time;
        }
      } else if (lostpack == 12) { // beacon mode active
        if ((time - last_beacon) > 30000000L) { // 30s
          last_beacon=time;
          beacon_send();
          RF22B_init_parameter(); // go back to normal RX 
          rx_reset();
        }
      }
#endif
      willhop = 1;
    } 
  }

  if (willhop==1) {
    Hopping();//Hop to the next frequency
    willhop =0;
  }
}
