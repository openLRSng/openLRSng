#if defined(COMPILE_TX)
#define BOARD_TYPE TX_BOARD_TYPE
#else
#define BOARD_TYPE RX_BOARD_TYPE
#endif

//####### Board Pinouts #########

#if (BOARD_TYPE == 0) // Flytron M1 TX
#ifndef COMPILE_TX
#error TX module cannot be used as RX
#endif

#define PPM_IN A5
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

void buzzerInit()
{
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
}

void buzzerOn(uint16_t freq)
{
  if (freq) {
    digitalWrite(BUZZER,HIGH);
  } else {
    digitalWrite(BUZZER,LOW);
  }
}

#define buzzerOff(foo) buzzerOn(0)

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

#define IRQ_interrupt 0
#endif

#if (BOARD_TYPE == 1) // Flytron M1 RX
#ifndef COMPILE_TX
#error M1 RX not verified yet
#endif

#define PPM_IN 5
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

void buzzerInit()
{
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
}

void buzzerOn(uint16_t freq)
{
  if (freq) {
    digitalWrite(BUZZER,HIGH);
  } else {
    digitalWrite(BUZZER,LOW);
  }
}

#define buzzerOff(foo) buzzerOn(0)

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

#define IRQ_interrupt 0
#endif

#if (BOARD_TYPE == 2)
#ifndef COMPILE_TX
#error TX module cannot be used as RX
#endif

#define PPM_IN 3
#define RF_OUT_INDICATOR A0
#define BUZZER 10
#define BTN 11
#define Red_LED 13
#define Green_LED 12

#define Red_LED_ON  PORTB |= _BV(5);
#define Red_LED_OFF  PORTB &= ~_BV(5);

#define Green_LED_ON   PORTB |= _BV(4);
#define Green_LED_OFF  PORTB &= ~_BV(4);

#define PPM_Pin_Interrupt_Setup  PCMSK2 = 0x08;PCICR|=(1<<PCIE2);
#define PPM_Signal_Interrupt PCINT2_vect
#define PPM_Signal_Edge_Check (PIND & 0x08)==0x08

void buzzerInit()
{
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
}

void buzzerOn(uint16_t freq)
{
  if (freq) {
    digitalWrite(BUZZER,HIGH);
  } else {
    digitalWrite(BUZZER,LOW);
  }
}

#define buzzerOff(foo) buzzerOn(0)

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

#define USE_ICP1 // use ICP1 for PPM input for less jitter

#ifdef USE_ICP1
#define PPM_IN 8 // ICP1
#else
#define PPM_IN 3
#define PPM_Pin_Interrupt_Setup  PCMSK2 = 0x08;PCICR|=(1<<PCIE2);
#define PPM_Signal_Interrupt PCINT2_vect
#define PPM_Signal_Edge_Check (PIND & 0x08)==0x08
#endif
#define BUZZER 6
#define BTN 7
void buzzerInit()
{
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
}

void buzzerOn(uint16_t freq)
{
  if (freq) {
    digitalWrite(BUZZER,HIGH);
  } else {
    digitalWrite(BUZZER,LOW);
  }
}

#define buzzerOff(foo) buzzerOn(0)
#else
#define PPM_OUT 9 // OCP1A
#define RSSI_OUT 3 // PD3 OC2B

void setup_RSSI_output()
{
  pinMode(RSSI_OUT, OUTPUT);
  digitalWrite(RSSI_OUT, LOW);
  TCCR2B = (1<<CS20);
  TCCR2A = (1<<WGM20);
}

void set_RSSI_output( uint8_t val )
{
  if ((val == 0) || (val == 255)) {
  TCCR2A &= ~(1<<COM2B1); // disable PWM output
    digitalWrite(RSSI_OUT, (val == 0) ? LOW : HIGH);
  } else {
    OCR2B = val;
    TCCR2A |= (1<<COM2B1);
  }
}

#define PWM_1 5
#define PWM_1_MASK 0x0020 //PD5
#define PWM_2 6
#define PWM_2_MASK 0x0040 //PD6
#define PWM_3 7
#define PWM_3_MASK 0x0080 //PD7
#define PWM_4 8
#define PWM_4_MASK 0x0100 //PB0
#define PWM_5 9
#define PWM_5_MASK 0x0200 // PB1
#define PWM_6 10
#define PWM_6_MASK 0x0400 // PB2
#define PWM_7 11
#define PWM_7_MASK 0x0800 // PB3
#define PWM_8 12
#define PWM_8_MASK 0x1000 // PB4

#define PPM_CH 4 // CH5 slot

const uint16_t PWM_MASK[8] = { PWM_1_MASK, PWM_2_MASK, PWM_3_MASK, PWM_4_MASK, PWM_5_MASK, PWM_6_MASK, PWM_7_MASK, PWM_8_MASK };
#define PWM_ALL_MASK 0x1FE0 // all bits used for PWM (logic OR of above)
#define PWM_WITHPPM_MASK 0x1DE0 // all but the PPM pin

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

#if (BOARD_TYPE == 4) // kha openLRSngTX
#ifndef COMPILE_TX
#error TX module cannot be used as RX
#endif

#define USE_ICP1 // use ICP1 for PPM input for less jitter
#define PPM_IN 8 // ICP1

#define BUZZER 3 // OCR2B
#define BTN A0
#define Red_LED 6
#define Green_LED 5

void buzzerInit()
{
  TCCR2A = (1<<WGM21); // mode=CTC
  TCCR2B = (1<<CS22) | (1<<CS20); // prescaler = 128
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
}

void buzzerOn(uint16_t freq)
{
  if (freq) {
    uint32_t ocr = 125000L / freq;
    if (ocr>255) {
      ocr=255;
    }
    if (!ocr) {
      ocr=1;
    }
    OCR2A = ocr;
    TCCR2A |= (1<<COM2B0); // enable output
  } else {
    TCCR2A &= ~(1<<COM2B0); // disable output
  }
}

#define buzzerOff(foo) buzzerOn(0)

#define Red_LED_ON  PORTD |= _BV(6);
#define Red_LED_OFF  PORTD &= ~_BV(6);

#define Green_LED_ON   PORTD |= _BV(5);
#define Green_LED_OFF  PORTD &= ~_BV(5);

//## RFM22B Pinouts for Public Edition (M2)
#define  nIRQ_1 (PIND & 0x04)==0x04 //D2
#define  nIRQ_0 (PIND & 0x04)==0x00 //D2

#define  nSEL_on PORTD |= (1<<4) //D4
#define  nSEL_off PORTD &= 0xEF //D4

#define  SCK_on  PORTB |= _BV(5)  //B5
#define  SCK_off PORTB &= ~_BV(5) //B5

#define  SDI_on  PORTB |= _BV(3)  //B3
#define  SDI_off PORTB &= ~_BV(3) //B3

#define  SDO_1 (PINB & _BV(4)) == _BV(4) //B4
#define  SDO_0 (PINB & _BV(4)) == 0x00  //B4

#define SDO_pin 12
#define SDI_pin 11
#define SCLK_pin 13
#define IRQ_pin 2
#define nSel_pin 4

#define IRQ_interrupt 0
#endif
