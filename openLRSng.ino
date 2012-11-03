// **********************************************************
// ************************ openLRSng ***********************
// **********************************************************
// ** by Kari Hautio - kha @ AeroQuad/RCGroups/IRC(Freenode)
//
// This code is based on original OpenLRS and thUndeadMod
//
// This code
// - extend resolution to 10bits (1024 positions)
// - use HW timer in input capture mode for PPM input
// - use HW timer for PPM generation (completely jitterless)
// 
// - collapse everything on single file
//
// Donations for development tools and utilities (beer) here
// https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=DSWGKGKPRX5CS

// **********************************************************
// ************ based on: OpenLRS thUndeadMod ***************
// Mihai Andrian - thUndead http://www.fpvuk.org/forum/index.php?topic=3642.0

// **********************************************************
// *************** based on: OpenLRS Code *******************
// ***  OpenLRS Designed by Melih Karakelle on 2010-2011  ***
// **  an Arudino based RC Rx/Tx system with extra futures **
// **       This Source code licensed under GPL            **
// **********************************************************

// **********************************************************
// ******************** OpenLRS DEVELOPERS ******************
// Mihai Andrian - thUndead http://www.fpvuk.org/forum/index.php?topic=3642.0
// Melih Karakelle (http://www.flytron.com) (forum nick name: Flytron)
// Jan-Dirk Schuitemaker (http://www.schuitemaker.org/) (forum nick name: CrashingDutchman)
// Etienne Saint-Paul (http://www.gameseed.fr) (forum nick name: Etienne)

//#############################
//### CONFIGURATION SECTION ###
//#############################

//####### COMPILATION TARGET #######
// Enable to compile transmitter code, default is RX
//#define COMPILE_TX

//####### TX BOARD TYPE #######
// tbd. 0 = Original M1 Tx Board
// tbd. 1 = OpenLRS Rx Board works as TX
// 2 = Original M2 Tx Board !! NOT TESTED !! Need to add wire between CPU pins 1 and 12!
// 3 = OpenLRS Rx v2 Board works as TX, servo signal to CH5.
#define TX_BOARD_TYPE 3

//####### RX BOARD TYPE #######
// tbd. 1 = OpenLRS Rx Board
// 3 = OpenLRS Rx v2 Board
#define RX_BOARD_TYPE 3

//####### BOOSTER (limit maximum power to 50mW, affects only TX) #######
// 0 = No booster
// 1 = Booster
#define BOOSTER 0

//######### Band Select ##########
// 0 = 433Mhz
// 1 = 459Mhz
#define BAND 0

//######### TRANSMISSION VARIABLES ##########
#define CARRIER_FREQUENCY 435000  //  startup frequency

//###### HOPPING CHANNELS #######
// put only single channel to the list to disable hopping
static unsigned char hop_list[] = {22,10,19,34,49,41};

//###### RF DEVICE ID HEADER #######
// Change this 4 byte values for isolating your transmission,
// RF module accepts only data with same header
static unsigned char RF_Header[4] = {'@','K','H','a'};

//###### SERIAL PORT SPEED - just debugging atm. #######
#define SERIAL_BAUD_RATE 115200 //115.200 baud serial port speed

// RF Data Rate --- choose wisely between range vs. performance

//#define DATARATE 4800 // best range, 20Hz update rate
#define DATARATE 9600 // medium range, 40Hz update rate
//#define DATARATE 19200 // medium range, 50Hz update rate + telemetry backlink

//####################
//### CODE SECTION ###
//####################

#include <Arduino.h>
#include <EEPROM.h>

#if (DATARATE == 4800)
  #define PACKET_INTERVAL 50000 //ms == 20Hz
  #define RFM22REG_6E 0x27
  #define RFM22REG_6F 0x52
  #define RFM22REG_1C 0x1A
  #define RFM22REG_20 0xA1
  #define RFM22REG_21 0x20
  #define RFM22REG_22 0x4E
  #define RFM22REG_23 0xA5
  #define RFM22REG_24 0x00
  #define RFM22REG_25 0x1B
  #define RFM22REG_1D 0x40
  #define RFM22REG_1E 0x0A
  #define RFM22REG_2A 0x1E
#elif (DATARATE == 9600)
  #define PACKET_INTERVAL 25000 //ms == 40Hz
  #define RFM22REG_6E 0x4E
  #define RFM22REG_6F 0xA5
  #define RFM22REG_1C 0x05
  #define RFM22REG_20 0xA1
  #define RFM22REG_21 0x20
  #define RFM22REG_22 0x4E
  #define RFM22REG_23 0xA5
  #define RFM22REG_24 0x00
  #define RFM22REG_25 0x20
  #define RFM22REG_1D 0x40
  #define RFM22REG_1E 0x0A
  #define RFM22REG_2A 0x24
#elif (DATARATE == 19200)
  #define PACKET_INTERVAL 20000 //ms == 50Hz
  #define RFM22REG_6E 0x9D
  #define RFM22REG_6F 0x49
  #define RFM22REG_1C 0x06
  #define RFM22REG_20 0xD0
  #define RFM22REG_21 0x00
  #define RFM22REG_22 0x9D
  #define RFM22REG_23 0x49
  #define RFM22REG_24 0x00
  #define RFM22REG_25 0x7B
  #define RFM22REG_1D 0x40
  #define RFM22REG_1E 0x0A
  #define RFM22REG_2A 0x28
#else
  #error Invalid DATARATE
#endif

#if defined(COMPILE_TX)
  #define BOARD_TYPE TX_BOARD_TYPE
#else
  #define BOARD_TYPE RX_BOARD_TYPE
#endif

//####### Board Pinouts #########

#if (BOARD_TYPE == 0)
  #error NOT SUPPORTED atm.
  #define PPM_IN A5
  #define RF_OUT_INDICATOR A4
  #define BUZZER 9
  #define BTN 10
  #define Red_LED 12
  #define Green_LED 11

  #define Red_LED_ON  PORTB |= _BV(4);
  #define Red_LED_OFF  PORTB &= ~_BV(4);

  #define Green_LED_ON   PORTB |= _BV(3);
  #define Green_LED_OFF  PORTB &= ~_BV(3);

  #define PPM_Pin_Interrupt_Setup  PCMSK1 = 0x20;PCICR|=(1<<PCIE1);
  #define PPM_Signal_Interrupt PCINT1_vect
  #define PPM_Signal_Edge_Check (PINC & 0x20)==0x20

  //## RFM22B Pinouts for Public Edition (M1 or Rx v1)
  #define  nIRQ_1 (PIND & 0x08)==0x08 //D3
  #define  nIRQ_0 (PIND & 0x08)==0x00 //D3

  #define  nSEL_on PORTD |= (1<<4) //D4
  #define  nSEL_off PORTD &= 0xEF //D4

  #define  SCK_on PORTD |= (1<<2) //D2
  #define  SCK_off PORTD &= 0xFB //D2

  #define  SDI_on PORTC |= (1<<1) //C1
  #define  SDI_off PORTC &= 0xFD //C1

  #define  SDO_1 (PINC & 0x01) == 0x01 //C0
  #define  SDO_0 (PINC & 0x01) == 0x00 //C0

  #define SDO_pin A0
  #define SDI_pin A1
  #define SCLK_pin 2
  #define IRQ_pin 3
  #define nSel_pin 4

#endif

#if (BOARD_TYPE == 1)
  #error NOT SUPPORTED atm.
  #define PPM_IN 5
  #define RF_OUT_INDICATOR 6
  #define BUZZER 7
  #define BTN 8

  #define Red_LED A3
  #define Green_LED A2

  #define Red_LED_ON  PORTC &= ~_BV(2);PORTC |= _BV(3);
  #define Red_LED_OFF  PORTC &= ~_BV(2);PORTC &= ~_BV(3);

  #define Green_LED_ON  PORTC &= ~_BV(3);PORTC |= _BV(2);
  #define Green_LED_OFF  PORTC &= ~_BV(3);PORTC &= ~_BV(2);

  #define PPM_Pin_Interrupt_Setup  PCMSK2 = 0x20;PCICR|=(1<<PCIE2);
  #define PPM_Signal_Interrupt PCINT2_vect
  #define PPM_Signal_Edge_Check (PIND & 0x20)==0x20

  //## RFM22B Pinouts for Public Edition (M1 or Rx v1)
  #define  nIRQ_1 (PIND & 0x08)==0x08 //D3
  #define  nIRQ_0 (PIND & 0x08)==0x00 //D3

  #define  nSEL_on PORTD |= (1<<4) //D4
  #define  nSEL_off PORTD &= 0xEF //D4

  #define  SCK_on PORTD |= (1<<2) //D2
  #define  SCK_off PORTD &= 0xFB //D2

  #define  SDI_on PORTC |= (1<<1) //C1
  #define  SDI_off PORTC &= 0xFD //C1

  #define  SDO_1 (PINC & 0x01) == 0x01 //C0
  #define  SDO_0 (PINC & 0x01) == 0x00 //C0

  #define SDO_pin A0
  #define SDI_pin A1
  #define SCLK_pin 2
  #define IRQ_pin 3
  #define nSel_pin 4
#endif

#if (BOARD_TYPE == 2)
  #ifndef COMPILE_TX
  #error TX module cannot be used as RX
  #endif

  #define PPM_IN 8 // NOTE needs hardware hack to connect D8 and  D3 (CPU pins 1 and 12)
  #define RF_OUT_INDICATOR A0
  #define BUZZER 10
  #define BTN 11
  #define Red_LED 13
  #define Green_LED 12

  #define Red_LED_ON  PORTB |= _BV(5);
  #define Red_LED_OFF  PORTB &= ~_BV(5);

  #define Green_LED_ON   PORTB |= _BV(4);
  #define Green_LED_OFF  PORTB &= ~_BV(4);

  //## RFM22B Pinouts for Public Edition (M2)
  #define  nIRQ_1 (PIND & 0x04)==0x04 //D2
  #define  nIRQ_0 (PIND & 0x04)==0x00 //D2

  #define  nSEL_on PORTD |= (1<<4) //D4
  #define  nSEL_off PORTD &= 0xEF //D4

  #define  SCK_on PORTD |= (1<<7) //D7
  #define  SCK_off PORTD &= 0x7F //D7

  #define  SDI_on PORTB |= (1<<0) //B0
  #define  SDI_off PORTB &= 0xFE //B0

  #define  SDO_1 (PINB & 0x02) == 0x02 //B1
  #define  SDO_0 (PINB & 0x02) == 0x00 //B1

  #define SDO_pin 9
  #define SDI_pin 8
  #define SCLK_pin 7
  #define IRQ_pin 2
  #define nSel_pin 4

  #define IRQ_interrupt 0
#endif

#if (BOARD_TYPE == 3)

  #ifdef COMPILE_TX
    #define PPM_IN 8 // ICP1
    #define RF_OUT_INDICATOR 5
    #define BUZZER 6
    #define BTN 7
  #else
    #define PPM_OUT 9 // OCP1A
    #define RSSI_OUT 11 

    #define PWM_1 3
    #define PWM_1_MASK 0x0008 //PD3
    #define PWM_2 5
    #define PWM_2_MASK 0x0020 //PD5
    #define PWM_3 6
    #define PWM_3_MASK 0x0040 //PD6
    #define PWM_4 7
    #define PWM_4_MASK 0x0080 //PD7
    #define PWM_5 8
    #define PWM_5_MASK 0x0100 // PB0
    #define PWM_6 9
    #define PWM_6_MASK 0x0200 // PB1
    #define PWM_7 10
    #define PWM_7_MASK 0x0400 // PB2
    #define PWM_8 12 // note ch9 slot, RSSI at ch8 !!
    #define PWM_8_MASK 0x1000 // PB4

    const unsigned short PWM_MASK[8] = { PWM_1_MASK, PWM_2_MASK, PWM_3_MASK, PWM_4_MASK, PWM_5_MASK, PWM_6_MASK, PWM_7_MASK, PWM_8_MASK };
    #define PWM_ALL_MASK 0x17E8 // all bits used for PWM (logic OR of above)

    #define PWM_MASK_PORTB(x) (((x)>>8) & 0xff)
    #define PWM_MASK_PORTD(x) ((x) & 0xff)

  #endif

  #define Red_LED A3
  #define Green_LED 13

  #define Red_LED_ON  PORTC |= _BV(3);
  #define Red_LED_OFF  PORTC &= ~_BV(3);    // Was originally #define Green_LED_OFF  PORTB |= _BV(5);   E.g turns it ON not OFF

  #define Green_LED_ON  PORTB |= _BV(5);
  #define Green_LED_OFF  PORTB &= ~_BV(5);

  //## RFM22B Pinouts for Public Edition (Rx v2)
  #define  nIRQ_1 (PIND & 0x04)==0x04 //D2
  #define  nIRQ_0 (PIND & 0x04)==0x00 //D2

  #define  nSEL_on PORTD |= (1<<4) //D4
  #define  nSEL_off PORTD &= 0xEF //D4

  #define  SCK_on PORTC |= (1<<2) //A2
  #define  SCK_off PORTC &= 0xFB //A2

  #define  SDI_on PORTC |= (1<<1) //A1
  #define  SDI_off PORTC &= 0xFD //A1

  #define  SDO_1 (PINC & 0x01) == 0x01 //A0
  #define  SDO_0 (PINC & 0x01) == 0x00 //A0

  #define SDO_pin A0
  #define SDI_pin A1
  #define SCLK_pin A2
  #define IRQ_pin 2
  #define nSel_pin 4

  #define IRQ_interrupt 0

#endif

//############ common prototypes ########################

void RF22B_init_parameter(void);
void to_tx_mode(void);
void to_rx_mode(void);
volatile unsigned char tx_buf[11]; // TX buffer
volatile unsigned char rx_buf[11]; // RX buffer

unsigned char RF_channel = 0;

#define PPM_CHANNELS 8
volatile int PPM[PPM_CHANNELS] = { 512,512,512,512,512,512,512,512 };

#ifdef COMPILE_TX

//############ VARIABLES ########################

unsigned char FSstate = 0; // 1 = waiting timer, 2 = send FS, 3 sent waiting btn release
unsigned long FStime = 0;  // time when button went down...

unsigned long lastSent = 0;

volatile unsigned char ppmAge = 0; // age of PPM data


volatile unsigned int startPulse = 0;
volatile byte         ppmCounter = PPM_CHANNELS; // ignore data until first sync pulse

#define TIMER1_FREQUENCY_HZ 50
#define TIMER1_PRESCALER    8
#define TIMER1_PERIOD       (F_CPU/TIMER1_PRESCALER/TIMER1_FREQUENCY_HZ)

/****************************************************
 * Interrupt Vector
 ****************************************************/
ISR(TIMER1_CAPT_vect) {
  unsigned int stopPulse = ICR1;

  // Compensate for timer overflow if needed
  unsigned int pulseWidth = ((startPulse > stopPulse) ? TIMER1_PERIOD : 0) + stopPulse - startPulse;

  if (pulseWidth > 5000) {      // Verify if this is the sync pulse (2.5ms)
    ppmCounter = 0;             // -> restart the channel counter
    ppmAge = 0;                 // brand new PPM data received
  }
  else {
    if (ppmCounter < PPM_CHANNELS) { // extra channels will get ignored here

      int out = ((int)pulseWidth - 1976) / 2; // convert to 0-1023 (1976 - 4024 ; 0.988 - 2.012 ms)
      if (out<0) out=0;
      if (out>1023) out=1023;

      PPM[ppmCounter] = out; // Store measured pulse length
      ppmCounter++;                     // Advance to next channel
    }
  }
  startPulse = stopPulse;         // Save time at pulse start
}

void setupPPMinput() {
  // Setup timer1 for input capture (PSC=8 -> 0.5ms precision, top at 20ms)
  TCCR1A = ((1<<WGM10)|(1<<WGM11));
  TCCR1B = ((1<<WGM12)|(1<<WGM13)|(1<<CS11)|(1<<ICES1));
  OCR1A = TIMER1_PERIOD;
  TIMSK1 |= (1<<ICIE1); // Enable timer1 input capture interrupt
}

void setup() {

  //RF module pins
  pinMode(SDO_pin, INPUT); //SDO
  pinMode(SDI_pin, OUTPUT); //SDI
  pinMode(SCLK_pin, OUTPUT); //SCLK
  pinMode(IRQ_pin, INPUT); //IRQ
  pinMode(nSel_pin, OUTPUT); //nSEL

  //LED and other interfaces
  pinMode(Red_LED, OUTPUT); //RED LED
  pinMode(Green_LED, OUTPUT); //GREEN LED
  pinMode(BUZZER, OUTPUT); //Buzzer
  pinMode(BTN, INPUT); //Buton

  pinMode(PPM_IN, INPUT); //PPM from TX
  pinMode(RF_OUT_INDICATOR, OUTPUT);

  Serial.begin(SERIAL_BAUD_RATE);

  setupPPMinput();

  RF22B_init_parameter();

  sei();

  digitalWrite(BUZZER, HIGH);
  digitalWrite(BTN, HIGH);
  Red_LED_ON ;
  delay(100);

  Check_Button();

  Red_LED_OFF;
  digitalWrite(BUZZER, LOW);

  digitalWrite(RF_OUT_INDICATOR,LOW);

  ppmAge = 255;
  rx_reset();

}


//############ MAIN LOOP ##############
void loop() {

  /* MAIN LOOP */

  if (spiReadRegister(0x0C)==0) // detect the locked module and reboot
  {
    Serial.println("module locked?");
    Red_LED_ON;
    RF22B_init_parameter();
    rx_reset();
    Red_LED_OFF;
  }

  unsigned long time = micros();

  if ((time - lastSent) >= PACKET_INTERVAL) {
    lastSent = time;
    if (ppmAge < 8) {
      ppmAge++;

      // Construct packet to be sent
      if (FSstate == 2) {
        tx_buf[0] = 0xF5; // save failsafe
        Red_LED_ON
      } else {
        tx_buf[0] = 0x5E; // servo positions
        Red_LED_OFF

      }

      cli(); // disable interrupts when copying servo positions, to avoid race on 2 byte variable
      tx_buf[1]=(PPM[0] & 0xff);
      tx_buf[2]=(PPM[1] & 0xff);
      tx_buf[3]=(PPM[2] & 0xff);
      tx_buf[4]=(PPM[3] & 0xff);
      tx_buf[5]=((PPM[0] >> 8) & 3) | (((PPM[1] >> 8) & 3)<<2) | (((PPM[2] >> 8) & 3)<<4) | (((PPM[3] >> 8) & 3)<<6);
      tx_buf[6]=(PPM[4] & 0xff);
      tx_buf[7]=(PPM[5] & 0xff);
      tx_buf[8]=(PPM[6] & 0xff);
      tx_buf[9]=(PPM[7] & 0xff);
      tx_buf[10]=((PPM[4] >> 8) & 3) | (((PPM[5] >> 8) & 3)<<2) | (((PPM[6] >> 8) & 3)<<4) | (((PPM[7] >> 8) & 3)<<6);
     sei();

      //Green LED will be on during transmission
      Green_LED_ON ;

      // Send the data over RF
      to_tx_mode();
      Hopping();//Hop to the next frequency

    } else {
      if (ppmAge==8) {
        Red_LED_ON
      }
      ppmAge=9;
      // PPM data outdated - do not send packets
    }

  }

  //Green LED will be OFF
  Green_LED_OFF;

  checkFS();
}

//############# BUTTON CHECK #################
void Check_Button(void)
{
  unsigned long time,loop_time;

  if (digitalRead(BTN)==0) // Check the button
    {
    delay(1000); // wait for 1000mS when buzzer ON
    digitalWrite(BUZZER, LOW); // Buzzer off

    time = millis();  //set the current time
    loop_time = time;

    while ((digitalRead(BTN)==0) && (loop_time < time + 4000)) {
      // wait for button reelase if it is already pressed.
      loop_time = millis();
    }

    //Check the button again. If it is already pressed start the binding proscedure
    if (digitalRead(BTN)!=0) {
      // if button released, reduce the power for range test.
      spiWriteRegister(0x6d, 0x00);
    }
  }
}


void checkFS(void)
{
  switch (FSstate) {
    case 0:
      if (digitalRead(BTN) == 0) {
        FSstate=1;
        FStime=millis();
      }
      break;
    case 1:
      if (digitalRead(BTN) == 0) {
        if ((millis() - FStime) > 1000) {
          FSstate = 2;
        }
      } else {
        FSstate = 0;
      }
      break;
    case 2:
      if (digitalRead(BTN)) {
        FSstate=0;
      }
      break;
  }
}

#else // COMPILE_RX

volatile unsigned char RF_Mode = 0;
#define Available 0
#define Transmit 1
#define Transmitted 2
#define Receive 3
#define Received 4

unsigned long last_pack_time = 0;

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
      }
      willhop = 1;
    } 
  }

  if (willhop==1) {
    Hopping();//Hop to the next frequency
    willhop =0;
  }

}

#endif


//####### FUNCTIONS #########

//############# FREQUENCY HOPPING ################# thUndead FHSS
void Hopping(void)
{
  RF_channel++;
  if ( RF_channel >= (sizeof(hop_list) / sizeof(hop_list[0])) ) RF_channel = 0;
  spiWriteRegister(0x79, hop_list[RF_channel]);
}

// **********************************************************
// **      RFM22B/Si4432 control functions for OpenLRS     **
// **       This Source code licensed under GPL            **
// **********************************************************

#define NOP() __asm__ __volatile__("nop")

#define RF22B_PWRSTATE_POWERDOWN    0x00
#define RF22B_PWRSTATE_READY        0x01
#define RF22B_PACKET_SENT_INTERRUPT 0x04
#define RF22B_PWRSTATE_RX           0x05
#define RF22B_PWRSTATE_TX           0x09

#define RF22B_Rx_packet_received_interrupt   0x02

unsigned char ItStatus1, ItStatus2;

void spiWriteBit ( unsigned char b );

void spiSendCommand(unsigned char command);
void spiSendAddress(unsigned char i);
unsigned char spiReadData(void);
void spiWriteData(unsigned char i);

unsigned char spiReadRegister(unsigned char address);
void spiWriteRegister(unsigned char address, unsigned char data);

void to_sleep_mode(void);

// **** SPI bit banging functions

void spiWriteBit( unsigned char b ) {
  if (b) {  
    SCK_off;
    NOP();
    SDI_on;
    NOP();
    SCK_on;
    NOP();
  } else {  
    SCK_off;
    NOP();
    SDI_off;
    NOP();
    SCK_on;
    NOP();
  }
}

unsigned char spiReadBit() {
  unsigned char r = 0;
  SCK_on;
  NOP();
  if (SDO_1) {
    r=1;
  }
  SCK_off;
  NOP();
  return r;
}

void spiSendCommand(unsigned char command) {

  nSEL_on;
  SCK_off;
  nSEL_off;
  for (unsigned char n=0; n<8 ; n++) {
    spiWriteBit(command&0x80);
    command = command << 1;
  }
  SCK_off;
}

void spiSendAddress(unsigned char i) {

  spiSendCommand(i & 0x7f);
}

void spiWriteData(unsigned char i) {

  for (unsigned char n=0; n<8; n++) {
    spiWriteBit(i&0x80);
    i = i << 1;
  }
  SCK_off;
}

unsigned char spiReadData(void) {

  unsigned char Result = 0;
  SCK_off;
  for(unsigned char i=0; i<8; i++) { //read fifo data byte
    Result=(Result<<1) + spiReadBit();
  }
  return(Result);
}

unsigned char spiReadRegister(unsigned char address) {
  unsigned char result;
  spiSendAddress(address);
  result = spiReadData();
  nSEL_on;
  return(result);
}

void spiWriteRegister(unsigned char address, unsigned char data) {
  address |= 0x80; // 
  spiSendCommand(address);
  spiWriteData(data);
  nSEL_on;
}

// **** RFM22 access functions

void RF22B_init_parameter(void) {
  
  ItStatus1 = spiReadRegister(0x03); // read status, clear interrupt
  ItStatus2 = spiReadRegister(0x04);
  spiWriteRegister(0x06, 0x00);    // no wakeup up, lbd,
  spiWriteRegister(0x07, RF22B_PWRSTATE_READY);      // disable lbd, wakeup timer, use internal 32768,xton = 1; in ready mode
  spiWriteRegister(0x09, 0x7f);  // c = 12.5p
  spiWriteRegister(0x0a, 0x05);
  spiWriteRegister(0x0b, 0x12);    // gpio0 TX State
  spiWriteRegister(0x0c, 0x15);    // gpio1 RX State

  spiWriteRegister(0x0d, 0xfd);    // gpio 2 micro-controller clk output
  spiWriteRegister(0x0e, 0x00);    // gpio    0, 1,2 NO OTHER FUNCTION.

  spiWriteRegister(0x70, 0x2C);    // disable manchest

  spiWriteRegister(0x6e, RFM22REG_6E);
  spiWriteRegister(0x6f, RFM22REG_6F);
  spiWriteRegister(0x1c, RFM22REG_1C);
  spiWriteRegister(0x20, RFM22REG_20);
  spiWriteRegister(0x21, RFM22REG_21);
  spiWriteRegister(0x22, RFM22REG_22);
  spiWriteRegister(0x23, RFM22REG_23);
  spiWriteRegister(0x24, RFM22REG_24);
  spiWriteRegister(0x25, RFM22REG_25);
  spiWriteRegister(0x1D, RFM22REG_1D);
  spiWriteRegister(0x1E, RFM22REG_1E);
  spiWriteRegister(0x2a, RFM22REG_2A);

  spiWriteRegister(0x30, 0x8c);    // enable packet handler, msb first, enable crc,

  spiWriteRegister(0x32, 0xf3);    // 0x32address enable for headere byte 0, 1,2,3, receive header check for byte 0, 1,2,3
  spiWriteRegister(0x33, 0x42);    // header 3, 2, 1,0 used for head length, fixed packet length, synchronize word length 3, 2,
  spiWriteRegister(0x34, 0x01);    // 7 default value or   // 64 nibble = 32byte preamble
  spiWriteRegister(0x36, 0x2d);    // synchronize word
  spiWriteRegister(0x37, 0xd4);
  spiWriteRegister(0x38, 0x00);
  spiWriteRegister(0x39, 0x00);
  spiWriteRegister(0x3a, RF_Header[0]); // tx header
  spiWriteRegister(0x3b, RF_Header[1]);
  spiWriteRegister(0x3c, RF_Header[2]);
  spiWriteRegister(0x3d, RF_Header[3]);
  spiWriteRegister(0x3e, 11);           // 11 byte normal packet

  //RX HEADER
  spiWriteRegister(0x3f, RF_Header[0]);   // verify header
  spiWriteRegister(0x40, RF_Header[1]);
  spiWriteRegister(0x41, RF_Header[2]);
  spiWriteRegister(0x42, RF_Header[3]);
  spiWriteRegister(0x43, 0xff);    // all the bit to be checked
  spiWriteRegister(0x44, 0xff);    // all the bit to be checked
  spiWriteRegister(0x45, 0xff);    // all the bit to be checked
  spiWriteRegister(0x46, 0xff);    // all the bit to be checked

  #if (BOOSTER == 0)
    spiWriteRegister(0x6d, 0x07); // 7 set power max power
  #else
    spiWriteRegister(0x6d, 0x06); // 6 set power 50mw for booster
  #endif

    spiWriteRegister(0x79, hop_list[0]);    // start channel

  #if (BAND== 0)
    spiWriteRegister(0x7a, 0x06);    // 60khz step size (10khz x value) // no hopping
  #else
    spiWriteRegister(0x7a, 0x05); // 50khz step size (10khz x value) // no hopping
  #endif

  spiWriteRegister(0x71, 0x23); // Gfsk, fd[8] =0, no invert for Tx/Rx data, fifo mode, txclk -->gpio
  spiWriteRegister(0x72, 0x30); // frequency deviation setting to 19.6khz (for 38.4kbps)

  spiWriteRegister(0x73, 0x00);
  spiWriteRegister(0x74, 0x00);    // no offset

  #if (BAND== 0)
    spiWriteRegister(0x75, 0x53);
  #else
    spiWriteRegister(0x75, 0x55);  //450 band
  #endif

  // frequency formulation from Si4432 chip's datasheet
  // original formulation is working with mHz values and floating numbers, I replaced them with kHz values.
  long frequency = CARRIER_FREQUENCY;
  frequency = frequency / 10;
  frequency = frequency - 24000;
  #if (BAND== 0)
  frequency = frequency - 19000; // 19 for 430?439.9 MHz band from datasheet
  #else
  frequency = frequency - 21000; // 21 for 450?459.9 MHz band from datasheet
  #endif
  frequency = frequency * 64; // this is the Nominal Carrier Frequency (fc) value for register setting

  spiWriteRegister(0x76, (byte) (frequency >> 8));
  spiWriteRegister(0x77, (byte) frequency);

}

void to_rx_mode(void)
{
  ItStatus1 = spiReadRegister(0x03);
  ItStatus2 = spiReadRegister(0x04);
  spiWriteRegister(0x07, RF22B_PWRSTATE_READY);
  delay(10);
  rx_reset();
  NOP();
}

void rx_reset(void) {

  spiWriteRegister(0x07, RF22B_PWRSTATE_READY);
  spiWriteRegister(0x7e, 36);    // threshold for rx almost full, interrupt when 1 byte received
  spiWriteRegister(0x08, 0x03);    //clear fifo disable multi packet
  spiWriteRegister(0x08, 0x00);    // clear fifo, disable multi packet
  spiWriteRegister(0x07, RF22B_PWRSTATE_RX );  // to rx mode
  spiWriteRegister(0x05, RF22B_Rx_packet_received_interrupt);
  ItStatus1 = spiReadRegister(0x03);  //read the Interrupt Status1 register
  ItStatus2 = spiReadRegister(0x04);
}


void to_tx_mode(void) {

  // ph +fifo mode
  spiWriteRegister(0x34, 0x06);  // 64 nibble = 32byte preamble
  spiWriteRegister(0x3e, 11);    // total tx 10 byte

  for (unsigned char i=0; i<11; i++) {
    spiWriteRegister(0x7f, tx_buf[i]);
  }

  spiWriteRegister(0x05, RF22B_PACKET_SENT_INTERRUPT);
  ItStatus1 = spiReadRegister(0x03);      //read the Interrupt Status1 register
  ItStatus2 = spiReadRegister(0x04);
  spiWriteRegister(0x07, RF22B_PWRSTATE_TX);    // to tx mode

  while(nIRQ_1);
}

