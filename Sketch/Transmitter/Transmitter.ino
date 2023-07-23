/**
  * Project Name: Detachable Arduino Based Safety Helmet for Motorcycle Riders
  * Description: This project aims to create a device that can ensure safety for the motorcycle riders
  * Main Author: Covey Jorjet De Luna
  * Group Mates: Frances Louela Joyce Reyes
  : Shela Marie Tojot
  : Junel Cabrera

  * Created: 01-15-2023
  * Last Modified: ON-GOING PROJECT
  * Email: cjdeluna1423@gmail.com
  * Telegram: https://t.me/Ambabo

  * Additional comments:
  * I do not own any of the included Libraries. All of them are open source, under the GNU License

  * BUY ME A COFFEE
  * LINK: https://buymeacoffee.com/coveyjorjet
**/

//Necessary Libraries
#include <Wire.h>
#include <SPI.h>
#include <printf.h>
#include <nRF24L01.h>  //NRF24L01 Library
#include <RF24.h>
#include <RF24_config.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>

//Defining Pin of the Variable
const uint8_t RF24CE_PIN = 9;
const uint8_t RF24CSN_PIN = 8;
const uint8_t IR_PIN = 5;
const uint8_t SHOCK_PIN = A1;
const uint8_t GPSTX_PIN = 3;
const uint8_t GPSRX_PIN = 2;

TinyGPSPlus gps;                                 // Creating constructor for the gps, to be passed sa library
SoftwareSerial gpsSerial(GPSTX_PIN, GPSRX_PIN);  // Creating software Serial Port called gpsSerial

RF24 wirelessCom(RF24CE_PIN, RF24CSN_PIN);  // CNS at CNE pinout ng Tranceiver (NRF24L01)
// const byte wirelessComAddress[6] = "0xC3B4B5E6C1LL"; //communication pipe address ng dalawang tranceiver, Unique sa bawat device
const byte wirelessComAddress[6] = { 0x73, 0x5a, 0xa0, 0xbe, 0x58, 0x4c };  //communication pipe address ng dalawang tranceiver, Unique sa bawat device

bool isDebug = true;  // turn the flag "true" if want mo na mag display ng Information sa Serial Monitor

//RUN ONCE
void setup() {
  //Begin Serial Communication with 115200 Baudrate
  Serial.begin(115200);
  //Begub Software Serial Communication to the Gps With 9600 Baudrate
  gpsSerial.begin(9600);
  //Begin Wireless Communication
  wirelessCom.begin();
  // Para to sa isDebugging Purpose if hindi nag start ng maayos ung transmitter natin
  if (!wirelessCom.begin()) {
    while (true) {
      Serial.println("Wireless Communication between two device is not responding!!");
      delay(500);
    }  // hold in infinite loop, hindi mag iinitialize ung device pag hindi naka attach ng maayos ung nrf24l01
  }
  wirelessCom.setPALevel(RF24_PA_MIN);  // Low Lang natin ung Pa Level kasi malapit lang din naman ung pag tatransmitan.
  // wirelessCom.setPayloadSize(sizeof(data)); // usefull to if want to limit the payload size na itatransmit niya, 2x int datatype occupy 8 bytes
  wirelessCom.stopListening();                      // put wirelessCom in TX or Sender mode
  wirelessCom.openWritingPipe(wirelessComAddress);  // Open natin ung pipe address na pag sesendan niya ng data
  // wirelessCom.setRetries(((2 * 3) % 12) + 3, 15); // Set natin dito ung timeout, maximum value is 15 for both args

  //Setting pins that is used as INPUT
  pinMode(SHOCK_PIN, INPUT);
  pinMode(IR_PIN, INPUT);

  if (isDebug) {
    printf_begin();
    wirelessCom.printDetails();  //ipiprint sa Serial Monitor ung details ng nrf24 module.
  }
}

//INFINITE LOOP
void loop() {
  const byte SHOCK_THRESHOLD = 3;             // Minimum number of consecutive readings of 1 to trigger a shock event
  const unsigned long SHOCK_DURATION = 3000;  // Minimum duration of shock event in milliseconds
  float irValue{};                            //creating variable forvceValue para ma access globally
  float shockValue{};                         //creating variable shockValue para ma access globally
  double gpsLat{}, gpsLon{}, gpsAlt{};        //creating variable using double para sa precise location
  int gpsMonth{}, gpsDay{}, gpsYear{}, gpsHour{}, gpsMinute{}, gpsSecond{};

  readGpsData();  //read and store natin ung gps data sa kanya-kanyang variable
  if (isGpsValid()) {
    gpsLat = gps.location.lat();
    gpsLon = gps.location.lng();
    gpsAlt = gps.altitude.meters();
    gpsMonth = gps.date.month();
    gpsDay = gps.date.day();
    gpsYear = gps.date.year();
    gpsHour = gps.time.hour();
    gpsMinute = gps.time.minute();
    gpsSecond = gps.time.second();
  }
  irValue = digitalRead(IR_PIN);                                                                                            //read and store natin ung Proximity Value sa variable na "float irValue;"
  shockValue = analogRead(SHOCK_PIN);                                                                                       //read and store natin ung Shock Value sa variable na "float shockValue;"
  startSendingData(irValue, shockValue, gpsLat, gpsLon, gpsAlt, gpsMonth, gpsDay, gpsYear, gpsHour, gpsMinute, gpsSecond);  //after storing all the data neede, i send natin sa another device using array "float data[];"
}

/**
  *This function is used to read the gpsData,
  *mag i-infinite loop to pag walang gps data na na-read.
  *if may na read na gps data mag e-execute ung storeGpsData().
**/
void readGpsData() {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }
  // If 5000 milliseconds pass and there are no characters coming in
  // over the software serial port, show a "No GPS detected" error
  if (millis() > 5000 && gps.charsProcessed() < 10) {
    while (true) {
      //Hold in Infinite Loop
      Serial.println("No GPS detected, Ensure that GPS (RX,TX) pin is connected to the right port in the Device");
      delay(500);
    }
  }
}

/**
  *This function is used to determine if gps is susccesfully processed and check if it is
  *processed accordingly.
  *return true if gps is valid, else false
**/
bool isGpsValid() {
  return gps.location.isValid() && gps.date.isValid() && gps.time.isValid();
}

/**
  *This function is used to send recorded sensors data continously.
  *This also check if the data is sent successfully, if hindi nakasent ibig sabihin naka off ung isang device
  *Use this function with three Parameters, @float status, @float _irValue, @shockValue
**/
void startSendingData(float _irValue, float _shockValue, double _gpsLat, double _gpsLon, double _gpsAlt, int _gpsMonth, int _gpsDay, int _gpsYear, int _gpsHour, int _gpsMinute, int _gpsSecond) {
  //Defining Array Place of each of the Variable
  const uint8_t IR_INDEX = 0;
  const uint8_t SHOCK_INDEX = 1;
  const uint8_t GPSLAT_INDEX = 2;
  const uint8_t GPSLON_INDEX = 3;
  const uint8_t GPSDAY_INDEX = 4;
  const uint8_t GPSYEAR_INDEX = 5;
  const uint8_t GPSHOUR_INDEX = 6;
  const uint8_t GPSMINUTE_INDEX = 7;
  const uint8_t GPSSECOND_INDEX = 8;

  float data[] = { _irValue, _shockValue, _gpsLat, _gpsLon, _gpsMonth, _gpsDay, _gpsYear, _gpsHour, _gpsMinute, _gpsSecond };
  unsigned long start_timer = micros();                      // start natin timer
  bool isDataSent = wirelessCom.write(&data, sizeof(data));  // transmit natin ung data at isave ung flag na isDataSent
  unsigned long end_timer = micros();                        // end natin timer

  if (isDebug) {
    if (isDataSent) {
      Serial.print(F("Transmission of Arrays :"));
      Serial.print(F(" Proximity="));
      Serial.print(data[IR_INDEX]);  // print natin kung ano ba ung sinend natin na Shock Value
      Serial.print(F(" SHOCK="));
      Serial.print(data[SHOCK_INDEX]);  // print natin kung ano ba ung sinend natin na FSR Vaue
      Serial.print(F(" Latitude="));
      Serial.print(_gpsLat, 6);  // print natin kung ano ba ung sinend natin na Lattitude Value
      Serial.print(F(" Longitude="));
      Serial.print(_gpsLon, 6);  // print natin kung ano ba ung sinend natin na Longitude Vaue
      Serial.print(F(" Date="));
      Serial.print(_gpsMonth);  // print natin kung ano ba ung sinend natin na Month Vaue
      Serial.print(F("/"));
      Serial.print(_gpsDay);  // print natin kung ano ba ung sinend natin na Day Vaue
      Serial.print("/");
      Serial.println(_gpsYear);  // print natin kung ano ba ung sinend natin na Year Vaue
      Serial.print(F(" Time="));
      Serial.print(_gpsHour);  // print natin kung ano ba ung sinend natin na Hour Vaue
      Serial.print(F("/"));
      Serial.print(_gpsMinute);  // print natin kung ano ba ung sinend natin na Minute Vaue
      Serial.print(F("/"));
      Serial.println(_gpsSecond);  // print natin kung ano ba ung sinend natin na Second Vaue
      Serial.print(F(" as node "));
      Serial.print(sizeof(data));  // print natin kung ilan ba ung size ng array na nasent natin
      Serial.print(F(" successful!"));
      Serial.print(F(" Time to transmit: "));
      Serial.print(end_timer - start_timer);  // print natin kung ilan ung seconds na natake niya sa pagsent ng data
      Serial.println(F(" us"));
      Serial.println();
      delay(500);  // slow transmissions down by 500 milliseconds para ma read mo ung serial monitor ng maayos
    } else {
      Serial.println("Send Failed");
      Serial.println("Check if the Receiver is Turned On");
    }
  }
}
