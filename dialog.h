/*
  Simple CLI dialog
*/

#define EDIT_BUFFER_SIZE 31

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

  Serial.print(F("7) Channel config:  "));
  Serial.println(chConfStr[bind_data.flags&0x07]);

  Serial.print(F("8) Telemetry:       "));
  Serial.println((bind_data.flags&TELEMETRY_ENABLED)?"Enabled":"Disabled");

  Serial.print(F("Calculated packet interval: "));
  Serial.print(getInterval(&bind_data));
  Serial.print(F(" == "));
  Serial.print(1000000L/getInterval(&bind_data));
  Serial.println(F("Hz"));

}

void rxPrint(void)
{
  uint8_t i,pins;
  Serial.print(F("RX type: "));
  if (rx_config.rx_type == RX_FLYTRON8CH) {
    pins=9;
    Serial.print(F("Flytron/OrangeRX UHF 8ch"));
  } else if (rx_config.rx_type == RX_OLRSNG4CH) {
    pins=4;
    Serial.print(F("OpenLRSngRX mini 4ch"));
  }
  for (i=0; i<pins; i++) {
    Serial.print(i);
    Serial.print(F(") pin CH"));
    Serial.print(i);
    Serial.print(F("function: "));
    if (rx_config.pinmapping[i]<16) {
      Serial.print(F("PWM channel "));
      Serial.println(rx_config.pinmapping[i]);
    } else if (rx_config.pinmapping[i]=PINMAP_PPM) {
      Serial.println(F("PPM output"));
    } else if (rx_config.pinmapping[i]=PINMAP_RSSI) {
      Serial.println(F("RSSI (8kHz PWM) output"));
    } else {
      Serial.println(F("unknown-BUG"));
    }
  }
  Serial.print(F("A) Stop PPM on failsafe : O"));
  Serial.println((rx_config.flags & FAILSAFE_NOPPM)?"N":"FF");
  Serial.print(F("B) Stop PWM on failsafe : O"));
  Serial.println((rx_config.flags & FAILSAFE_NOPWM)?"N":"FF");
  Serial.print(F("C) Failsafe speed       : "));
  Serial.println((rx_config.flags & FAILSAFE_FAST)?"FAST":"SLOW");
  Serial.print(F("D) Failsafe beacon frq. : "));
  if (rx_config.beacon_frequency) {
    Serial.println(rx_config.beacon_frequency);
    Serial.print(F("E) Failsafe beacon delay: "));
    Serial.println(rx_config.beacon_deadtime);
    Serial.print(F("F) Failsafe beacon intv.: "));
    Serial.println(rx_config.beacon_interval);
  } else {
    Serial.println(F("DISABLED"));
  }
}

void CLI_menu_headers(void)
{

  switch (CLI_menu) {
  case -1:
    Serial.write(0x0c); // form feed
    Serial.println(F("\nopenLRSng v2.2"));
    Serial.println(F("Use numbers [0-9] to edit parameters"));
    Serial.println(F("[S] save settings to EEPROM and exit menu"));
    Serial.println(F("[X] revert changes and exit menu"));
    Serial.println(F("[I] renitialize settings to sketch defaults"));
    Serial.println(F("[R] calculate random key and hop list"));
    Serial.println(F("[F] display actual frequencies used"));
    Serial.println(F("[Z] enter receiver configuration utily"));

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
    Serial.println(F("Set Channel config: "));
    Serial.println(F("Valid choices: 1 - 4+4 / 2 - 8 / 3 - 8+4 / 4 - 12 / 5 - 12+4 / 6 - 16"));
    break;
  case 8:
    Serial.println(F("Toggled telemetry!"));
    break;
  }

  // Flush input
  delay(10);
  while (Serial.available()) {
    Serial.read();
  }
}

void RX_menu_headers(void)
{

  switch (CLI_menu) {
  case -1:
    Serial.write(0x0c); // form feed
    Serial.println(F("\nopenLRSng v2.2 - receiver confifurator"));
    Serial.println(F("Use numbers [0-9] to edit outputs A-F for settings"));
    Serial.println(F("[S] save settings to EEPROM and exit RX config"));
    Serial.println(F("[X] aboty changes and exit RX config"));
    Serial.println(F("[I] renitialize settings to defaults"));
    Serial.println();
    rxPrint();
    break;
  case 5:
  case 6:
  case 7:
  case 8:
  case 9:
    if (rx_config.type != RX_FLYTRON8CH) {
      break;
    }
    // Fallthru
  case 1:
  case 2:
  case 3:
  case 4:
    Serial.print(F("Set output for output "));
    Serial.println(CLI_menu);
    Serial.print(F("Valid choices are: [1]-[16] (channel 1-16)"));
    if (rx_config.type==RX_FLYTRON8CH) {
      if (CLI_menu == 9) {
        Serial.print(F(", [0] (RSSI)"));
      }
      if (CLI_menu == 5) {
        Serial.print(F(", [0] (PPM)"));
      }
    } else if (RX_config.type == RX_OLRSNG4CH) {
      switch (CLI_menu) {
      case 1:
        Serial.print(F(", [0] (PPM)"));
        break;
      case 2:
        Serial.print(F(", [0] (RSSI)"));
        break;
      case 3:
        Serial.print(F(", [0] (I2C SCL)"));
        break;
      case 4:
        Serial.print(F(", [0] (I2C SDA)"));
        break;
      }
    }
    Serial.println();
    break;
  case 'A':
  case 'a':
    Serial.println(F("Toggled 'stop PPM'"));
    break;
  case 'B':
  case 'b':
    Serial.println(F("Toggled 'stop PWM'"));
    break;
  case 'C':
  case 'c':
    Serial.println(F("Toggled failsafe speed"));
    break;
  case 'D':
  case 'd':
    Serial.println(F("Set beacon frequency in Hz: 0=disable, Px=PMR channel x, Fx=FRS channel x"));
    break;
  case 'E':
  case 'e':
    Serial.println(F("Set beacon delay"));
    break;
  case 'F':
  case 'f':
    Serial.println(F("Set beacon interval"));
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

void handleRXmenu(char c)
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
      CLI_menu = c - '0';
      CLI_menu_headers();
      break;
    case '8':
    case '8':
      CLI_menu = 8;
      CLI_menu_headers();
      bind_data.flags ^= TELEMETRY_ENABLED;
      CLI_menu = -1;
      CLI_menu_headers();
      break;
    case 'z':
    case 'Z':
      CLI_RX_config();
      CLI_menu = -1;
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
          if ((value >= 1) && (value <= 6)) {
            bind_data.flags &= 0xf8;
            bind_data.flags |= value;
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

void CLI_RX_config()
{
  uint8_t tx_buf[1+sizeof(rx_config)];
  uint32_t last_time = micros();
  Serial.println(F("Connecting to RX, power up the RX (with bind plug if not using always bind)"));
  init_rfm(1);
  while ((RF_Mode==Receive) && ((micros()-last_time)<10000000)) {
    tx_buf[0]='p';
    tx_packet(tx_buf,1);
    delay(200);
    RF_Mode = Receive;
    to_rx_mode();
    Serial.print(".");
  }
  if (RF_Mode == Receive) {
    Serial.println("TIMEOUT");
    return;
  } else {
  }
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
      CLI_menu = c - '0';
      CLI_menu_headers();
      break;
    case '8':
    case '8':
      CLI_menu = 8;
      CLI_menu_headers();
      bind_data.flags ^= TELEMETRY_ENABLED;
      CLI_menu = -1;
      CLI_menu_headers();
      break;
    case 'z':
    case 'Z':
      CLI_RX_config();
      CLI_menu = -1;
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
          if ((value >= 1) && (value <= 6)) {
            bind_data.flags &= 0xf8;
            bind_data.flags |= value;
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
