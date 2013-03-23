/*
  Simple CLI dialog
  
  There should be at lesat 2 variables handling the operations
  1. menu
  2. level
  
*/

uint8_t CLI_menu  = 0;
uint8_t CLI_level = 0;

void CLI_head(void) {
  Serial.write(0x0c); // form feed
  Serial.print(F("openLRSng v "));
  Serial.print(1.8); // version should be fed from some version constant/variable in the code
  
  if (CLI_level == 0) {
    Serial.println(F(" - use numbers [0-9] to navigate through the menu"));  
  } else {
    Serial.println(F(" - press [B] to go back one level"));
  }
}

void CLI_initial_screen(void) {
  CLI_head();
  
  Serial.println(F("1 - Print Binding Info"));
  Serial.println(F("2 - Enter Scanner Mode"));
}

void handleCLI(void)
{
  uint8_t c = Serial.read();
  
  if (CLI_menu == 0) {
    if (CLI_level == 0) { 
      if (c == '?') {
        CLI_initial_screen();
      } else if (c == '1') { // Print binding info
        CLI_level = 1;
        
        CLI_head();
        bindPrint();
      } else if (c == '2') { // Enter scanner mode
        buzzerOff();
        scannerMode();
      }
    } else if (CLI_level == 1) {
      if (c == 'b') { // go back to the main menu
        CLI_level = 0; // reset level
        CLI_initial_screen();
      }
    }
  }
}
