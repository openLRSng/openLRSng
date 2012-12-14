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
#define COMPILE_TX

//####### TX BOARD TYPE #######
// tbd. 0 = Original M1 Tx Board
// tbd. 1 = OpenLRS Rx Board works as TX
// 2 = Original M2/M3 Tx Board
// 3 = OpenLRS Rx v2 Board works as TX, servo signal to CH5.
#define TX_BOARD_TYPE 3

//####### RX BOARD TYPE #######
// tbd. 1 = OpenLRS Rx Board
// 3 = OpenLRS Rx v2 Board
#define RX_BOARD_TYPE 3

//###### SERIAL PORT SPEED - just debugging atm. #######
#define SERIAL_BAUD_RATE 115200 //115.200 baud serial port speed

// Following can be changed in 'config' mode via serial connection to TX

//####### RADIOLINK RF POWER (beacon is always 100/13/1.3mW) #######
// 7 == 100mW (or 1000mW with M3)
// 6 == 50mW (use this when using booster amp), (800mW with M3)
// 5 == 25mW
// 4 == 13mW
// 3 == 6mW
// 2 == 3mW
// 1 == 1.6mW
// 0 == 1.3mW
#define DEFAULT_RF_POWER 7

//######### TRANSMISSION VARIABLES ##########
#define DEFAULT_CARRIER_FREQUENCY 435000000  // Hz  startup frequency

//###### HOPPING CHANNELS #######
// put only single channel to the list to disable hopping
#define DEFAULT_HOPLIST 22,10,19,34,49,41

//###### RF DEVICE ID HEADER #######
// Change this 4 byte values for isolating your transmission,
// RF module accepts only data with same header
#define DEFAULT_HEADER '@','K','H','a'
static unsigned char RF_Header[4] = {'@','K','H','a'};

// RF Data Rate --- choose wisely between range vs. performance
//  0 -- 4800bps, best range, 20Hz update rate
//  1 -- 9600bps, medium range, 40Hz update rate
//  2 -- 19200bps, medium range, 50Hz update rate + telemetry backlink
#define DEFAULT_DATARATE 0

// Enable RF beacon when link lost for long time...
#define DEFAULT_FAILSAFE_BEACON_ON

// helpper macro for European PMR channels
#define EU_PMR_CH(x) (445993750L + 12500L * x) // valid for ch1-ch8

#define DEFAULT_BEACON_FREQUENCY EU_PMR_CH(1)
#define DEFAULT_BEACON_DEADTIME 30 // time to wait until go into beacon mode (s)
#define DEFAULT_BEACON_INTERVAL 10 // interval between beacon transmits (s)

//####################
//### CODE SECTION ###
//####################

// Frequency sanity checks...
#if ((DEFAULT_CARRIER_FREQUENCY < 413000000) || (DEFAULT_CARRIER_FREQUENCY>453000000))
#error CARRIER_FREQUENCY is invalid
#endif
#if ((DEFAULT_BEACON_FREQUENCY < 413000000) || (DEFAULT_BEACON_FREQUENCY>453000000))
#error BEACON_FREQUENCY is invalid
#endif

#include <Arduino.h>
#include <EEPROM.h>

struct rfm22_modem_regs {
  unsigned long bps;
  unsigned long interval;
  unsigned char r_1c, r_1d, r_1e, r_20, r_21, r_22, r_23, r_24, r_25, r_2a, r_6e, r_6f;
} modem_params[3] = {
  { 4800,  50000, 0x1a, 0x40, 0x0a, 0xa1, 0x20, 0x4e, 0xa5, 0x00, 0x1b, 0x1e, 0x27, 0x52 },
  { 9600,  25000, 0x05, 0x40, 0x0a, 0xa1, 0x20, 0x4e, 0xa5, 0x00, 0x20, 0x24, 0x4e, 0xa5 },
  { 19200, 20000, 0x06, 0x40, 0x0a, 0xd0, 0x00, 0x9d, 0x49, 0x00, 0x7b, 0x28, 0x9d, 0x49 }
};

#include "hardware.h"

//############ common prototypes ########################

void RF22B_init_parameter(void);
void to_tx_mode(void);
void to_rx_mode(void);
volatile unsigned char tx_buf[11]; // TX buffer
volatile unsigned char rx_buf[11]; // RX buffer

unsigned char RF_channel = 0;

#define PPM_CHANNELS 8
volatile int PPM[PPM_CHANNELS] = { 512,512,512,512,512,512,512,512 };

#include "common.h"

#ifdef COMPILE_TX
  #include "TX.h"
#else // COMPILE_RX
  #include "RX.h"
#endif
