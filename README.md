openLRSng
=========

my fork of openLRS code (based on thUndeadMod of openLRS)

TRANSMITTER HW:
===============
  o Flytron openLRS M2/M3 TX unit -- set TX_BOARD_TYPE 2

  o OrangeRX UHF TX unit -- set TX_BOARD_TYPE 2
  
  o Flytron openLRS M2 RX as TX -- set TX_BOARD_TYPE 3
    - connect PPM input to 5th slot (fifth from left )
    - button between ground and ch4 (fourth frem left
    - buzzer at ch3 (active high)
    
  o OrangeRX UHF RX as TX -- set TX_BOARD_TYPE 3
    - connect PPM input to 'ch4' slot
    - button between ground and ch3
    - buzzer at ch2 (active high)

RECEIVER HW:  
============
  o Flytron openLRS RX 
  o OrangeRX UHF
  
  RSSI outputted at 'first' connector (marked as RSSI on OrangeRX) 500Hz PWM signal.
  
  Rest are parallel PWM outputs for channel1-8 (50Hz)
  
  To enable PPM mode connect a jumpper between ch1 and ch2 (second and third column). PPM is outputted on 6th column (ch5).  
  
SOFTWARE CONFIGURATION:
=======================
  Modify configurations on openLRSng.ino as needed, mostly you are intrested in 

  - DEFAULT_CARRIER_FREQUENCY -- sets base frequency
  - DEFAULT_RF_POWER -- limits maximum power

  - DEFAULT_HOPLIST  -- these two parameters bind the tx/rx, note that you can generate random values by using the
  - default_rf_magic -- "randomize channels and magic" feature on TX.
  
UPLOADING:
==========
  Use a 3v3 FTDI (or other USB to TTL serial adapter) and Arduino >= 1.0. 

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
    - power up while keeping button down for ~6 seconds (buzzer starts to emit beeps) and release button
    - binding mode is entered automatically
  - Setting failsafe
    - Press and hold button for ~1s during normal operation until red LED lights and buzzer beeps, release button.
  - LEDs
    - Green(or blue) LED is lit when module is transmitting
    - Red LED indicates setting of failsafe, or problem with radio module.

RX:
  - Binding
    - RX always binds at boot (and timeouts after 0.5s) so it is enough to put TX to bind mode and power up RX.
      On successfull bind blue led lights up (both LEDs remain on until TX is put on normal mode)
    - RX will also enter bind mode forciby (without timeout) if EEPROM data is incorrect or a jumpper is placed between ch7 and ch8
  - Failsafe:
    - Failsafe activates after ~2s of no input data
  - LEDs
    - Blue LED lights when packet is received (losing a single packet shows as no pulse on LED)
    - Red LED indicates trouble, it blinks when two consequent packets are lost, and lights up when more than 2 packets are lost
  - Beacon (if enabled) automatically starts after long enough time with no data from TX, the beecon will send three tone 'FM' modulated signal hearable on PMR channel 1. The signal starts with 500Hz @ 100mW and continues with 250Hz @ 15mW and 166Hz @ 1mW. The degrading signal allows the estimate frequency.
    - you can use cheap PMR walkie to listen to this signal and using your body as shield determine the direction of it. Alternatively use a directional 433Mhz antenna.

