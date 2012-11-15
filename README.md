openLRSng
=========

my fork of openLRS code (based on thUndeadMod of openLRS)

Please note that this version requires modifications to HW as the used pins 
are different to allow usage of HW timers.

Currently only "RX v2" module is supported as TX and RX.

In theory TX M2 should work too (as TX) but you need to modify it by adding 
a jumpper between CPU pins 1 (PD3) and 12 (PD8) to allow PPM signal to ICP1.

Power ampfiller support is not there ! (to be added soon)

TX (v2 RX as TX):
  - PPM to ch5
  - button between ch4 and ground
  - buzzer driven by ch3 (not used atm.)

RX:
  - PPM output at ch6
  - parallel 8ch PPM on ch1-ch7,ch9
  - RSSI at ch8

COMPILATION/SETTINGS
====================
1) select TX and RX BOARDTYPES by 
  #define TX_BOARD_TYPE xxx
  #define RX_BOARD_TYPE yyy

2) select wanted band by 
  #define BAND x

3) select wanted base frequency, hopped channels are _above_ this frequency
  #define CARRIER_FREQUENCY 435000

4) select hop frequencies (can be more or less than 6)
  static unsigned char hop_list[] = {22,10,19,34,49,41};

5) select DATARATE (RF rate) e.g. 
  #define DATARATE 9600 // medium range, 40Hz update rate

6) enable/disable the distress beacon as per your liking
  #define FAILSAFE_BEACON

7) define COMPILE_TX and upload to TX module

8) comment out COMPILE_TX define and upload to RX


USERS GUIDE
===========

TX:
  - Setting failsafe
    - Press and hold button for ~1s until red LED lights
  - Rangetest mode
    - power up while keeping button pressed (not implemented) yet).
  - LEDs
    - Green(or blue) LED is lit when module is transmitting
    - Red LED indicates setting of failsafe, or problem with radio module.

RX:
  - Selecting betwwen PWM (paralllel) vs. PPM output:
    - RX defaults to PWM via ch1-ch7,ch9 connectors (ch8 is RSSI). To enable PPM connect a jumpper between ch1 and ch2 signal pins. PPM is outputted on ch6.
  - RSSI output:
    - RSSI is outputted on ch8 (PWM) you may need to add external RC filtter. RSSI pulls down to zero when more than 1 consequent packet is lost.
  - Failsafe:
    - Failsafe activates after ~2s of no input data
  - LEDs
    - Blue LED lights when packet is received (losing a single packet shows as no pulse on LED)
    - Red LED indicates trouble, it blinks when two consequent packets are lost, and lights up when more than 2 packets are lost
  - Beacon (if enabled) automatically starts after long enough time with no data from TX, the beecon will send three tone 'FM' modulated signal hearable on PMR channel 1. The signal starts with 500Hz @ 100mW and continues with 250Hz @ 15mW and 166Hz @ 1mW. The degrading signal allows the estimate frequency.
    - you can use cheap PMR walkie to listen to this signal and using your body as shield determine the direction of it. Alternatively use a directional 433Mhz antenna.

