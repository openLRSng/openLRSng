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
#define COMPILE_TX

//####### TX BOARD TYPE #######
// 0 = Original Flytron M1 Tx Board (not verified)
// 1 = Original Flytron M1 Rx Board as TX (not verified)
// 2 = Original M2/M3 Tx Board or OrangeRx UHF TX
// 3 = OpenLRS Rx v2 Board works as TX
// 4 = OpenLRSngTX (tbd.)
#define TX_BOARD_TYPE 2

//####### RX BOARD TYPE #######
// 3 = OpenLRS Rx v2 Board or OrangeRx UHF RX
#define RX_BOARD_TYPE 3

//###### SERIAL PORT SPEED - just debugging atm. #######
#define SERIAL_BAUD_RATE 115200 //115.200 baud serial port speed

//###### Should receiver always bind on bootup for 0.5s ######
//###### If disabled a jumpper must be placed on RX ch1-ch2 to allow it to bind
#define RX_ALWAYS_BIND

//### Forced PPM enablingthis will put RX into combined PPM/PWM mode
//### having channels 1-7 available in PWM on slots CH1-CH4,CH6-CH8
//#define FORCED_PPM_OUTPUT

//### Module type selection (only for modified HW)
//#define RFMXX_868

//####################
//### CODE SECTION ###
//####################

#include <Arduino.h>
#include <EEPROM.h>

#include "hardware.h"
#include "binding.h"
#include "common.h"

#ifdef COMPILE_TX
#include "dialog.h"
#include "TX.h"
#else // COMPILE_RX
#include "RX.h"
#endif
