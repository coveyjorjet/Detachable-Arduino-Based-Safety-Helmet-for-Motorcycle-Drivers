/**
  * Project Name: DETACHABLE MICROCONTROLLER-BASED MOTORCYCLE HELMET DEVICE AND SAFETY SYSTEM
  * Description: This project aims to create a device that can ensure safety for the motorcycle riders
  * Main Author: Covey Jorjet De Luna
  * Group Mates: Frances Louela Joyce Reyes
               : Shela Marie Tojot
               : Junel Cabrera

  * Created: 01-15-2023
  * Last Modified: 05-30-2023
  * Email: cjdeluna1423@gmail.com
  * Telegram: https://t.me/Ambabo
  * Facebook: https://www.facebook.com/coveyjorjet

  * Additional comments:
  * I do not own any of the included Libraries. All of them are open source, under the GNU License
  * Please Fork this project on my github repositories and improve a block of code that you think is wrong.

  * BUY ME A COFFEE
  * LINK: https://buymeacoffee.com/coveyjorjet
**/

//Necessary Libraries
#include <SPI.h>     //Used for Wireless Communication
#include <printf.h>  //Library that must be included to print the details of current NRF24L01 Device attached
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>        //Tranceiver (NRF24L01) Library, gamit sa pag implement ng tranceiver
#include <EEPROM.h>             //EEPROM Library, gamit sa pag store ng password na sinave ni User
#include <Wire.h>               //This library allows us to communicate sa I2C device natin which is the LCD I2C 16x2
#include <LiquidCrystal_I2C.h>  //LCD_I2C Library
#include <Keypad.h>             //4x4 Keypad Library
#include <Sim800L.h>            //Sim800L V2 Library
#include <SoftwareSerial.h>     //Library to blend with The Sim800l Library Serial Communication
#include <LowPower.h>

//Allowed Password length + 1 for Null Terminator!
#define PASSWORD_LENGTH 5

// EEPROM addresses
#define PASS_ADDRESS 0   //Address ng Password sa EEPROM Basically 0-3 address ay olupado ni password kasi 4 ung length ng password
#define EMNUM_ADDRESS 5  //Emergency Number address sa EEPROM, from 5-15 address ay ukopado naman ng emergency emergencyNumberFormat, without the countrycode "+63"

//Constants Index of an array
const uint8_t IR_INDEX = 0;
const uint8_t SHOCK_INDEX = 1;
const uint8_t GPSLAT_INDEX = 2;
const uint8_t GPSLON_INDEX = 3;
const uint8_t GPSDAY_INDEX = 4;
const uint8_t GPSYEAR_INDEX = 5;
const uint8_t GPSHOUR_INDEX = 6;
const uint8_t GPSMINUTE_INDEX = 7;
const uint8_t GPSSECOND_INDEX = 8;

//Constants Sensor Pins
const uint8_t FACTORY_RESET_PIN = 4;
const uint8_t BLUE_LED_PIN = 5;
const uint8_t GREEN_LED_PIN = 6;
const uint8_t RED_LED_PIN = 7;
const uint8_t SHOCK_SENSOR_PIN = 9;
const uint8_t IR_PIN = 8;
const uint8_t VOLTAGE_METER_PIN = A6;
const uint8_t WAKEUP_PIN = A7;

//Relay Pins
const uint8_t ENGINE_RELAY_PIN = 10;
const uint8_t STARTER_RELAY_PIN = 11;
const uint8_t IGNITION_RELAY_PIN = 12;
const uint8_t ALARM_RELAY_PIN = 13;

//Sim 800L Module TX and RX pin
const uint8_t SIMTX_PIN = 30;
const uint8_t SIMRX_PIN = 28;

//NRF24L01 Module CE and CSN pin
const uint8_t RF24CE_PIN = 48;
const uint8_t RF24CSN_PIN = 49;

Sim800L sim(SIMRX_PIN, SIMTX_PIN);          // TX And RX pin ng GSM Module
LiquidCrystal_I2C lcd(0x27, 16, 2);         // LCD I2C dimension declaration
RF24 wirelessCom(RF24CE_PIN, RF24CSN_PIN);  // CE And CSN pin ng Tranceiver(NRF24L01)
// const byte wirelessComAddress[6] = "0xC3B4B5E6C1LL";  //communication pipe address ng dalawang tranceiver, Unique sa bawat device
const byte wirelessComAddress[6] = { 0x73, 0x5a, 0xa0, 0xbe, 0x58, 0x4c };  //communication pipe address ng dalawang tranceiver, Unique sa bawat device

//Variables for 4x4 Matrix Keypad
const byte ROWS = 4;
const byte COLS = 4;
char hexaKeys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
const byte rowPins[ROWS] = { 32, 34, 36, 38 };                               //Pins ng Rows
const byte colPins[COLS] = { 39, 37, 35, 33 };                               //Pins ng Columns
Keypad keypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);  //I-map natin ung 4x4 keypad

//Variables for
char inputKeyData[PASSWORD_LENGTH];
char tempInputKeyData[PASSWORD_LENGTH];
char password[PASSWORD_LENGTH];
byte inputDataCount{};
byte keyState{};
char key;
char lastPressKey;

double receivedData[10];
double lat{}, lon{};
char emergencyNumber[11];
String emergencyNumberFormat = "+63";  //-> emergency Contact emergencyNumberFormat, to be parsed in first start
String emergencyMessageFormat = "Shock Detected\nLatest Known Location:\n";
String welcomeMessageFormat = "GREETINGS!!!.\nThank You for Purchasing DABS, Detachable Arduino-Based Safety Helmet for Motorcycle Riders.\nCreated using Arduino Programming.\nSALAMAT\n\nThis Message is auto generated by Dabs Project.";

bool isWirelessComAvailable{};
bool isHelmetInstalled{};
bool isSmsSent{};
bool isCallSent{};
bool isEngineActive{};
bool isEmergency{};
bool isLostKey{};
bool isDebug = true;
bool showWelcomeMessage = true;
bool alarmActive = false;

enum Mode {
  MENU_MODE,
  DEFAULT_MODE,
  INITIAL_START_MODE,
  UNLOCK_MODE,
  SET_NEW_PASS_MODE,
  CONFIRM_PASS_MODE,
  DETACHED_MODE,
  INSTALLED_MODE,
  CONNECT_IGNITION_MODE,
  EMERGENCY_USE_MODE,
  ACCIDENT_HAPPENED_MODE,
  LOCKED_MODE
};
Mode currentMode = MENU_MODE;

/**
  * Forward Function Declaration
  * Here we define all the functions used in the software before the system start,
  * Although it is not mandatory in Arduino Programming, i prefer writing my code this way.
  * It is a good practice to remember this small things and can help other students like you,
  * to better understand what this lines means.
**/
void startSystem();
void lockSystem();
void collectInputKey();
void clearInputData();
void startMotorcycle();
void checkEeprom();
void resetEeprom();
void showIfDebug(String _debugMessage);
void pulseISR();
void rgbLed(uint8_t _red_state, uint8_t _blue_state, uint8_t _green_state);
void (*resetFunc)(void) = 0;  //this is a built-in arduino function to reboot the device.

/**
  *Instead of using the delay() function, 
  *it is good to use the millis function as it is not blocking any codes to be executed.
  *delay() is just use for debugging and not in real operating system.
**/
void waitTwoSeconds() {
  const int twoSeconds = 2000;
  unsigned long start_timer = millis();
  while (millis() - start_timer < twoSeconds) {
    // Wait for two seconds
  }
}

/**
  *Struct named "Show" is created for more readable System and easy implementation.
  *This is usefull to access the void functions in one struct methods.
  *Adding additional methods here, requires the system to define it above this line.
  *The methods in this structs are:
  *@showMenu, @defaultMode, @initialStartMode, @setNewPassMode @confirmPassMode, @unLockedMode 
  *@detachedMode, @installedMode, @connectIgnitionMode, @emergencyUseMode, @accidentHappenedMode, @lockedMode
**/
struct Show {
  //Used to Display Menu in the First Boot of the system
  void showMenu() {
    while (inputDataCount == 0) {  //show this block of code in lcd if the user doesn't choose what to process
      lcd.setCursor(0, 0);
      lcd.print("Start  or  Lock?");
      lcd.setCursor(0, 1);
      lcd.print(" [A]        [B] ");
      key = keypad.getKey();
      if (key) {
        Serial.println(key);
        inputDataCount++;
      }
    }
    switch (key) {
      case 'A':
        showIfDebug("Entering Initial Start Mode");
        currentMode = INITIAL_START_MODE;
        lcd.clear();
        clearInputData();
        break;
      case 'B':
        while ((analogRead(WAKEUP_PIN) * 0.019550342) >= 3) {  //Give a Prompt to a user to turn off the motorcycle before locking the device
          lcd.backlight();
          lcd.setCursor(0, 0);
          lcd.print("Turn off the key");
          lcd.setCursor(0, 1);
          lcd.print("to locked DABSH!");
        }
        lockSystem();
        break;
      default:
        break;
    }
    lcd.clear();
    clearInputData();
  }

  //Function to Show The Default Mode
  void defaultMode() {
    const uint8_t MAX_PASS_INPUT = 4;
    const uint8_t MAX_NUMBER_INPUT = 10;
    char passInput[MAX_PASS_INPUT];
    char numberInput[MAX_NUMBER_INPUT];

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Entering Default");
    lcd.setCursor(4, 1);
    lcd.print("Configuration");
    waitTwoSeconds();
    lcd.clear();

    //Prompt a user to Enter Their Emergency Mobile Number
    do {
      lcd.setCursor(0, 0);
      lcd.print("ENTER NEW PASSWORD");
      lcd.setCursor(0, 1);
      lcd.print("PASS: ");  //the next Cursor Will be at Index 6
      key = keypad.getKey();
      if (key) {
        lcd.setCursor(inputDataCount + 6, 1);  //that is why i added 6 in here
        lcd.print(key);
        passInput[inputDataCount] = key;
        inputDataCount++;
      }
    } while (inputDataCount != MAX_PASS_INPUT);
    EEPROM.put(PASS_ADDRESS, passInput);  //save the passinput to EEPROM
    EEPROM.get(PASS_ADDRESS, password);
    showIfDebug("Saved PASSWORD is" + String(passInput));

    //Prompt a user to Enter Their Emergency Mobile Number
    lcd.clear();
    clearInputData();
    do {
      lcd.setCursor(0, 0);
      lcd.print("EMERGENCY NUMBER");
      lcd.setCursor(0, 1);
      lcd.print("#:+63");  //the next Cursor Will be at Index 5
      key = keypad.getKey();
      if (key) {
        lcd.setCursor(inputDataCount + 5, 1);  //that is why i added 3 in here
        lcd.print(key);
        numberInput[inputDataCount] = key;
        inputDataCount++;
      }
    } while (inputDataCount != MAX_NUMBER_INPUT);
    EEPROM.put(EMNUM_ADDRESS, numberInput);  //save the user input to EEPROM
    EEPROM.get(EMNUM_ADDRESS, emergencyNumber);
    showIfDebug("Saved EMERGENCY MOBILE NUMBER is" + String(numberInput));
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sending Welcome");
    lcd.setCursor(0, 1);
    lcd.print("Message...");
    emergencyNumberFormat += emergencyNumber;
    char* numberChar = emergencyNumberFormat.c_str();
    sim.sendSms(numberChar, welcomeMessageFormat.c_str());
    lcd.clear();
    clearInputData();
    showIfDebug("Entering Menu Mode");
    currentMode = MENU_MODE;
  }

  //Function to Show The Initiall Start Mode
  void initialStartMode() {
    const uint8_t MAX_PASS_INPUT = 4;
    lcd.clear();
    clearInputData();
    do {
      lcd.setCursor(1, 0);
      lcd.print("Enter Password");
      key = keypad.getKey();
      if (key) {
        inputKeyData[inputDataCount] = key;
        lcd.setCursor(inputDataCount + 6, 1);
        lcd.print(key);
        inputDataCount++;
      }
    } while (inputDataCount != MAX_PASS_INPUT);
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Verifying...");
    waitTwoSeconds();
    lcd.clear();
    if (strcmp(inputKeyData, password) == 0) {
      rgbLed(LOW, LOW, HIGH);
      digitalWrite(ALARM_RELAY_PIN, HIGH);
      showIfDebug("Entering UnLock Mode");
      currentMode = UNLOCK_MODE;
    } else {
      lcd.setCursor(1, 0);
      lcd.print("Wrong Password");
      lcd.setCursor(3, 1);
      lcd.print("Try Again");
      waitTwoSeconds();
    }
    lcd.clear();
    clearInputData();

    //TODO : Implemtationn sa pag detachh ng device pag hindi pa naka UNLOCKED;
  }

  //Function to Show The Set New Password Mode
  void setNewPassMode() {
    while (inputDataCount != PASSWORD_LENGTH - 1) {
      lcd.setCursor(0, 0);
      lcd.print("Set New Password");
      collectInputKey();
    }
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Verifying...");
    waitTwoSeconds();
    lcd.clear();
    memcpy(tempInputKeyData, inputKeyData, PASSWORD_LENGTH);
    clearInputData();
    showIfDebug("Entering Confirm Password Mode");
    currentMode = CONFIRM_PASS_MODE;
  }

  //Function to Show The Confirm Password Mode
  void confirmPassMode() {
    while (inputDataCount != PASSWORD_LENGTH - 1) {
      lcd.setCursor(0, 0);
      lcd.print("Password Again");
      collectInputKey();
    }
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Verifying...");
    waitTwoSeconds();
    lcd.clear();
    if (strcmp(inputKeyData, tempInputKeyData) == 0) {
      lcd.setCursor(4, 0);
      lcd.print("Saving...");
      for (int i = 0; i <= 100; i += 10) {
        lcd.setCursor(4, 1);
        lcd.print(i);
        lcd.setCursor(7, 1);
        lcd.print("%");
        delay(200);
      }
      EEPROM.put(PASS_ADDRESS, inputKeyData);
      EEPROM.get(PASS_ADDRESS, password);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("New Password is ");
      lcd.setCursor(4, 1);
      lcd.print(inputKeyData);
      waitTwoSeconds();
      showIfDebug("Entering Initial Start Mode");
      currentMode = INITIAL_START_MODE;
    } else {
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("PASSWORD");
      lcd.setCursor(3, 1);
      lcd.print("NOT MATCH!");
      waitTwoSeconds();
      showIfDebug("Entering Initial Start Mode");
      currentMode = INITIAL_START_MODE;
    }
    lcd.clear();
    clearInputData();
  }

  //Function to Show The Unlocked Mode
  void unLockedMode() {
    lcd.setCursor(0, 0);
    lcd.print("Continue?");
    lcd.setCursor(0, 1);
    lcd.print("[B]: YES");
    key = keypad.getKey();
    switch (key) {
      case 'A':
        lcd.clear();
        clearInputData();
        showIfDebug("Entering Emergency Use Mode");
        isLostKey = true;
        isEmergency = true;
        currentMode = EMERGENCY_USE_MODE;
        break;
      case 'B':
        bool isDetached;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Succesfully");
        lcd.setCursor(8, 1);
        lcd.print("Unlocked");
        waitTwoSeconds();
        lcd.clear();
        do {
          isDetached = digitalRead(IR_PIN);
          lcd.setCursor(0, 0);
          lcd.print("Please Detach");
          lcd.setCursor(0, 1);
          lcd.print("The Prototype");
        } while (!isDetached);
        lcd.clear();
        showIfDebug("Entering Detached Mode");
        currentMode = DETACHED_MODE;
        break;
      case '*':
        lcd.clear();
        clearInputData();
        showIfDebug("Entering Menu Mode");
        currentMode = MENU_MODE;
        break;
      case '#':
        lcd.clear();
        clearInputData();
        showIfDebug("Entering Set New Password Mode");
        currentMode = SET_NEW_PASS_MODE;
        break;
      default:
        break;
    }
  }
  
  //Function to Show The Detached Mode
  void detachedMode() {
    if (isWirelessComAvailable) {
      showIfDebug("Entering Installed Mode");
      currentMode = INSTALLED_MODE;
    } else {
      lcd.setCursor(0, 0);
      lcd.print("WirelessCom is");
      lcd.setCursor(0, 1);
      lcd.print("Not Available");
    }
  }

  //Function to Show The Installed Mode
  void installedMode() {
    if (isHelmetInstalled) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Successfully");
      lcd.setCursor(0, 1);
      lcd.print("Installed");
      waitTwoSeconds();
      lcd.clear();
      rgbLed(LOW, HIGH, LOW);
      showIfDebug("Entering Connect Ignition Mode");
      currentMode = CONNECT_IGNITION_MODE;
    } else {
      lcd.setCursor(0, 0);
      lcd.print("Checking if Pro");
      lcd.setCursor(0, 1);
      lcd.print("perly Installed");
    }
  }

  //Function to Show The Connect Ignition Mode
  void connectIgnitionMode() {
    if (isEmergency) {
      lcd.setCursor(2, 0);
      lcd.print("Emergency Use...");
      lcd.setCursor(3, 1);
      lcd.print("Drive Safely...");
      digitalWrite(ENGINE_RELAY_PIN, HIGH);
      digitalWrite(IGNITION_RELAY_PIN, HIGH);
      while (true)
        ;  //Hold in infinite loop to start driving as emergency is happening;
        //this loop can only stop if and only if the reset button is pressed.
    }
    if (isHelmetInstalled) {
      const unsigned int SHOCK_THRESHOLD = 10;        //3 is the minimum required threshold for shock detection
      const unsigned long countdownDuration = 10000;  // Countdown duration in milliseconds
      bool isFalseAlarm{};
      digitalWrite(ENGINE_RELAY_PIN, HIGH);  //Connecting the engine Wires using relay LOW is ON, HIGH is OFF
      startMotorcycle();
      lcd.setCursor(0, 0);
      lcd.print("State: Connected");
      // Convert from 0-1023 range to 0-15V range
      // (.019550342  = 20.0 / 1023)
      float voltage_in = analogRead(VOLTAGE_METER_PIN) * 0.019550342;
      lcd.setCursor(0, 1);
      lcd.println("Voltage: " + String((float)voltage_in, 2) + 'V');
      if (receivedData[SHOCK_INDEX] >= SHOCK_THRESHOLD) {   // If a shock event is detected
        unsigned long startTime = millis();                 // Record the start time of countdown
        while (millis() - startTime < countdownDuration) {  // Countdown loop
          key = keypad.getKey();
          if (key) {
            isFalseAlarm = true;
            break;
          }
          lcd.clear();
          lcd.setCursor(2, 0);
          lcd.print("WARNING!!!");
          lcd.setCursor(14, 0);
          lcd.print(countdownDuration / 1000 - (millis() - startTime) / 1000);
          lcd.setCursor(0, 1);
          lcd.print("Shock Detected");
          delay(1000);
        }
        if (!isFalseAlarm) {
          lcd.clear();
          clearInputData();
          digitalWrite(ALARM_RELAY_PIN, LOW);  // Turn on the alarm
          currentMode = ACCIDENT_HAPPENED_MODE;
        }
      } else {
        digitalWrite(ALARM_RELAY_PIN, HIGH);  // Turn off the alarm
      }
    } else {
      const unsigned long countdownDuration = 10000;  // Countdown duration in milliseconds
      unsigned long startTime = millis();             // Record the start time of countdown
      unsigned int counter = 10;
      while (millis() - startTime < countdownDuration && receivedData[IR_INDEX] == 1) {  // Countdown loop
        if (wirelessCom.available()) { wirelessCom.read(&receivedData, sizeof(receivedData)); } //Check for incoming Data
        lcd.clear();
        lcd.setCursor(2, 0);
        lcd.print("WARNING!!!");
        lcd.setCursor(14, 0);
        lcd.print(countdownDuration / 1000 - (millis() - startTime) / 1000);
        lcd.setCursor(0, 1);
        lcd.print("Helmet Detached");
        delay(1000);
        counter--;
      }
      if (counter <= 0) {
        digitalWrite(ENGINE_RELAY_PIN, LOW);
        digitalWrite(IGNITION_RELAY_PIN, LOW);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Timeout Reached");
        waitTwoSeconds();
        lcd.clear();
        lcd.setCursor(2, 0);
        lcd.print("Please Wear");
        lcd.setCursor(0, 1);
        lcd.print("Your Helmet...");
        waitTwoSeconds();
        lcd.clear();
        showIfDebug("Entering Initial Start Mode");
        currentMode = INSTALLED_MODE;
      }
    }
  }

  //Function to Show The Emergency use Mode
  void emergencyUseMode() {
    isEmergency = true;
    lcd.setCursor(0, 0);
    lcd.print("Emergency Use");
    lcd.setCursor(0, 1);
    lcd.print("Is Turned On");
    waitTwoSeconds();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Overriding in");
    lcd.setCursor(0, 1);
    lcd.print("Progress...");
    emergencyNumberFormat = "+63";
    emergencyNumberFormat += emergencyNumber;
    char* numberChar = emergencyNumberFormat.c_str();
    sim.sendSms(numberChar, "The User overrode the system.");
    lcd.clear();
    showIfDebug("Entering Connect Ignition Mode");
    currentMode = CONNECT_IGNITION_MODE;
  }

  //Function to Show The Accident Happened Mode
  void accidentHappenedMode() {
    char* messageChar;
    char* numberChar;

    digitalWrite(ENGINE_RELAY_PIN, LOW);
    digitalWrite(ALARM_RELAY_PIN, LOW);
    lcd.clear();
    lcd.setCursor(4, 4);
    lcd.print("SHOCK");
    lcd.setCursor(3, 1);
    lcd.print("DETECTED");
    waitTwoSeconds();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sending Sms");
    lcd.setCursor(0, 1);
    lcd.print("In Progress...");
    waitTwoSeconds();
    emergencyMessageFormat += "Latitude: ";
    emergencyMessageFormat += String((double)lat, 6);
    emergencyMessageFormat += "\nLongitude: ";
    emergencyMessageFormat += String((double)lon, 6);
    emergencyMessageFormat += "\nTrack Here: http://www.google.com/maps/place/";
    emergencyMessageFormat += String((double)lat, 6);
    emergencyMessageFormat += ",";
    emergencyMessageFormat += String((double)lon, 6);
    emergencyNumberFormat += emergencyNumber;
    numberChar = emergencyNumberFormat.c_str();
    messageChar = emergencyMessageFormat.c_str();
    sim.sendSms(numberChar, messageChar);
    sim.callNumber(numberChar);
    lcd.clear();
    clearInputData();
    showIfDebug("Entering Initial Start Mode");
    currentMode = LOCKED_MODE;
  }

  //Function to Show The Locked Mode
  void lockedMode() {
    static unsigned long alarmStartTime = 0;
    const unsigned long ALARM_TIMEOUT = 180000;  // alarm timeout in milliseconds (3 minutes)

    if (digitalRead(SHOCK_SENSOR_PIN) == HIGH) {
      // turn on the alarm if it's not already active
      if (!alarmActive) {
        digitalWrite(ALARM_RELAY_PIN, LOW);  // turn on the alarm relay
        rgbLed(HIGH, LOW, LOW);
        alarmActive = true;
        alarmStartTime = millis();
        showIfDebug("Alarm Activated.");
      }
    }

    if (alarmActive && (millis() - alarmStartTime >= ALARM_TIMEOUT)) {
      // turn off the alarm if it's active and the timeout has passed
      digitalWrite(ALARM_RELAY_PIN, HIGH);  // turn off the alarm relay
      rgbLed(LOW, LOW, LOW);
      alarmStartTime = 0;
      alarmActive = false;
      showIfDebug("Alarm Deactivated after 3 minutes.");
      lcd.backlight();
      lcd.setCursor(0, 0);
      lcd.print("Alarm Deactivated");
      lcd.setCursor(0, 1);
      lcd.print("Locking Again");
      waitTwoSeconds();
      lcd.clear();
      clearInputData();
      resetFunc();
    }

    // check for incoming SMS to turn off the alarm
    uint8_t index = sim.checkForSMS();
    if (index != 0) {
      String sms = sim.readSms(1);  // read the first SMS in the inbox
      showIfDebug(sms);
      if (sms.indexOf("off") != -1) {
        digitalWrite(ALARM_RELAY_PIN, HIGH);  // turn off the alarm relay
        rgbLed(LOW, LOW, LOW);
        alarmStartTime = 0;
        alarmActive = false;
        showIfDebug("Alarm Deactivated using SMS");
        lcd.backlight();
        lcd.setCursor(0, 0);
        lcd.print("Alarm Deactivated");
        lcd.setCursor(0, 1);
        lcd.print("Locking Again");
        waitTwoSeconds();
        lcd.clear();
        clearInputData();
        resetFunc();
        sim.delAllSms();  // delete the SMS after processing
      }
    }

    key = keypad.getKey();
    if (key && key == 'D') {
      lcd.backlight();
      lcd.setCursor(1, 0);
      lcd.print("Entering Lost");
      lcd.setCursor(3, 1);
      lcd.print("Key Feature");
      waitTwoSeconds();
      const uint8_t MAX_PASS_INPUT = 4;
      lcd.clear();
      clearInputData();
      do {
        lcd.setCursor(1, 0);
        lcd.print("Enter Password");
        key = keypad.getKey();
        if (key) {
          inputKeyData[inputDataCount] = key;
          lcd.setCursor(inputDataCount + 6, 1);
          lcd.print(key);
          inputDataCount++;
        }
      } while (inputDataCount != MAX_PASS_INPUT);
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("Verifying...");
      waitTwoSeconds();
      lcd.clear();
      if (strcmp(inputKeyData, password) == 0) {
        digitalWrite(ALARM_RELAY_PIN, HIGH);
        alarmActive = false;
        lcd.setCursor(1, 0);
        lcd.print("Succesfully Launched");
        lcd.setCursor(3, 1);
        lcd.print("Lost Key Mode");
        waitTwoSeconds();
        showIfDebug("Successfully Verified...");
        isLostKey = true;
        currentMode = UNLOCK_MODE;
      } else {
        lcd.setCursor(1, 0);
        lcd.print("Wrong Password");
        lcd.setCursor(3, 1);
        lcd.print("Try Again");
        waitTwoSeconds();
      }
      lcd.clear();
      clearInputData();
    }

    // Convert from 0-1023 range to 0-20V range
    // (.019550342  = 20.0 / 1023)
    unsigned int ignitionKeyState = analogRead(WAKEUP_PIN) * 0.019550342;
    if (ignitionKeyState >= 8) {
      digitalWrite(ALARM_RELAY_PIN, HIGH);  // turn off the alarm relay
      alarmActive = false;
      alarmStartTime = 0;
      showIfDebug("System Has Started Succesfully");
      waitTwoSeconds();
      resetFunc();
    }

    //If Alarm is not Activated, go to sleep every 8 Seconds
    // if (!alarmActive && millis() - lastShockDetectedTime >= READ_SHOCK_TIMEOUT) {
    //   showIfDebug("Entering Sleep Mode!");
    //   LowPower.idle(SLEEP_8S, ADC_OFF, TIMER5_OFF, TIMER4_OFF, TIMER3_OFF,
    //                 TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART3_OFF,
    //                 USART2_OFF, USART1_OFF, USART0_OFF, TWI_OFF);
    // }
  }

  //Function to Show The Error Message if Wireless Communication is not Installed Mode
  void errorInWirelessCommunication() {
    while (inputDataCount == 0) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("NRF24L01 is not");
      lcd.setCursor(0, 1);
      lcd.print("responding !!!");
      showIfDebug("Wireless Communication hardware is not responding!!");
      waitTwoSeconds();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("[A] EMERGENCY USE");
      lcd.setCursor(0, 1);
      lcd.print("[D] RESTART!");
      waitTwoSeconds();
      collectInputKey();
    }
    switch (lastPressKey) {
      case 'A':
        lcd.clear();
        clearInputData();
        showIfDebug("Entering Initial Start Mode");
        currentMode = INITIAL_START_MODE;
        break;
      case 'D':
        lcd.noBacklight();
        lcd.clear();
        clearInputData();
        resetFunc();
        break;
      default:
        break;
    }
    lcd.clear();
    clearInputData();
  }
} show;

//Setup Function This is we're All the Fun Starts :)
void setup() {
  /**
    *Start Initializing all objects and methods that is required to start the Software
    *Serial Communication, Sim Module, LCD_I2C, and Wireless Communication (NRF24)
    *This Block of code only run once and can only be excuted every System Start
  **/
  Serial.begin(115200);
  sim.begin(115200);
  lcd.begin();
  lcd.backlight();
  printf_begin();
  wirelessCom.begin();
  if (!wirelessCom.begin()) {
    show.errorInWirelessCommunication();
  }
  wirelessCom.setPALevel(RF24_PA_MIN);                 // Low Lang natin ung Pa Level kasi malapit lang din naman ung pag tatransmitan.
  wirelessCom.setPayloadSize(sizeof(receivedData));    // usefull to if want to limit the payload size na itatransmit niya,  2x int datatype occupy 8 bytes
  wirelessCom.openReadingPipe(1, wirelessComAddress);  // Set the addresses for all pipes to TX nodes
  wirelessCom.startListening();                        // put wirelessCom in RX mode
  wirelessCom.printDetails();

  //Setting pins that is used as INPUT
  pinMode(VOLTAGE_METER_PIN, INPUT);
  pinMode(WAKEUP_PIN, INPUT);
  pinMode(IR_PIN, INPUT);
  pinMode(SHOCK_SENSOR_PIN, INPUT);
  pinMode(FACTORY_RESET_PIN, INPUT);
  //Setting pins that is used as OUTPUT
  pinMode(ENGINE_RELAY_PIN, OUTPUT);
  pinMode(STARTER_RELAY_PIN, OUTPUT);
  pinMode(ALARM_RELAY_PIN, OUTPUT);
  pinMode(IGNITION_RELAY_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);

  //On Start set the Red LED ON
  rgbLed(HIGH, LOW, LOW);

  //Setting the relay pins default to OFF
  //it is reverse in arduino, HIGH IS OFF and LOW IS ON
  //so HIGH means default it to OFF
  digitalWrite(ENGINE_RELAY_PIN, LOW);
  digitalWrite(STARTER_RELAY_PIN, HIGH);
  digitalWrite(IGNITION_RELAY_PIN, LOW);
  digitalWrite(ALARM_RELAY_PIN, HIGH);

  //Check EEPROM For Saved Passwords and Numbers
  checkEeprom();

  //Welcome Message (OPTIONAL);
  if (showWelcomeMessage) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Welcome to DABS");
    lcd.setCursor(4, 1);
    lcd.print("ENJOY!!!");
    waitTwoSeconds();
    lcd.clear();
  }
}

//Loop Function, Infinite Loop, The int main() in c++
void loop() {
  unsigned int ignitionKeyState = analogRead(WAKEUP_PIN) * 0.019550342;  // get the ignition Key State
  // Check if Motorcycle Key is on or the user lost the key or the system is in Lock Mode
  // If those condition is true, the System will SuccessFully Start
  // else the System will enter Locked Mode;
  ((ignitionKeyState >= 3 || isLostKey) ? startSystem() : lockSystem());
}

//The System Core, Starting all the process needed to execute.
void startSystem() {
  uint8_t bytes = wirelessCom.getPayloadSize();  // get the size of the payload
  // Check if Wireless Communication is Available
  isWirelessComAvailable = wirelessCom.available() ? true : false;
  if (wirelessCom.available()) {
    wirelessCom.read(&receivedData, bytes);
    //Improvised Declaration to make sure the helmet is properly worn every time Data from wireless Device is Received.
    isHelmetInstalled = (receivedData[IR_INDEX] == 0 ? true : false);
    //Making Sure that Recceived Gps is valid.
    //if lattitude and longitude is not 0.00 the very last valid gps will be stored to variable lat and lon
    if (receivedData[GPSLAT_INDEX] != 0 || receivedData[GPSLON_INDEX] != 0) {
      lat = receivedData[GPSLAT_INDEX];
      lon = receivedData[GPSLON_INDEX];
    }
  }

  //Check Which Mode is used, Show each mode if it is selected
  switch (currentMode) {
    case MENU_MODE:
      show.showMenu();
      break;
    case DEFAULT_MODE:
      show.defaultMode();
      break;
    case INITIAL_START_MODE:
      show.initialStartMode();
      break;
    case SET_NEW_PASS_MODE:
      show.setNewPassMode();
      break;
    case CONFIRM_PASS_MODE:
      show.confirmPassMode();
      break;
    case UNLOCK_MODE:
      show.unLockedMode();
      break;
    case DETACHED_MODE:
      show.detachedMode();
      break;
    case INSTALLED_MODE:
      show.installedMode();
      break;
    case CONNECT_IGNITION_MODE:
      show.connectIgnitionMode();
      break;
    case EMERGENCY_USE_MODE:
      show.emergencyUseMode();
      break;
    case ACCIDENT_HAPPENED_MODE:
      show.accidentHappenedMode();
      break;
    case LOCKED_MODE:
      show.lockedMode();
    default:
      break;
  }
}

//Etnter a Lock State
void lockSystem() {
  //Clear All inputed Data in the Keypad
  lcd.clear();
  lcd.noBacklight();  //Turn off the Backlight of Lcd to Save Power
  clearInputData();
  sim.delAllSms();  //Delete all Messages before Proceeding
  //Prepeare The Sim Module to Received Messages;
  sim.prepareForSmsReceive();
  rgbLed(LOW, LOW, LOW);  //Turn off RGB to also save power
  showIfDebug("Entering Locked Mode");
  while (true) {
    if (isLostKey) {
      break;
    }
    show.lockedMode();
  }
}

//Collecting the user input data in Keypad and displaying those in Lcd
void collectInputKey() {  //this function is used to collect keys clicked in the keypad
  key = keypad.getKey();
  if (key) {
    lastPressKey = key;
    if (key == 'C') {  //if C is clicked all the input will be cleared
      clearInputData();
      lcd.clear();
      showIfDebug("Entering initial Start Mode");
      currentMode = INITIAL_START_MODE;
    } else {
      inputKeyData[inputDataCount] = key;
      lcd.setCursor(6 + inputDataCount, 1);
      lcd.print(inputKeyData[inputDataCount]);
      showIfDebug("Clicked " + String(inputKeyData[inputDataCount]));
      inputDataCount++;
    }
  }
}

//Clearing Data or actually Clearing User input data in Keypad
void clearInputData() {
  while (inputDataCount != 0) {
    inputKeyData[inputDataCount--] = 0;
  }
  return;
}

//Starting the Motorcycle Engine and Trigger the relay for start in about 1 second
void startMotorcycle() {
  if (!isEngineActive) {
    digitalWrite(STARTER_RELAY_PIN, LOW);
    delay(1000);
    digitalWrite(STARTER_RELAY_PIN, HIGH);
    isEngineActive = true;
  }
}

//check if the EEprom has Saved Value for Password and Emergency Number
void checkEeprom() {
  EEPROM.get(PASS_ADDRESS, password);          //get the EEPROM Value stored in PASS_ADDRESS and store it in variable password
  EEPROM.get(EMNUM_ADDRESS, emergencyNumber);  //get the EEPROM Value stored in PASS_ADDRESS and store it in variable password
  if (password[0] == NULL) {
    showIfDebug("Invalid Password Found in EEPROM, Entering Default Configuration");
    showIfDebug("Entering Default Mode");
    currentMode = DEFAULT_MODE;
  } else {
    showIfDebug("Password and Emergency Mobile Number Found in EEPROM");
    showIfDebug("PASSWORD: " + String((char*)password));
    showIfDebug("EMERGENCY MOBILE NUMBER: +63" + String((char*)emergencyNumber));
    showIfDebug("Entering Menu Mode");
    currentMode = MENU_MODE;
  }
}

//re-usable Function for driving RGB Led, this can only accept HIGH or LOW State
void rgbLed(uint8_t _red_state, uint8_t _green_state, uint8_t _blue_state) {
  digitalWrite(RED_LED_PIN, _red_state);
  digitalWrite(GREEN_LED_PIN, _green_state);
  digitalWrite(BLUE_LED_PIN, _blue_state);
}

//Reset all stored Data in EEPROM for a fresh Start
void resetEeprom() {
  int length = EEPROM.length();
  for (int a = 0; a <= length; a++) {
    EEPROM.write(a, NULL);
    showIfDebug("Resetting the EEPROM address " + String((int)a) + "=0");
  }
}

//Show Every Step of the Program in Serial Monitor, Mainly use for Debugging
void showIfDebug(String _debugMessage) {
  if (isDebug) {
    Serial.println(_debugMessage);
  }
}
