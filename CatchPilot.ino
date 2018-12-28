/*
        CatchPilot

        Copyright (C) 2018 Martin Bettermann <catchpilot.de@gmail.com>

        https://github.com/tourdion/CatchPilot

        Version beta 0.3   Stand 28.12.2018

        catchpilot.de@gmail.com

        GNU General Public License v3.0

        This program is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
        (at your option) any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <string.h>
#include <RTClib.h>             // Batteriegepuffertes Uhrenmodul
#include <SoftwareSerial.h>     // zusätzliche serielle Schnittstelle
#include <Adafruit_SSD1306.h>   // OLED-Display
#include <avr/wdt.h>            // WatchDog Timer

// ###   I2C: SCL an A5;    SDA an A4
#define OLED_RESET 4 // not used / nicht genutzt bei diesem Display
Adafruit_SSD1306 display(OLED_RESET);

RTC_DS1307 RTC;  // Adress of ClockModule is 0x68

//Create software serial object to communicate with GSM-Module
#define GSM_TX_PIN 8
#define GSM_RX_PIN 7
SoftwareSerial serialGSM(GSM_TX_PIN, GSM_RX_PIN);

// SMS an diese Nummer
char Telefonnummer[25] = "AT+CMGS=\"017xxxxxxxx\"\r";

/*  Fangkontakt an D5 und VCC 5V
    1kOhm zwischen GND und D5
*/
byte ReedKontakt = 5;
bool Fang = false;

/*  SMS ja oder nein
    Schiebeschalter an D2 und VCC 5V
    1kOhm zwischen GND und D2
*/
byte SchalterSMS = 2;
bool StatusSMS = false;

/*  Batteriespannung messen an Pin A0 über Spannungsteiler
     100K Poti zwischen 5V und Gnd, Mitte an A0
*/
char Akku[3];

byte x;
byte i;
int FehlerCheck;
int BAUDRATE = 19200;

char Nachricht[30];
char Uhrzeit[7];
char Fangzeit[7];
char y[4];


// #### Subroutine um serialGSM-Abfragen in String umzuwandeln
#define ZEILENTRENNZEICHEN 13   // 13 ist Steuerzeichen CR (Carriage Return)
char* receiveBuffer() {
  static char lineBuffer[40];
  static byte ErrorCounter = 0;
  char c;
  if (serialGSM.available() == 0) return NULL;
  if (ErrorCounter == 0) memset(lineBuffer, 0, sizeof(lineBuffer));
  c = serialGSM.read();
  if (c == ZEILENTRENNZEICHEN)
  {
    ErrorCounter = 0;
    return lineBuffer;
  }
  else if (c >= 32) // kein Steuerzeichen?
  {
    lineBuffer[ErrorCounter] = c; // Zeichen im Zeilenpuffer einfügen
    if (ErrorCounter < sizeof(lineBuffer) - 2) ErrorCounter++;
  }
  return NULL;
}     //  Ende  receiveBuffer()

/* Subroutine um Statusmeldungen zur GSM-Verbindung abzurufen
   Die Werte für die String-Bearbeitung funktionieren mit dem ALDITalk (E-Plus)-Netz
   und müssen für andere Netze ggf. angepasst werden.
*/
void getStatusGSM() {
  while (!serialGSM);
  serialGSM.println("at+csq");                   // Empfangsstärke
  serialGSM.println("at+cusd=1,\"*100#\",15");   // Guthaben
  serialGSM.println("at+cops?");                 // Netz
  i = 1;
  FehlerCheck = 1;
  char Store[10];
  do
  {
    char* text = receiveBuffer();
    if (text != NULL) {
      if ( !strncmp (text, "+CSQ", 4 )) {
        strcpy(Store, text + 6);
        strncpy(Nachricht, Store, 2);
        strcat(Nachricht, " ");
        i++;
      }
      if ( !strncmp (text, "+COPS", 5 )) {
        strcpy(Store, text + 12);
        strncat(Nachricht, Store, 6);
        strcat(Nachricht, " ");
        i++;
      }
      if ( !strncmp (text, "+CUSD", 5 )) {
        strcpy(Store, text + 32);
        strncat(Nachricht, Store, 5);
        strcat(Nachricht, "Euro ");
        i++;
      }
    }
    FehlerCheck++;
  } while (i < 4 || FehlerCheck > 2000 );  //Abbruch wenn zu lange Wartezeit
  return;
}   // Ende getStatusGSM()


void setup() {

  wdt_enable(WDTO_8S); // Start des AVR-Watchdog

  RTC.begin();
  RTC.adjust(DateTime(__DATE__, __TIME__));    // Uhr stellen

  // OLED Display mit I2C-Adresse 0x3c initialisieren
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();

  pinMode(ReedKontakt, INPUT);
  pinMode(SchalterSMS, INPUT);
  pinMode(A0, INPUT);

  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("CatchPilot");
  display.setTextSize(1);

  serialGSM.begin(BAUDRATE);
  while (!serialGSM);
  
  memset(Nachricht, 0, sizeof(Nachricht));
  getStatusGSM();
  display.setCursor(0, 21);
  display.print(Nachricht);
  Serial.println(Nachricht);
  Serial.println();

  display.display();
  delay(3500);

}

void loop() {

  display.clearDisplay();

  // ####  Tägliche Statusmeldungen

  if ( !strncmp ( Uhrzeit, "09:00:00", 8 ) || !strncmp ( Uhrzeit, "21:00:00", 8 )) {
    memset(Nachricht, 0, sizeof(Nachricht));
    getStatusGSM();
    strncat(Nachricht, Uhrzeit, 5);
    strcat(Nachricht, " Bat");
    strncat(Nachricht, Akku, 4);
    strcat(Nachricht, "V");
    Serial.println(Nachricht);
    if ( StatusSMS == true ) {    // ###### Senden der SMS
      serialGSM.println("AT+CMGF=1");
      delay(1000);
      serialGSM.print(Telefonnummer);
      delay(1000);
      serialGSM.println(Nachricht);
      delay(100);
      serialGSM.println((char)26);
      delay(1000);
    }
  }

  // #### SMS an oder aus

  if ( digitalRead(SchalterSMS) == 1) {
    StatusSMS = true;
    display.setCursor(80, 0);
    display.print("SMS on ");
  }
  else {
    StatusSMS = false;
    display.setCursor(80, 0);
    display.print("SMS off");
  }

  // #### Akku-Spannung abrufen (an Pin A0)
  dtostrf((analogRead(A0) / 810.0 * 12.38), 4, 1, Akku);
  display.setCursor(0, 12);
  display.print("Akku ");
  display.print(Akku);
  display.print("V");
  delay(20);

  // ####  Uhrzeit
  DateTime now = RTC.now();
  memset(Uhrzeit, 0, sizeof(Uhrzeit));
  x = now.hour();
  itoa(x, y, 10);
  if (x < 10)  {
    strcat (Uhrzeit, "0");
  }
  strcat (Uhrzeit, y);
  strcat (Uhrzeit, ":");
  x = now.minute();
  itoa(x, y, 10);
  if (x < 10)  {
    strcat (Uhrzeit, "0");
  }
  strcat (Uhrzeit, y);
  strcat (Uhrzeit, ":");
  x = now.second();
  itoa(x, y, 10);
  if (x < 10)  {
    strcat (Uhrzeit, "0");
  }
  strcat (Uhrzeit, y);

  display.setCursor(0, 0);
  display.print(Uhrzeit);

  // ####   Abfrage Reed-Kontakt
  if (digitalRead(ReedKontakt) == 1)  {
    Fang = false;
    display.setCursor(0, 24);
    display.print("Falle bereit");
  }
  else  {
    display.setCursor(0, 24);
    display.print("Fang um ");
    display.print(Fangzeit);
    if ( Fang == false ) {
      strncpy(Fangzeit, Uhrzeit, 5);
      strcpy(Nachricht, "Fang um ");
      strncat(Nachricht, Fangzeit, 5);
      Fang = true;  // Verriegelung bis Falle wieder bereit
      if ( StatusSMS == true ) {    // ###### Senden der SMS
        serialGSM.println("AT+CMGF=1");
        delay(1000);
        serialGSM.print(Telefonnummer);
        delay(1000);
        serialGSM.println(Nachricht);
        delay(100);
        serialGSM.println((char)26);
        delay(1000);
      }
    }
  }

  display.display();

  wdt_reset(); // WatchDog Timer Reset

}
