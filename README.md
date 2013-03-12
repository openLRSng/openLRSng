openLRSng
=========

my fork of openLRS code (based on thUndeadMod of openLRS)

TRANSMITTER HW:
===============
  - Flytron openLRS M2/M3 TX unit -- set TX_BOARD_TYPE 2

  - OrangeRX UHF TX unit -- set TX_BOARD_TYPE 2
  
  - Flytron openLRS M2 RX as TX -- set TX_BOARD_TYPE 3
    - connect PPM input to 5th slot (fifth from left )
    - button between ground and ch4 (fourth frem left
    - buzzer at ch3 (active high)
    
  - OrangeRX UHF RX as TX -- set TX_BOARD_TYPE 3
    - connect PPM input to 'ch4' slot
    - button between ground and ch3
    - buzzer at ch2 (active high)

RECEIVER HW:  
============
  - Flytron openLRS RX 
  - OrangeRX UHF RX
  
  RSSI output at 'first' connector (marked as RSSI on OrangeRX) 500Hz PWM signal. To make this analog you can use a simple RC filter (R=10kOhm C=100nF).
  
  CH1-CH8 are parallel PWM outputs for channel1-8 (50Hz)
  
  To enable PPM (combined) mode connect a jumper between CH7-CH8. PWM channels 1-6 are available at CH1-CH4,CH6,CH7(which is jumppered to CH8)
  NOTE: you can make the connection in the AVRISP header (MISO-MOSI) to have servo at CH7 (=channel6)
  
SOFTWARE CONFIGURATION:
=======================
Modify configurations in openLRSng.ino as needed, mostly you are intrested in:

  - DEFAULT_CARRIER_FREQUENCY
    - sets base frequency

  - DEFAULT_RF_POWER
    - limits maximum power

  - DEFAULT_HOPLIST/default_rf_magic
    - these two parameters bind the tx/rx, note that you can generate random values by using the
      "randomize channels and magic" feature on TX.

Note: for settings to take effect the TX must be reinitialised by either randomizing or by resetting to 'factory settings'. The RX will need to be paired again.

  
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
  - Reset settings and randomize channels and 'magic'
    - power up while keeping button down for ~3 seconds (buzzer starts to emit beeps) and release button
    - binding mode is entered automatically
  - Reset settings to .ino values
    - power up the TX and keep button down for >~7 seconds (buzzer beeps continously).
    - binding mode is entered automatically
  - Setting failsafe
    - Press and hold button for ~1s during normal operation until red LED lights and buzzer beeps, release button.
  - LEDs
    - Green(or blue) LED is lit when module is transmitting
    - Red LED indicates setting of failsafe, or problem with radio module.

RX:
  - Binding
    - RX always binds at boot (and times out after 0.5s) so it is enough to put TX to bind mode and power up RX.
      On successful bind blue led lights up (both LEDs remain on until TX is put on normal mode)
    - RX will also enter bind mode forcibly (without timeout) if EEPROM data is incorrect or a jumpper is placed between CH1 and CH2
  - Failsafe:
    - Failsafe activates after ~2s of no input data
  - LEDs
    - Blue LED lights when packet is received (losing a single packet shows as no pulse on LED)
    - Red LED indicates trouble, it blinks when two consequent packets are lost, and lights up when more than 2 packets are lost
  - Beacon (if enabled) automatically starts after 'deadtime' with no data from TX, the beacon will send three tone 'FM' modulated signal hearable on PMR channel 1. The signal starts with 500Hz @ 100mW and continues with 250Hz @ 15mW and 166Hz @ 1mW. The degrading signal allows to estimate distance.
    - you can use cheap PMR walkie to listen to this signal and using your body as shield determine the direction of it. Alternatively use a directional 433Mhz antenna.

SPECIAL FUNCTIONS
======= =========

Both TX and RX can be used as spectrum analysers with the "openLRS spectrum analyser GUI). See http://www.rcgroups.com/forums/showthread.php?t=1617297

TX: Put TX into binding mode and connect with GUI (may need to press update once). 

RX: put jumper on CH3-CH4. This will force the RX to act as spectrum scanner.
