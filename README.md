openLRSng
=========

my fork of openLRS code (based on thUndeadMod of openLRS)

TRANSMITTER HW:
===============
  - Flytron openLRS M2/M3 TX unit -- set TX_BOARD_TYPE 2

  - OrangeRX UHF TX unit -- set TX_BOARD_TYPE 2
  
  - Flytron openLRS RX v2 / OrangeRX UHF RX / Hawk Eye openLRS RX as TX -- set TX_BOARD_TYPE 3
    - connect PPM input to 5th slot from left (channel 4)
    - button between ground and 4th slot from left (ch3)
    - buzzer via transistor on 3rd slot (ch2) (active high)
    
  - openLRSngTX -- set TX_BOARD_TYPE 4

  - DTFUHF 4ch RX as TX -- set TX_BOARD_TYPE 5

  - Hawk Eye OpenLRSng TX -- set TX_BOARD_TYPE 4

RECEIVER HW:  
============
  - Flytron openLRS RX 
  - OrangeRX UHF RX (NOTE both LEDs are RED!!)
  - Hawk Eye OpenLRS RX
  - DTF UHF 4ch RX
  
  Flytron / OrangeRX / Hawk Eye (RX_BOARD_TYPE 3) default settings:
    RSSI output at 'first' port (marked as RSSI on OrangeRX / ch1 on Flytron) 32kHz PWM signal. To make this analog you can use a simple RC filter (R=10kOhm C=100nF).
    Ports 2-9 are parallel PWM outputs for channels 1-8 (50Hz).
  
  DTF UHF 4ch (RX_BOARD_TYPE 5) default settings:
    CH1-CH4 outputted as PWM (50Hz).

  Receiver pin functiontions can be changed by using the TX CLI system and powering up RX when asked (so it enters config mode).  


SOFTWARE CONFIGURATION:
=======================
  Only hardware related selections are done in the openLRSng.ino.

  Run time configuration is done by connecting to the TX module (which is put into binding mode) with serial terminal. For best restults use real terminal program like Putty, TeraTerm, minicom(Linux) but it is possible to use Arduino Serial Monitor too.
  Sending '<CR>' (enter) will enter the menu which will guide further. It should be noted that doing edits will automatically change 'RF magic' to force rebinding, if you want to use a specific magic set that using the command (and further automatic changes are ceased for the edit session). 

  Datarates are: 0==4800bps 1==9600bps 2==19200 bps
  
UPLOADING:
==========
Use a 3v3 FTDI (or other USB to TTL serial adapter) and Arduino >= 1.0. 

  o set board to "Arduino Pro or Pro Mini (5V, 16MHz) w/ atmega328" (yes it runs really on 3v3 but arduino does not need to know that)

  o define COMPILE_TX and upload to TX module

  o comment out COMPILE_TX and upload to RX


USERS GUIDE
===========

TX:
  - Enter binding mode
    - power up while keeping button down and release button after ~1 second.
      Buzzer should emit short beep ~5 times/s in sync with led.
    - To exit bindmode powercycle TX.
  - Randomize channels and 'magic'
    - power up while keeping button down for ~5 seconds (buzzer starts to emit beeps) and release button
    - binding mode is entered automatically
  - Reset settings and randomize channels and 'magic'
    - power up the TX and keep button down for >~10 seconds (buzzer beeps continously).
    - binding mode is entered automatically
  - Setting failsafe
    - Press and hold button for ~1s during normal operation until red LED lights and buzzer beeps, release button.
  - LEDs
    - Green(or blue) LED is lit when module is transmitting
    - Red LED indicates setting of failsafe, or problem with radio module.
  - Link/RX settings are changed via menu, connect using serial terminal while the TX is in binding mode.

RX:
  - Binding
    - If enabled in the .ino RX always binds at boot (and times out after 0.5s) so it is enough to put TX to bind mode and power up RX.
      On successful bind both red and blue (or a second red on OrangeRX RX) leds light up (and remain lit until TX is put on normal mode)
    - RX will also enter bind mode forcibly (without timeout) if EEPROM data is incorrect or a jumpper is placed between two first outputs (RSSI&CH1 on orange/flytron)
  - Failsafe:
    - Failsafe activates after 1s of no link by default, can be changed via menu (0.1 - 25s)
  - LEDs
    - Blue LED lights when packet is received (losing a single packet shows as no pulse on LED)
    - Red LED indicates trouble, it blinks when two consequent packets are lost, and lights up when more than 2 packets are lost
  - Beacon (if enabled) automatically starts after 'deadtime' with no data from TX, the beacon will send three tone 'FM' modulated signal hearable on PMR channel 1. The signal starts with 500Hz @ 100mW and continues with 250Hz @ 15mW and 166Hz @ 1mW. The degrading signal allows to estimate distance.
    - you can use cheap PMR walkie to listen to this signal and using your body as shield determine the direction of it. Alternatively use a directional 433Mhz antenna.

SPECIAL FUNCTIONS
=================

Both TX and RX can be used as spectrum analysers with the "openLRS spectrum analyser GUI). See http://www.rcgroups.com/forums/showpost.php?p=24549162&postcount=551 http://www.rcgroups.com/forums/showthread.php?t=1617297

TX: Put TX into binding mode and connect with GUI (may need to press update once). 

RX: put jumper on output pins 3 and 4 (CH2&3 by default). This will force the RX to act as spectrum scanner, both LEDs will be off in this mode.
