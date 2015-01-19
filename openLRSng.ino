// **********************************************************
// ************************ openLRSng ***********************
// **********************************************************
// ** by Kari Hautio - kha @ AeroQuad/RCGroups/IRC(Freenode)
// ** other commits by cTn-dev, rlboyd, DTFUHF, pwarren
//
// Developer chat at IRC: #openLRS @ freenode
//
// This code is based on original OpenLRS and thUndeadMod
//
// Donations for development tools and utilities (beer) here
// https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=DSWGKGKPRX5CS
//
// Please note that for basic usage there is no need to use this
// code in source form. Instead use configurator program available
// freely in Google Chrome store.
// http://goo.gl/iX7dJx
//
// **********************************************************
// ************ based on: OpenLRS thUndeadMod ***************
// Mihai Andrian - thUndead http://www.fpvuk.org/forum/index.php?topic=3642.0
//
// **********************************************************
// *************** based on: OpenLRS Code *******************
// ***  OpenLRS Designed by Melih Karakelle on 2010-2011  ***
// **  an Arudino based RC Rx/Tx system with extra futures **
// **       This Source code licensed under GPL            **
// **********************************************************

// **********************************************************
// **************** original OpenLRS DEVELOPERS *************
// Mihai Andrian - thUndead http://www.fpvuk.org/forum/index.php?topic=3642.0
// Melih Karakelle (http://www.flytron.com) (forum nick name: Flytron)
// Jan-Dirk Schuitemaker (http://www.schuitemaker.org/) (forum nick name: CrashingDutchman)
// Etienne Saint-Paul (http://www.gameseed.fr) (forum nick name: Etienne)

//################################
//### HW CONFIGURATION SECTION ###
//################################

// NOTE: All settings are made via the CLI or configurator interface at runtime

// To compile with Arduino select TX/RX and BOARD_TYPE setting as needed below

//####### COMPILATION TARGET #######
// Enable to compile transmitter code, default is RX (remove leading //)
//#define COMPILE_TX 0 // compile RX code
//#define COMPILE_TX 1 // compile TX code

//####### TX BOARD TYPE #######
// Enable one of the lines below (remove leading //)
//#define BOARD_TYPE 0 // 0 = Flytron OpenLRS M1 Tx Board (not verified)
//#define BOARD_TYPE 1 // 1 = Flytron OpenLRS M1 Rx Board as TX (not verified)
//#define BOARD_TYPE 2 // 2 = Flytron OpenLRS M2/M3 Tx Board / OrangeRx UHF TX
//#define BOARD_TYPE 3 // 3 = Flytron OpenLRS Rx v2 Board / OrangeRx UHF RX / HawkEye UHF RX (RX and TX supported)
//#define BOARD_TYPE 4 // 4 = OpenLRSngTX / HawkEye UHF TX
//#define BOARD_TYPE 5 // 5 = OpenLRSngRX-4/6ch (DTF UHF/HawkEye) (RX and TX supported)
//#define BOARD_TYPE 6 // 6 = DTF UHF/HawkEye DeluxeTX (Atmega32u4)
//#define BOARD_TYPE 9 // 9 = BroversityRX

//### Module type selection (default = 433, only needed for modified HW)
//#define RFMTYPE 868
//#define RFMTYPE 915

//### Enabled Features (some features can be enabled / disabled with compile flag)
#define CLI // Command-line interface
#define CONFIGURATOR // Phoenix Serial Protocol (required for Configurator to work)

//### DEBUG flags, may be dangerous
//#define TEST_NO_ACK_BY_CH1 // disable sending of acks from RX by channel 1
//#define TEST_HALT_RX_BY_CH2 // freeze RX
//#define TEST_HALT_TX_BY_CH3 // freeze TX
//#define SLAVE_STATISTICS // output master/slave stats on RX serial
//#define DEBUG_DUMP_PPM // dump PPM data on serial (both TX/RX)

//####################
//### CODE SECTION ###
//####################

#include <Arduino.h>

#include "version.h"
#include "binding.h"
#include "hardware.h"
#include "wd.h"
#include "common.h"

#if (COMPILE_TX == 1)
#include "binary_com.h"
#include "rxc.h"
#ifdef CLI_ENABLED
#include "dialog.h"
#endif
#include "frskytx.h"
#include "chpicker.h"
#include "TX.h"
#else
#include "I2C.h"
#include "serialPPM.h"
#include "RX.h"
#endif
