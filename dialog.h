/*
  Simple CLI dialog
*/

int8_t CLI_menu = 0;
char CLI_buffer[20];
uint8_t CLI_buffer_needle = 0;

void bindPrint(void)
{

  Serial.print("1) Base frequency:   ");
  Serial.println(bind_data.rf_frequency);
  Serial.print("2) RF magic:         ");
  Serial.println(bind_data.rf_magic, 16);
  Serial.print("3) RF power (0-7):   ");
  Serial.println(bind_data.rf_power);
  Serial.print("4) Channel spacing:  ");
  Serial.println(bind_data.rf_channel_spacing);
  Serial.print("5) Hop channels (");
  Serial.print(bind_data.hopcount);
  Serial.print("): ");

  for (uint8_t c = 0; c < bind_data.hopcount; c++) {
    Serial.print(bind_data.hopchannel[c]);   // max 8 channels
    if (c + 1 != bind_data.hopcount) {
      Serial.print(",");
    }
  }

  Serial.println();
//  Serial.print("NCH: ");
//  Serial.println(bind_data.rc_channels); //normally 8
  Serial.print("6) Baudrate (0-2):   ");
  Serial.println(bind_data.modem_params);
  Serial.print("7) Beacon frequency: ");
  Serial.println(bind_data.beacon_frequency);
  Serial.print("8) Beacon Interval:  ");
  Serial.println(bind_data.beacon_interval);
  Serial.print("9) Beacon Deadtime:  ");
  Serial.println(bind_data.beacon_deadtime);
}

void CLI_menu_headers(void)
{
  Serial.write(0x0c); // form feed

  switch (CLI_menu) {
  case -1:
    Serial.print(F("\nopenLRSng v "));
    Serial.print(1.8);
    Serial.println();
    Serial.println(F("Use numbers [0-9] to edit value"));
    Serial.println(F("[S] save settings to EEPROM and exit menu"));
    Serial.println(F("[X] revert changes and exit menu"));
    Serial.println(F("[I] renitialize settings to sketch defaults"));
    Serial.println(F("[R] calculate random key and hop list"));
    Serial.println(F("[F] display actual frequencies used"));
    Serial.println();

    bindPrint();
    break;
  case 1:
    Serial.print(F("Set base frequency (in Hz): "));
    break;
  case 2:
    Serial.print(F("Set RF magic (HEX): "));
    break;
  case 3:
    Serial.print(F("Set RF power (0-7): "));
    break;
  case 4:
    Serial.print(F("Set channel spacing: "));
    break;
  case 5:
    Serial.print(F("Set Hop channels (separated by coma) [MAX 8]: "));
    break;
  case 6:
    Serial.print(F("Set Baudrate (0-2): "));
    break;
  case 7:
    Serial.print(F("Set Beacon Frequency: "));
    break;
  case 8:
    Serial.print(F("Set Beacon Interval: "));
    break;
  case 9:
    Serial.print(F("Set Beacon Deadtime: "));
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

uint8_t CLI_inline_edit(char c)
{

  if (c == 0x7F) { // Backspace
    if (CLI_buffer_needle > 0) {
      // Remove last char from the buffer
      CLI_buffer_needle--;
      CLI_buffer[CLI_buffer_needle] = 0;

      // Redraw the output
      CLI_menu_headers();

      // Print data stored in the buffer
      for (uint8_t i = 0; i < CLI_buffer_needle; i++) {
        Serial.write(CLI_buffer[i]);
      }
    }
  } else if(c == 0x0D) { // Enter
    return 1; // signal editing done
  } else {
    Serial.write(c);
    CLI_buffer[CLI_buffer_needle++] = c; // Store char in the buffer
  }
  return 0;
}

void CLI_buffer_reset(void)
{
  // Empty buffer and reset needle
  CLI_buffer_needle = 0;
  memset(CLI_buffer, 0, sizeof CLI_buffer);
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
      CLI_menu_headers();
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
      if (CLI_buffer_needle == 0) { // no input - abort
        CLI_menu = -1;
        CLI_menu_headers();
      } else {
        switch (CLI_menu) {
        case 1: {
          uint32_t frq = atol(CLI_buffer);
          if ((frq>MIN_RFM_FREQUENCY) && (frq<MAX_RFM_FREQUENCY)) {
            bind_data.rf_frequency = atol(CLI_buffer);
          } else {
            Serial.println("Invalid frequency entered, discarded\n");
          }
        }
        break;
        case 2:
          // TODO
          break;
        case 3:
          bind_data.rf_power = atoi(CLI_buffer);
          break;
        case 4:
          bind_data.rf_channel_spacing = atoi(CLI_buffer);
          break;
        case 5: {
          char* slice = strtok(CLI_buffer, ",");
          uint8_t channel = 0;
          while (slice != NULL) {
            bind_data.hopchannel[channel++] = atoi(slice);
            slice = strtok(NULL, ",");
          }
          bind_data.hopcount = channel;
        }
        break;
        case 6:
          if ((uint8_t)(atoi(CLI_buffer)) < DATARATE_COUNT) {
            bind_data.modem_params = atoi(CLI_buffer);
          } else {
            Serial.println("Invalid datarate entered, discarded\n");
          }
          break;
        case 7: {
          uint32_t frq = atol(CLI_buffer);
          if ((!frq) || ((frq>MIN_RFM_FREQUENCY) && (frq<MAX_RFM_FREQUENCY))) {
            bind_data.beacon_frequency = atol(CLI_buffer);
          } else {
            Serial.println("Invalid frequency entered, discarded\n");
          }
        }
        break;
        case 8:
          bind_data.beacon_interval = atoi(CLI_buffer);
          break;
        case 9:
          bind_data.beacon_deadtime = atoi(CLI_buffer);
          break;
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
