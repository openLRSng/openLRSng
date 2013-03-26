/*
  Simple CLI dialog
*/

#define EDIT_BUFFER_SIZE 20

int8_t  CLI_menu = 0;
char    CLI_buffer[EDIT_BUFFER_SIZE+1];
uint8_t CLI_buffer_position = 0;
bool    CLI_magic_set = 0;

void bindPrint(void)
{

  Serial.print(F("1) Base frequency:   "));
  Serial.println(bind_data.rf_frequency);

  Serial.print(F("2) RF magic:         "));
  Serial.println(bind_data.rf_magic, 16);

  Serial.print(F("3) RF power (0-7):   "));
  Serial.println(bind_data.rf_power);

  Serial.print(F("4) Channel spacing:  "));
  Serial.println(bind_data.rf_channel_spacing);

  Serial.print(F("5) Hop channels ("));
  Serial.print(bind_data.hopcount);
  Serial.print("): ");
  for (uint8_t c = 0; c < bind_data.hopcount;) {
    Serial.print(bind_data.hopchannel[c++]);   // max 8 channels
    if (c != bind_data.hopcount) {
      Serial.print(",");
    } else {
      Serial.println();
    }
  }

  Serial.print(F("6) Baudrate (0-2):   "));
  Serial.println(bind_data.modem_params);

  Serial.print(F("7) Beacon frequency: "));
  Serial.println(bind_data.beacon_frequency);

  Serial.print(F("8) Beacon Interval:  "));
  Serial.println(bind_data.beacon_interval);

  Serial.print(F("9) Beacon Deadtime:  "));
  Serial.println(bind_data.beacon_deadtime);
}

void CLI_menu_headers(void)
{

  switch (CLI_menu) {
  case -1:
    Serial.write(0x0c); // form feed
    Serial.println(F("\nopenLRSng v2.0"));
    Serial.println(F("Use numbers [0-9] to edit parameters"));
    Serial.println(F("[S] save settings to EEPROM and exit menu"));
    Serial.println(F("[X] revert changes and exit menu"));
    Serial.println(F("[I] renitialize settings to sketch defaults"));
    Serial.println(F("[R] calculate random key and hop list"));
    Serial.println(F("[F] display actual frequencies used"));
    Serial.println();
    bindPrint();
    break;
  case 1:
    Serial.println(F("Set base frequency (in Hz): "));
    break;
  case 2:
    Serial.println(F("Set RF magic (HEX): "));
    break;
  case 3:
    Serial.println(F("Set RF power (0-7): "));
    break;
  case 4:
    Serial.println(F("Set channel spacing: "));
    break;
  case 5:
    Serial.println(F("Set Hop channels (separated by coma) [MAX 8]: "));
    break;
  case 6:
    Serial.println(F("Set Baudrate (0-2): "));
    break;
  case 7:
    Serial.println(F("Set Beacon Frequency: "));
    break;
  case 8:
    Serial.println(F("Set Beacon Interval: "));
    break;
  case 9:
    Serial.println(F("Set Beacon Deadtime: "));
    break;
  }

  // Flush input
  delay(10);
  while (Serial.available()) {
    Serial.read();
  }
}

void showFrequencies()
{
  for (uint8_t ch=0; ch < bind_data.hopcount ; ch++ ) {
    Serial.print("Hop channel ");
    Serial.print(ch);
    Serial.print(" @ ");
    Serial.println(bind_data.rf_frequency + 10000L * bind_data.hopchannel[ch] * bind_data.rf_channel_spacing);
  }
}

void CLI_buffer_reset(void)
{
  // Empty buffer and reset position
  CLI_buffer_position = 0;
  memset(CLI_buffer, 0, sizeof(CLI_buffer));
}

uint8_t CLI_inline_edit(char c)
{
  if (c == 0x7F || c == 0x08) { // Delete or Backspace
    if (CLI_buffer_position > 0) {
      // Remove last char from the buffer
      CLI_buffer_position--;
      CLI_buffer[CLI_buffer_position] = 0;

      // Redraw the output with last character erased
      Serial.write('\r');
      for (uint8_t i = 0; i < CLI_buffer_position; i++) {
        Serial.write(CLI_buffer[i]);
      }
      Serial.write(' ');
      Serial.write('\r');
      for (uint8_t i = 0; i < CLI_buffer_position; i++) {
        Serial.write(CLI_buffer[i]);
      }
    } else {
      Serial.print('\007'); // bell
    }
  } else if (c == 0x1B) { // ESC
    CLI_buffer_reset();
    return 1; // signal editing done
  } else if(c == 0x0D) { // Enter
    return 1; // signal editing done
  } else {
    if (CLI_buffer_position < EDIT_BUFFER_SIZE) {
      Serial.write(c);
      CLI_buffer[CLI_buffer_position++] = c; // Store char in the buffer
    } else {
      Serial.print('\007'); // bell
    }
  }
  return 0;
}

void handleCLImenu(char c)
{
  if (CLI_menu == -1) {
    switch (c) {
    case '\n':
    case '\r':
      CLI_menu_headers();
      break;
    case 's':
    case 'S':
      // save settings to EEPROM
      bindWriteEeprom();
      Serial.println("Settings saved to EEPROM\n");
      // leave CLI
      CLI_menu = -2;
      break;
    case 'x':
    case 'X':
    case 0x1b: //ESC
      // restore settings from EEPROM
      bindReadEeprom();
      Serial.println("Reverted settings from EEPROM\n");
      // leave CLI
      CLI_menu = -2;
      break;
    case 'i':
    case 'I':
      // restore factory settings
      bindInitDefaults();
      Serial.println("Loaded factory defautls\n");

      CLI_menu_headers();
      break;
    case 'r':
    case 'R':
      // randomize channels and key
      bindRandomize();
      Serial.println("Key and channels randomized\n");

      CLI_menu_headers();
      break;
    case 'f':
    case 'F':
      showFrequencies();
      //CLI_menu_headers();
      break;
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      CLI_menu = c - '0';
      CLI_menu_headers();
      break;
    }
  } else { // we are inside the menu
    if (CLI_inline_edit(c)) {
      if (CLI_buffer_position == 0) { // no input - abort
        CLI_menu = -1;
        CLI_menu_headers();
      } else {
        uint32_t value = strtoul(CLI_buffer, NULL, 0);
        bool valid_input = 0;
        switch (CLI_menu) {
        case 1:
          if ((value > MIN_RFM_FREQUENCY) && (value < MAX_RFM_FREQUENCY)) {
            bind_data.rf_frequency = value;
            valid_input = 1;
          }
          break;
        case 2:
          bind_data.rf_magic = value;
          CLI_magic_set = 1; // user wants specific magic, do not auto update
          valid_input = 1;
          break;
        case 3:
          if (value < 8) {
            bind_data.rf_power = value;
            valid_input = 1;
          }
          break;
        case 4:
          if ((value > 0) && (value<11)) {
            bind_data.rf_channel_spacing = value;
            valid_input = 1;
          }
          break;
        case 5: {
          char* slice = strtok(CLI_buffer, ",");
          uint8_t channel = 0;
          while (slice != NULL) {
            if (channel < 8) {
              bind_data.hopchannel[channel++] = atoi(slice);
            }
            slice = strtok(NULL, ",");
          }
          valid_input = 1;
          bind_data.hopcount = channel;
        }
        break;
        case 6:
          if (value < DATARATE_COUNT) {
            bind_data.modem_params = value;
            valid_input = 1;
          }
          break;
        case 7:
          if ((CLI_buffer[0]=='P') || (CLI_buffer[0]=='p')) {
            value = strtoul(CLI_buffer + 1, NULL, 0);
            if ((value) && (value < 9)) {
              value = EU_PMR_CH(value);
            } else {
              value = 1; //invalid
            }
          } else if ((CLI_buffer[0]=='F') || (CLI_buffer[0]=='f')) {
            value = strtoul(CLI_buffer + 1, NULL, 0);
            if ((value) && (value < 8)) {
              value = US_FRS_CH(value);
            } else {
              value = 1; //invalid
            }
          }
          if ((value == 0) || ((value > MIN_RFM_FREQUENCY) && (value < MAX_RFM_FREQUENCY))) {
            bind_data.beacon_frequency = value;
            valid_input = 1;
          }
          break;
        case 8:
          if ((value > 10) && (value < 256)) {
            bind_data.beacon_interval = value;
            valid_input = 1;
          }
          break;
        case 9:
          if ((value > 10) && (value < 256)) {
            bind_data.beacon_deadtime = value;
            valid_input = 1;
          }
          break;
        }
        if (valid_input) {
          if (CLI_magic_set == 0) {
            bind_data.rf_magic++;
          }
        } else {
          Serial.println("\r\nInvalid input - discarded!\007");
        }
        CLI_buffer_reset();
        // Leave the editing submenu
        CLI_menu = -1;
        Serial.println('\n');
        CLI_menu_headers();
      }
    }
  }
}

void handleCLI()
{
  CLI_menu = -1;
  CLI_magic_set = 0;
  CLI_menu_headers();
  while (CLI_menu != -2) { // LOCK user here until settings are saved
    if (Serial.available()) {
      handleCLImenu(Serial.read());
    }
  }

  // Clear buffer
  delay(10);
  while (Serial.available()) {
    Serial.read();
  }
}
