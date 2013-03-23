/*
  Simple CLI dialog
*/

uint8_t CLI_menu = 0;
char CLI_buffer[20];
uint8_t CLI_buffer_needle = 0;

void bindPrint(void)
{

  Serial.print("1) Base frequency: ");
  Serial.println(bind_data.rf_frequency);
  Serial.print("2) RF magic:       ");
  Serial.print(bind_data.rf_magic[0], 16);
  Serial.print(bind_data.rf_magic[1], 16);
  Serial.print(bind_data.rf_magic[2], 16);
  Serial.println(bind_data.rf_magic[3], 16);
  Serial.print("3) RF power (0-7): ");
  Serial.println(bind_data.rf_power);
  Serial.print("4) Number of hops: ");
  Serial.println(bind_data.hopcount);
  Serial.print("5) Hop channels:   ");

  for (uint8_t c = 0; c < bind_data.hopcount; c++) {
    Serial.print(bind_data.hopchannel[c], 16);   // max 8 channels

    if (c + 1 != bind_data.hopcount) {
      Serial.print(":");
    }
  }

  Serial.println();
//  Serial.print("NCH: ");
//  Serial.println(bind_data.rc_channels); //normally 8
  Serial.print("6) Baudrate (0-2): ");
  Serial.println(bind_data.modem_params);
  Serial.print("7) Beacon freq.:   ");
  Serial.println(bind_data.beacon_frequency);
  Serial.print("8) Beacon Interval:");
  Serial.println(bind_data.beacon_interval);
  Serial.print("9) Beacon Deadtime:");
  Serial.println(bind_data.beacon_deadtime);
}

void CLI_initial_screen(void) {
  Serial.write(0x0c); // form feed

  Serial.print(F("openLRSng v "));
  Serial.print(1.8);
  Serial.println();
  Serial.println(F("Use numbers [0-9] to edit value"));
  Serial.println(F("Press [S] to save settings to EEPROM"));
  Serial.println();

  bindPrint();
}

void CLI_menu_1(void) {
  Serial.write(0x0c); // form feed
  Serial.print(F("Set base frequency (in Hz): "));  
}

void CLI_menu_2(void) {
  Serial.write(0x0c); // form feed
  Serial.print(F("Set RF magic: "));  
}

void CLI_menu_3(void) {
  Serial.write(0x0c); // form feed
  Serial.print(F("Set RF power (0-7): "));  
}

void CLI_menu_4(void) {
  Serial.write(0x0c); // form feed
  Serial.print(F("Set number of hops: "));  
}

void CLI_menu_5(void) {
  Serial.write(0x0c); // form feed
  Serial.print(F("Set Hop channels: "));  
}

void CLI_menu_6(void) {
  Serial.write(0x0c); // form feed
  Serial.print(F("Set Baudrate (0-2): "));  
}

void CLI_menu_7(void) {
  Serial.write(0x0c); // form feed
  Serial.print(F("Set Beacon Frequency: "));  
}

void CLI_menu_8(void) {
  Serial.write(0x0c); // form feed
  Serial.print(F("Set Beacon Interval: "));  
}

void CLI_menu_9(void) {
  Serial.write(0x0c); // form feed
  Serial.print(F("Set Beacon Deadtime: "));  
}


void CLI_inline_edit(char c, bool str = 0) {
  if (c == 0x7F) { // Backspace
    if (CLI_buffer_needle > 0) {
      // Remove last char from the buffer
      CLI_buffer_needle--;
      CLI_buffer[CLI_buffer_needle] = 0;
    }
  } else {
    Serial.write(c);
    CLI_buffer[CLI_buffer_needle++] = c; // Store char in the buffer
  }
}

void handleCLI(void)
{
  char c = Serial.read();
  
  if (CLI_menu == 0) {
    switch (c) {
      case '?':
        CLI_initial_screen();
        break;
      case 's':
        // save settings to EEPROM
        
        // TODO
        break;
      case '1':
        CLI_menu = 1;
        CLI_menu_1();
        break;
      case '2':
        CLI_menu = 2;
        CLI_menu_2();
        break;
      case '3':
        CLI_menu = 3;
        CLI_menu_3();
        break;
      case '4':
        CLI_menu = 4;
        CLI_menu_4();
        break;
      case '5':
        CLI_menu = 5;
        CLI_menu_5();
        break;
      case '6':
        CLI_menu = 6;
        CLI_menu_6();
        break;
      case '7':
        CLI_menu = 7;
        CLI_menu_7();
        break;
      case '8':
        CLI_menu = 8;
        CLI_menu_8();
        break;
      case '9':
        CLI_menu = 9;
        CLI_menu_9();
        break;        
    }
  } else { // we are inside the menu (this enables simple inline editing)
    switch (CLI_menu) {
      case 1:
        CLI_inline_edit(c);
        
        if (c == 0x7F) { // Backspace
          CLI_menu_1();
          
          // Print data stored in the buffer
          for (uint8_t i = 0; i < CLI_buffer_needle; i++) {
            Serial.write(CLI_buffer[i]);
          }
        } else if (c == 0x0D) { // Enter
          bind_data.rf_frequency = atoi(CLI_buffer);
          
          // Empty buffer and reset needle
          CLI_buffer_needle = 0;
          memset(CLI_buffer, 0, sizeof CLI_buffer);
          
          // Leave the editing submenu
          CLI_menu = 0;
          CLI_initial_screen();
        }
        break;
      case 2:
        break;
      case 3:
        break;
      case 4:
        break;
      case 5:
        break;
      case 6:
        break;
      case 7:
        break;
      case 8:
        break;
      case 9:
        break;        
    }
  }
}
