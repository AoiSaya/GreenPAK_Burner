#include <M5StickC.h>
#include <stdlib.h>
#include <string.h>

#define NVM_CONFIG 0x02
#define EEPROM_CONFIG 0x03

int count = 0;
uint8_t slave_address = 0x00;
bool device_present[16] = {false};

uint8_t data_array[16][16] = {};
 
// Store nvmData in PROGMEM to save on RAM
const char nvmData[] PROGMEM ="\
";
// Store eepromData in PROGMEM to save on RAM
const char eepromData[] PROGMEM ="\
";

////////////////////////////////////////////////////////////////////////////////
// setup 
////////////////////////////////////////////////////////////////////////////////
void setup() {
  M5.begin();
  Wire.begin(32, 33);  
  Wire.setClock(100000);
  Serial.begin(115200);
  
  M5.Lcd.setRotation(3);
  M5.Lcd.setCursor(0, 0, 4);
  M5.Lcd.print("GreenPAK\n");
  M5.Lcd.print("Burner\n");
  M5.Lcd.print("ver.0.1");
  delay(100);
}

////////////////////////////////////////////////////////////////////////////////
// loop 
////////////////////////////////////////////////////////////////////////////////
void loop() {
  String myString = "";
  int incomingByte = 0;
  slave_address = 0x00;
  String NVMorEEPROM = "";

  char selection = query("\nMENU: r = read, e = erase, w = write, p = ping\n");

  switch (selection)
  {
    case 'r': 
        Serial.println(F("Reading chip!"));
        requestSlaveAddress();
        NVMorEEPROM = requestNVMorEeprom();
        readChip(NVMorEEPROM);
        // Serial.println(F("Done Reading!"));
        break;
    case 'e': 
        Serial.println(F("Erasing Chip!"));
        requestSlaveAddress();
        NVMorEEPROM = requestNVMorEeprom();
        
        if (eraseChip(NVMorEEPROM) == 0) {
          // Serial.println(F("Done erasing!"));
        } else {
          Serial.println(F("Erasing did not complete correctly!"));
        }
        delay(100);
        ping();
        break;
    case 'w': 
        Serial.println(F("Writing Chip!"));
        requestSlaveAddress();
        NVMorEEPROM = requestNVMorEeprom();
        
        if (eraseChip(NVMorEEPROM) == 0) {
          // Serial.println(F("Done erasing!"));
        } else {
          Serial.println(F("Erasing did not complete correctly!"));
        }

        ping();
        
        if (writeChip(NVMorEEPROM) == 0) {
          // Serial.println(F("Done writing!"));
        } else {
          Serial.println(F("Writing did not complete correctly!"));
        }

        ping();

        readChip(NVMorEEPROM);
        // Serial.println(F("Done Reading!"));
        break;
    case 'p': 
        Serial.println(F("Pinging!"));
        ping();
        // Serial.println(F("Done Pinging!"));
        break;
    default:
        break;
  }
}

////////////////////////////////////////////////////////////////////////////////
// request slave address 
////////////////////////////////////////////////////////////////////////////////
void requestSlaveAddress() {
  ping();

  while(1) {
    char mySlaveAddress = query("Submit slave address, 0-F: ");
    String myString = (String)mySlaveAddress;
    slave_address = strtol(&myString[0], NULL, 16);

    //Check for a valid slave address
    if (device_present[slave_address] == false)
    {
      Serial.println(F("You entered an incorrect slave address. Submit slave address, 0-F: "));
      continue;
    }
    else {
      PrintHex8(slave_address);
      Serial.println();
      return;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// request NVM or EEPROM 
////////////////////////////////////////////////////////////////////////////////
String requestNVMorEeprom() {
  while (1)
  {    
    char selection = query("MENU: n = NVM, e = EEPROM: ");

    switch (selection)
    {
      case 'n':
          Serial.print(F("NVM"));
          Serial.println();
          return "NVM";
      case 'e':
          Serial.print(F("EEPROM"));
          Serial.println();
          return "EEPROM";
      default:
          Serial.println(F("Invalid selection. You did not enter \"n\" or \"e\"."));
          continue;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// query 
////////////////////////////////////////////////////////////////////////////////
char query(String queryString) {
  Serial.println();

  Serial.print(queryString);
  while (1) {
    if (Serial.available() > 0) {
      String myString = Serial.readString();
      return myString[0];
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// print hex 8 
////////////////////////////////////////////////////////////////////////////////
void PrintHex8(uint8_t data) {
  if (data < 0x10) {
    Serial.print("0");
  }
  Serial.print(data, HEX);
}

////////////////////////////////////////////////////////////////////////////////
// readChip 
////////////////////////////////////////////////////////////////////////////////
int readChip(String NVMorEEPROM) {
  int control_code = slave_address << 3;

  if (NVMorEEPROM == "NVM")
  {
    control_code |= NVM_CONFIG;
  }
  else if (NVMorEEPROM == "EEPROM")
  {
    control_code |= EEPROM_CONFIG;
  }

  for (int i = 0; i < 16; i++) {
    Wire.beginTransmission(control_code);
    Wire.write(i << 4);
    Wire.endTransmission(false);
    delay(10);
    Wire.requestFrom(control_code, 16);

    while (Wire.available()) {
      PrintHex8(Wire.read());
    }
    Serial.println();
  }
}

////////////////////////////////////////////////////////////////////////////////
// eraseChip 
////////////////////////////////////////////////////////////////////////////////
int eraseChip(String NVMorEEPROM) {
  int control_code = slave_address << 3;
  int addressForAckPolling = control_code;

  for (uint8_t i = 0; i < 16; i++) {
    Serial.print(F("Erasing page: 0x"));
    PrintHex8(i);
    Serial.print(F(" "));

    Wire.beginTransmission(control_code);
    Wire.write(0xE3);

    if (NVMorEEPROM == "NVM")
    {
      Serial.print(F("NVM "));
      Wire.write(0x80 | i);
    }
    else if (NVMorEEPROM == "EEPROM")
    {
      Serial.print(F("EEPROM "));
      Wire.write(0x90 | i);
    }
    
/* To accommodate for the non-I2C compliant ACK behavior of the Page Erase Byte, we've removed the software check for an I2C ACK
 *  and added the "Wire.endTransmission();" line to generate a stop condition.
 *  - Please reference "Issue 2: Non-I2C Compliant ACK Behavior for the NVM and EEPROM Page Erase Byte"
 *    in the SLG46824/6 (XC revision) errata document for more information. */

//    if (Wire.endTransmission() == 0) {
//      Serial.print(F("ack "));
//    } 
//    else { 
//      Serial.print(F("nack "));  
//      return -1;
//    }

    Wire.endTransmission();

    if (ackPolling(addressForAckPolling) == -1)
    {
      return -1;
    } else {
      Serial.print(F("ready "));
      delay(100);
    }
    Serial.println();
  }

  powercycle(control_code);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// writeChip 
////////////////////////////////////////////////////////////////////////////////
int writeChip(String NVMorEEPROM) {
  int control_code = 0x00;
  int addressForAckPolling = 0x00;
  bool NVM_selected = false;
  bool EEPROM_selected = false;

  if (NVMorEEPROM == "NVM")
  {
    // Serial.println(F("Writing NVM"));
    // Set the slave address to 0x00 since the chip has just been erased
    slave_address = 0x00;
    // Set the control code to 0x00 since the chip has just been erased
    control_code = 0x00;
    control_code |= NVM_CONFIG;
    NVM_selected = true;
    addressForAckPolling = 0x00;
  }
  else if (NVMorEEPROM == "EEPROM")
  {
    // Serial.println(F("Writing EEPROM"));
    control_code = slave_address << 3;
    control_code |= EEPROM_CONFIG;
    EEPROM_selected = true;
    addressForAckPolling = slave_address << 3;
  }

  Serial.print(F("Control Code: 0x"));
  PrintHex8(control_code);
  Serial.println();
  
  // Assign each byte to data_array[][] array;
  // http://www.gammon.com.au/progmem

  // Serial.println(F("New NVM data:"));
  for (size_t i = 0; i < 16; i++)
  {
    // Pull current page NVM from PROGMEM and place into buffer
    char buffer [64];
    if (NVM_selected)
    {
      memcpy(buffer,&nvmData[i*43+9],32);
    }
    else if (EEPROM_selected)
    {
      memcpy(buffer,&eepromData[i*43+9],32);
    }

    for (size_t j = 0; j < 16; j++)
    {
      String temp = (String)buffer[2 * j] + (String)buffer[(2 * j) + 1];
      long myNum = strtol(&temp[0], NULL, 16);
      data_array[i][j] = (uint8_t) myNum;
    }
  }
  Serial.println();

  if (NVM_selected)
  {
    while(1) {
      char newSA = query("Enter new slave address 0-F: ");
      String newSAstring = (String)newSA;
      int temp = strtol(&newSAstring[0], NULL, 16);
      
      if (temp < 0 || temp > 15)
      {
        Serial.println(temp);
        Serial.println(F(" is not a valid slave address."));
        continue;
      }
      else 
      {
        slave_address = temp;
        Serial.print(F("0x"));
        PrintHex8(slave_address);
        Serial.println();
        break;
      }
    }

    data_array[0xC][0xA] = (data_array[0xC][0xA] & 0xF0) | slave_address;
  }

  // Write each byte of data_array[][] array to the chip
  for (int i = 0; i < 16; i++) {
    Wire.beginTransmission(control_code);
    Wire.write(i << 4);

    PrintHex8(i);
    Serial.print(F(" "));

    for (int j = 0; j < 16; j++) {
      Wire.write(data_array[i][j]);
      PrintHex8(data_array[i][j]);
    }
    
    if (Wire.endTransmission() == 0) {
      Serial.print(F(" ack "));
    } else {
      Serial.print(F(" nack\n"));
      Serial.println(F("Oh No! Something went wrong while programming!"));
      return -1;
    }

    if (ackPolling(addressForAckPolling) == -1)
    {
      return -1;
    } else {
      Serial.println(F("ready"));
      delay(100);
    }
  }

  powercycle(control_code);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// ping 
////////////////////////////////////////////////////////////////////////////////
void ping() {
  delay(100);
  for (int i = 0; i < 16; i++) {
    Wire.beginTransmission(i << 3);

    if (Wire.endTransmission() == 0) {
      Serial.print(F("device 0x"));
      PrintHex8(i);
      Serial.println(F(" is present"));
      device_present[i] = true;
    } else {
      device_present[i] = false; 
    }
  }
  delay(100);
}

////////////////////////////////////////////////////////////////////////////////
// ack polling 
////////////////////////////////////////////////////////////////////////////////
int ackPolling(int addressForAckPolling) {
    int nack_count = 0;
    while (1) {
      Wire.beginTransmission(addressForAckPolling);
      if (Wire.endTransmission() == 0) {
        return 0;
      }
      if (nack_count >= 1000)
      {
        Serial.println(F("Geez! Something went wrong while programming!"));
        return -1;
      }
      nack_count++;
      delay(1);
    }
}

////////////////////////////////////////////////////////////////////////////////
// power cycle 
////////////////////////////////////////////////////////////////////////////////
void powercycle(int code) {
  Serial.println(F("Power Cycling!"));
// Software reset
  Wire.beginTransmission(code & 0x78);
  Wire.write(0xC8);
  Wire.write(0x02);
  Wire.endTransmission();
  // Serial.println(F("Done Power Cycling!"));
}
