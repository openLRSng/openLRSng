/*
  Simple CLI dialog
*/

#define EDIT_BUFFER_SIZE 100

int8_t  CLI_menu = 0;
char    CLI_buffer[EDIT_BUFFER_SIZE + 1];
uint8_t CLI_buffer_position = 0;
bool    CLI_magic_set = 0;

const static char *chConfStr[8] = { "N/A", "4+4", "8", "8+4", "12", "12+4", "16", "N/A" };

void printYesNo(uint8_t yes)
{
  Serial.println(yes?"Yes":"No");
}

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

  Serial.print(F("5) Hop channels:     "));
  for (uint8_t c = 0; (c < MAXHOPS) && (bind_data.hopchannel[c] != 0); c++) {
    if (c) {
      Serial.print(",");
    }
    Serial.print(bind_data.hopchannel[c]);
  }
  Serial.println();

  Serial.print(F("6) Datarate (0-2):   "));
  Serial.println(bind_data.modem_params);

  Serial.print(F("7) Channel config:  "));
  Serial.println(chConfStr[bind_data.flags & 0x07]);

  Serial.print(F("8) Telemetry:       "));
  switch (bind_data.flags & TELEMETRY_MASK) {
  case TELEMETRY_OFF:
    Serial.println(F("Disabled"));
    break;
  case TELEMETRY_PASSTHRU:
    Serial.println(F("Transparent"));
    break;
  case TELEMETRY_FRSKY:
    Serial.println(F("FrSky"));
    break;
  case TELEMETRY_SMARTPORT:
    Serial.println(F("smartPort"));
    break;
  }

  Serial.print(F("9) Serial baudrate:"));
  Serial.println(bind_data.serial_baudrate);

  Serial.print(F("0) Mute buzzer (mostly):"));
  printYesNo(tx_config.flags & MUTE_TX);

  Serial.print(F("A) Inverted PPM in     :"));
  printYesNo(tx_config.flags & INVERTED_PPMIN);

  Serial.print(F("B) Micro (half) PPM    :"));
  printYesNo(tx_config.flags & MICROPPM);

  Serial.print(F("Calculated packet interval: "));
  Serial.print(getInterval(&bind_data));
  Serial.print(F(" == "));
  Serial.print(1000000L / getInterval(&bind_data));
  Serial.println(F("Hz"));
}

void rxPrintDTime(uint8_t val)
{
  if (!val) {
    Serial.println(F("Disabled"));
  } else {
    uint32_t ms = delayInMs(val) / 100;
    Serial.print(ms / 10);
    Serial.print('.');
    Serial.print(ms % 10);
    Serial.println('s');
  }
}

void rxPrint(void)
{
  uint8_t i;
  Serial.print(F("RX type: "));
  if (rx_config.rx_type == RX_FLYTRON8CH) {
    Serial.println(F("Flytron/OrangeRX UHF 8ch"));
  } else if (rx_config.rx_type == RX_OLRSNG4CH) {
    Serial.println(F("OpenLRSngRX mini 4/6ch"));
  } else if (rx_config.rx_type == RX_DTFUHF10CH) {
    Serial.println(F("DTF UHF 32-bit 10ch"));
  } else if (rx_config.rx_type == RX_PTOWER) {
    Serial.println(F("PowerTower"));
  }
  for (i=0; i < rxcNumberOfOutputs; i++) {
    Serial.print((char)(((i + 1) > 9) ? (i + 'A' - 9) : (i + '1')));
    Serial.print(F(") port "));
    Serial.print(i + 1);
    Serial.print(F("function: "));
    if (rx_config.pinMapping[i] < 32) {
      Serial.print(F("PWM channel "));
      if (rx_config.pinMapping[i] > 15) {
        Serial.print("S");
      }
      Serial.println((rx_config.pinMapping[i] & 0x0f) + 1);
    } else {
      Serial.println(SPECIALSTR(rx_config.pinMapping[i]));
    }
  }
  Serial.print(F("F) Failsafe delay       : "));
  rxPrintDTime(rx_config.failsafeDelay);
  Serial.print(F("G) PPM stop delay       : "));
  rxPrintDTime(rx_config.ppmStopDelay);
  Serial.print(F("H) PWM stop delay       : "));
  rxPrintDTime(rx_config.pwmStopDelay);
  Serial.print(F("I) Failsafe beacon frq. : "));
  if (rx_config.beacon_frequency) {
    Serial.println(rx_config.beacon_frequency);
    Serial.print(F("J) Failsafe beacon delay (0-255 => 10s - 150min): "));
    Serial.println(delayInMsLong(rx_config.beacon_deadtime));
    Serial.print(F("K) Failsafe beacon intv. (1-255s): "));
    Serial.println(rx_config.beacon_interval);
  } else {
    Serial.println(F("DISABLED"));
  }
  Serial.print(F("L) PPM minimum sync (us)  : "));
  Serial.println(rx_config.minsync);
  Serial.print(F("M) PPM RSSI to channel    : "));
  if (rx_config.RSSIpwm < 64) {
    Serial.println(rx_config.RSSIpwm + 1);
  } else {
    Serial.println(F("DISABLED"));
  }
  Serial.print(F("N) PPM output limited     : "));
  Serial.println((rx_config.flags & PPM_MAX_8CH) ? "8ch" : "N/A");
  Serial.print(F("O) Timed BIND at startup  : "));
  printYesNo(rx_config.flags & ALWAYS_BIND);
  Serial.print(F("P) Slave mode (experimental): "));
  printYesNo(rx_config.flags & SLAVE_MODE);
  Serial.print(F("Q) Output before link (=FS) : "));
  printYesNo(rx_config.flags & IMMEDIATE_OUTPUT);
}

void CLI_menu_headers(void)
{

  switch (CLI_menu) {
  case -1:
    Serial.println(F("\n\nopenLRSng "));
    printVersion(version);
    Serial.println(F(" - System configuration"));
    Serial.println(F("Use numbers [0-9] to edit parameters"));
    Serial.println(F("[S] save settings to EEPROM and exit menu"));
    Serial.println(F("[X] revert changes and exit menu"));
    Serial.println(F("[I] reinitialize settings to sketch defaults"));
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
    Serial.println(F("Set RF magic (hex) e.g. 0xDEADF00D: "));
    break;
  case 3:
    Serial.println(F("Set RF power (0-7): "));
    break;
  case 4:
    Serial.println(F("Set channel spacing (x10kHz): "));
    break;
  case 5:
    Serial.println(F("Set Hop channels (max 24, separated by commas) valid values 1-255: "));
    break;
  case 6:
    Serial.println(F("Set Datarate (0-2): "));
    break;
  case 7:
    Serial.println(F("Set Channel config: "));
    Serial.println(F("Valid choices: 1 - 4+4 / 2 - 8 / 3 - 8+4 / 4 - 12 / 5 - 12+4 / 6 - 16"));
    break;
  case 9:
    Serial.println(F("Set serial baudrate: "));
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
  uint8_t ch;
  switch (CLI_menu) {
  case -1:
    Serial.print(F("\n\nopenLRSng "));
    printVersion(version);
    Serial.print(F(" - receiver configurator, rx sw "));
    printVersion(rxcVersion);
    Serial.println(F("Use numbers [1-D] to edit ports [E-Q] for settings"));
    Serial.println(F("[R] revert RX settings to defaults"));
    Serial.println(F("[S] save settings to RX"));
    Serial.println(F("[X] abort changes and exit RX config"));
    Serial.println();
    rxPrint();
    break;
  default:
    if ((CLI_menu > 0) && (CLI_menu <= rxcNumberOfOutputs)) {
      Serial.print(F("Set output for port "));
      Serial.println(CLI_menu);
      Serial.print(F("Valid choices are: [1]-[16] (channel 1-16)"));
      ch=40;
      for (uint8_t i = 0; i < rxcSpecialPinCount; i++) {
        if (rxcSpecialPins[i].output == CLI_menu - 1) {
          Serial.print(", [");
          Serial.print(ch);
          Serial.print("] (");
          Serial.print(SPECIALSTR(rxcSpecialPins[i].type));
          Serial.print(")");
          ch++;
        }
      }
      Serial.println();
    }
    break;
  }
}

void showFrequencies()
{
  for (uint8_t ch = 0; (ch < MAXHOPS) && (bind_data.hopchannel[ch] != 0) ; ch++) {
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
  } else if (c == 0x0D || c == 0x0A) { // Enter/Newline
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
  uint8_t ch;
  if (CLI_menu == -1) {
    switch (c) {
    case '!':
    case '@':
      Serial.println("NOT ENABLED");
      break;
    case '\n':
    case '\r':
      RX_menu_headers();
      break;
    case 's':
    case 'S': {
      Serial.println("Sending settings to RX\n");
      uint8_t tx_buf[1 + sizeof(rx_config)];
      tx_buf[0] = 'u';
      memcpy(tx_buf + 1, &rx_config, sizeof(rx_config));
      tx_packet(tx_buf, sizeof(rx_config) + 1);
      rx_reset();
      RF_Mode = Receive;
      delay(200);
      if (RF_Mode == Received) {
        spiSendAddress(0x7f);   // Send the package read command
        tx_buf[0] = spiReadData();
        if (tx_buf[0] == 'U') {
          Serial.println(F("*****************************"));
          Serial.println(F("RX Acked - update successful!"));
          Serial.println(F("*****************************"));
        }
      }
    }
    break;
    case 'r':
    case 'R': {
      Serial.println("Resetting settings on RX\n");
      uint8_t tx_buf[1 + sizeof(rx_config)];
      tx_buf[0] = 'i';
      tx_packet(tx_buf, 1);
      rx_reset();
      RF_Mode = Receive;
      delay(200);
      if (RF_Mode == Received) {
        spiSendAddress(0x7f); // Send the package read command
        tx_buf[0] = spiReadData();
        for (uint8_t i = 0; i < sizeof(rx_config); i++) {
          tx_buf[i + 1] = spiReadData();
        }
        memcpy(&rx_config, tx_buf + 1, sizeof(rx_config));
        if (tx_buf[0]=='I') {
          Serial.println(F("*****************************"));
          Serial.println(F("RX Acked - revert successful!"));
          Serial.println(F("*****************************"));
        }
      }
    }
    break;
    case 'x':
    case 'X':
    case 0x1b: //ESC
      // restore settings from EEPROM
      Serial.println("Aborted edits\n");
      // leave CLI
      CLI_menu = -2;
      break;
    case 'a':
    case 'b':
    case 'c':
    case 'd':
      c -= 'a' - 'A';
      // Fallthru
    case 'A':
    case 'B':
    case 'C':
    case 'D':
      c -= 'A' - 10 - '0';
      // Fallthru
    case '9':
    case '8':
    case '7':
    case '6':
    case '5':
    case '4':
    case '3':
    case '2':
    case '1':
      c -= '0';
      if ( c > rxcNumberOfOutputs) {
        Serial.println("invalid selection");
        break;
      }
      CLI_menu = c;
      RX_menu_headers();
      break;
    case 'f':
    case 'F':
      CLI_menu = 20;
      Serial.println(F("Set failsafe delay (0 disabled, 1-255 == 0.1s - 50min)"));
      break;
    case 'g':
    case 'G':
      CLI_menu = 21;
      Serial.println(F("Set PPM stop delay (0 disabled, 1-255 == 0.1s - 50min)"));
      break;
    case 'h':
    case 'H':
      CLI_menu = 22;
      Serial.println(F("Set PWM stop delay (0 disabled, 1-255 == 0.1s - 50min)"));
      break;
    case 'i':
    case 'I':
      CLI_menu = 23;
      Serial.println(F("Set beacon frequency in Hz: 0=disable, Px=PMR channel x, Fx=FRS channel x"));
      break;
    case 'j':
    case 'J':
      CLI_menu = 24;
      Serial.println(F("Set beacon delay"));
      break;
    case 'k':
    case 'K':
      CLI_menu = 25;
      Serial.println(F("Set beacon interval"));
      break;
    case 'l':
    case 'L':
      CLI_menu = 26;
      Serial.println(F("Set PPM minimum sync"));
      break;
    case 'm':
    case 'M':
      CLI_menu = 27;
      Serial.println(F("Set RSSI injection channel (0==disable)"));
      break;
    case 'n':
    case 'N':
      Serial.println(F("Toggled PPM channel limit"));
      rx_config.flags ^= PPM_MAX_8CH;
      CLI_menu = -1;
      RX_menu_headers();
      break;
    case 'o':
    case 'O':
      Serial.println(F("Toggled 'always bind'"));
      rx_config.flags ^= ALWAYS_BIND;
      CLI_menu = -1;
      RX_menu_headers();
      break;
    case 'p':
    case 'P':
      Serial.println(F("Toggled 'slave mode'"));
      rx_config.flags ^= SLAVE_MODE;
      CLI_menu = -1;
      RX_menu_headers();
      break;
    case 'q':
    case 'Q':
      Serial.println(F("Toggled 'immediate output'"));
      rx_config.flags ^= IMMEDIATE_OUTPUT;
      CLI_menu = -1;
      RX_menu_headers();
      break;
    }
    while (Serial.available()) {
      Serial.read();
    }
  } else { // we are inside the menu
    if (CLI_inline_edit(c)) {
      if (CLI_buffer_position == 0) { // no input - abort
        CLI_menu = -1;
        RX_menu_headers();
      } else {
        uint32_t value = strtoul(CLI_buffer, NULL, 0);
        bool valid_input = 0;
        switch (CLI_menu) {
        case 13:
        case 12:
        case 11:
        case 10:
        case 9:
        case 8:
        case 7:
        case 6:
        case 5:
        case 4:
        case 3:
        case 2:
        case 1:
          if (CLI_menu > rxcNumberOfOutputs) {
            break;
          }
          if ((value > 0) && (value <= 32)) {
            rx_config.pinMapping[CLI_menu - 1] = value - 1;
            valid_input = 1;
          } else {
            ch=40;
            for (uint8_t i = 0; i < rxcSpecialPinCount; i++) {
              if (rxcSpecialPins[i].output != (CLI_menu - 1)) {
                continue;
              }
              if (ch == value) {
                rx_config.pinMapping[CLI_menu - 1] = rxcSpecialPins[i].type;
                valid_input = 1;
              }
              ch++;
            }
          }
          break;
        case 20:
          if (value <= 255) {
            rx_config.failsafeDelay = value;
            valid_input = 1;
          }
          break;
        case 21:
          if (value <= 255) {
            rx_config.ppmStopDelay = value;
            valid_input = 1;
          }
          break;
        case 22:
          if (value <= 255) {
            rx_config.pwmStopDelay = value;
            valid_input = 1;
          }
          break;
        case 23:
          if ((CLI_buffer[0] | 0x20) == 'p') {
            value = strtoul(CLI_buffer + 1, NULL, 0);
            if ((value >= 1) && (value <= 8)) {
              value=EU_PMR_CH(value);
            } else {
              value = 1; //invalid
            }
          } else if ((CLI_buffer[0] | 0x20) == 'f') {
            value = strtoul(CLI_buffer + 1, NULL, 0);
            if ((value >= 1) && (value <= 7)) {
              value = US_FRS_CH(value);
            } else {
              value = 1; //invalid
            }
          }
          if ((value == 0) || ((value >= MIN_RFM_FREQUENCY) && (value <= MAX_RFM_FREQUENCY))) {
            rx_config.beacon_frequency = value;
            valid_input = 1;
          }
          break;
        case 24:
          if ((value >= MIN_DEADTIME) && (value <= MAX_DEADTIME)) {
            rx_config.beacon_deadtime = value;
            valid_input = 1;
          }
          break;
        case 25:
          if ((value >= MIN_INTERVAL) && (value <= MAX_INTERVAL)) {
            rx_config.beacon_interval = value;
            valid_input = 1;
          }
          break;
        case 26:
          if ((value >= 2500) && (value <= 10000)) {
            rx_config.minsync = value;
            valid_input = 1;
          }
        case 27:
          if (value == 0) {
            rx_config.RSSIpwm = 255;
            valid_input = 1;
          } else if (value <= 64) {
            rx_config.RSSIpwm = value - 1;
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
        RX_menu_headers();
      }
    }
  }
}

void CLI_RX_config()
{
  Serial.println(F("Connecting to RX, power up the RX (with bind plug if not using always bind)"));
  Serial.println(F("Press any key to cancel"));
  if (Serial.available()) {
    handleRXmenu(Serial.read());
  }

  switch (rxcConnect()) {
  case 2:
    Serial.println(F("Timeout when connecting to RX"));
    return;
  case 3:
    Serial.println(F("Protocol error with RX"));
    return;
  }

  CLI_menu = -1;
  CLI_magic_set = 0;
  RX_menu_headers();
  while (CLI_menu != -2) { // LOCK user here until settings are saved or abandonded
    if (Serial.available()) {
      handleRXmenu(Serial.read());
    }
  }
}

void handleCLImenu(char c)
{
  if (CLI_menu == -1) {
    switch (c) {
    case '!':
    case '@':
      Serial.println("NOT ENABLED");
      break;
    case '\n':
    case '\r':
      break;
    case 's':
    case 'S':
      // save settings to EEPROM
      txWriteEeprom();
      Serial.println("Settings saved to EEPROM\n");
      // leave CLI
      CLI_menu = -2;
      break;
    case 'x':
    case 'X':
    case 0x1b: //ESC
      // restore settings from EEPROM
      txReadEeprom();
      Serial.println("Reverted settings from EEPROM\n");
      // leave CLI
      CLI_menu = -2;
      break;
    case 'i':
    case 'I':
      // restore factory settings
      bindInitDefaults();
      txInitDefaults();
      Serial.println("Loaded factory defaults\n");
      break;
    case 'r':
    case 'R':
      // randomize channels and key
      bindRandomize(true);
      Serial.println("Key and channels randomized\n");
      break;
    case 'f':
    case 'F':
      showFrequencies();
      break;
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
      CLI_menu = c - '0';
      break;
    case '8':
      Serial.println(F("Toggled telemetry!"));
      {
        uint8_t newf = (bind_data.flags + TELEMETRY_PASSTHRU) & TELEMETRY_MASK;
        bind_data.flags &= ~TELEMETRY_MASK;
        bind_data.flags |= newf;
      }
      CLI_menu = -1;
      break;
    case '9':
      CLI_menu = 9;
      break;
    case '0':
      Serial.println(F("Toggled TX muting!"));
      tx_config.flags ^= MUTE_TX;
      CLI_menu = -1;
      break;
    case 'a':
    case 'A':
      Serial.println(F("Toggled inverted PPM!"));
      tx_config.flags ^= INVERTED_PPMIN;
      CLI_menu = -1;
      break;
    case 'b':
    case 'B':
      Serial.println(F("Toggled microPPM"));
      tx_config.flags ^= MICROPPM;
      CLI_menu = -1;
      break;
    case 'z':
    case 'Z':
      CLI_RX_config();
      CLI_menu = -1;
      break;
    }
    if (CLI_menu != -2) {
      CLI_menu_headers();
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
          if ((value > 0) && (value < 11)) {
            bind_data.rf_channel_spacing = value;
            valid_input = 1;
          }
          break;
        case 5: {
          char* slice = strtok(CLI_buffer, ",");
          uint8_t channel = 0;
          while ((slice != NULL) && (atoi(slice) != 0)) {
            if (channel < MAXHOPS) {
              bind_data.hopchannel[channel++] = atoi(slice);
            }
            slice = strtok(NULL, ",");
          }
          valid_input = 1;
          while (channel < MAXHOPS) {
            bind_data.hopchannel[channel++] = 0;
          }
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
        case 9:
          if ((value >0) && (value <= 115200)) {
            bind_data.serial_baudrate = value;
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

