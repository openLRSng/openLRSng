/*
  Simple CLI dialog
  
  There should be at lesat 2 variables handling the operations
  1. menu
  2. level
  
*/

uint8_t CLI_menu  = 0;
uint8_t CLI_level = 0;

void CLI_head(void) {
  Serial.print(F("openLRSng v "));
  Serial.print(1.8);
  
  if (CLI_level == 0) {
    Serial.println();  
  } else if (CLI_level == 100) {
    Serial.println(F(" - press [B] to go back to the main menu"));
  } else {
    Serial.println(F(" - press [B] to go back one level"));
  }
}

void handleCLI(char c)
{
  switch (CLI_menu) {
    case 0: // initial screen
      switch (CLI_level) {
        case 0: // level 0
          if (c == '?') {
            Serial.write(0x0c); // form feed
            
            CLI_head();
          } else if (c == '1') { // print binding info
            CLI_level = 100;
            
            CLI_head();
            
            bindPrint();
          }          
          break;
        case 100: // we are in binding info
          if (c == 'b') { // go back to the main menu
            Serial.write(0x0c); // form feed
            
            CLI_level = 0; // reset level
            
            CLI_head();
          }
          break;
      }
      break;
    case 1: // configuration screen
      switch (CLI_level) {
        case 0:
          break;
      }
      break;
  }
  
  /*
  switch (c) {
  case '?':
    bindPrint();
    break;

  case '#':
    buzzerOff();
    scannerMode();
    break;
  }
  */
}
