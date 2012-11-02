openLRSng
=========

my fork of openLRS code (based on thUndeadMod of openLRS)

Please note that this version requires modifications to HW as the used pins 
are different to allow usage of HW timers.

Currently only "RX v2" module is supported as TX and RX.

In theory TX M2 should work too (as TX) but you need to modify it by adding 
a jumpper between CPU pins 1 (PD3) and 12 (PD8) to allow PPM signal to ICP1.


TX (v2 RX as TX):
  - PPM to ch5
  - button between ch4 and ground
  - buzzer driven by ch3 (not used atm.)

RX:
  - PPM output at ch6
  - parallel 8ch PPM on ch1-ch7,ch9
  - RSSI at ch8
  
  
  USERS