/*
        CatchPilot by tourdion

        https://github.com/tourdion/CatchPilot

        Version beta 0.2   Stand 22.01.2018

        catchpilot.de@gmail.com

        GNU General Public License v3.0

*/

#include <RTClib.h>
#include <SoftwareSerial.h>;
#include <LiquidCrystal_I2C.h>

// #####   Achtung, bei SIM800L Modul: 4,2V nicht 5V, das Modul kann zerstört werden! Zehner-Diode verwenden.
// #####   Prinzipiell ist jedes GSM Modul verwendbar, da nur mit AT-Befehlen gearbeited wird
// #####   SIM_TX  an  D8
// #####   SIM_RX  an  D7

// #####    I2C: SCL an A5 (gelb);    SDA an A4  (orange)
// Set the pins on the I2C chip used for LCD connections:
//                    addr, en,rw,rs,d4,d5,d6,d7,bl,blpol
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

RTC_DS1307 RTC;  // Adress of ClockModule is 0x68

// SMS an diese Nummer
char Telefonnummer[30] = "AT+CMGS=\"017xxxxxxxxx\"\r";

char Nachricht[160];
char Datum[11];
char Uhrzeit[12];
char Fangzeit[9];

int Fang = 0;         // #####  Fangkontakt an D5 und VCC 5V
int ReedKontakt = 5;  // #####  1kOhm zwischen GND und D5

int SchalterSMS = 2;  // #####  Schiebeschalter an D2 und VCC 5V
int SMSStatus = 0;    // #####  1kOhm zwischen GND und D2

char Empfang[15];
char Guthaben[50];
char Netz[25];

byte x;
byte i;
int FehlerCheck;
char y[4];

char Akku[12];
float Volt;

//Create software serial object to communicate with SIM800
#define GSM_TX_PIN 8
#define GSM_RX_PIN 7
SoftwareSerial serialGSM(GSM_TX_PIN, GSM_RX_PIN);

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

void getStatusGSM() {
  while (!serialGSM);
  serialGSM.println("at+csq");                   // Empfangsstärke
  serialGSM.println("at+cusd=1,\"*100#\",15");   // Guthaben
  serialGSM.println("at+cops?");                 // Netz
  i = 1;
  FehlerCheck = 1;
  do
  {
    char* text = receiveBuffer();
    if (text != NULL) {
      if ( !strncmp (text, "+CSQ", 4 )) {
        strcpy(Empfang, text);
        i++;
      }
      if ( !strncmp (text, "+COPS", 5 )) {
        strcpy(Netz, text);
        i++;
      }
      if ( !strncmp (text, "+CUSD", 5 )) {
        strcpy(Guthaben, text);
        i++;
      }
    }
    FehlerCheck++;
  } while (i < 4 || FehlerCheck > 1500 );  //Abbruch nach zu langer Wartezeit
  return;
}   // Ende getStatusGSM()

void getAkkuStatus() {   // ##### Batteriestatus     Messleitung an A0
  Volt = analogRead(A0) / 810.0 * 12.38;
  char Spannung[5] = "";
  memset(Akku, 0, sizeof(Akku));
  dtostrf(Volt, 4, 2, Spannung);
  strcat(Akku, "Akku ");
  strcat(Akku, Spannung);
  strcat(Akku, " V");
  return;
}

void togglePower() {
  // schaltet das SIM900 Shield ein
  digitalWrite(9, HIGH);
  delay(1000);
  digitalWrite(9, LOW);
  delay(5000);
  Serial.println("Power on/off");
  return;
}

int SerialSpeed = 19200;

void setup() {

  pinMode(ReedKontakt, INPUT);
  pinMode(SMSStatus, INPUT);    //  SMS senden/sperren
  pinMode(A0, INPUT);           //  Akku über Spannungsteiler

  RTC.begin();
  RTC.adjust(DateTime(__DATE__, __TIME__));

  lcd.begin(20, 4);
  lcd.backlight();
  lcd.print("CatchPilot");
  delay(300);



  // togglePower();  // Shield einschalten

  Serial.begin(SerialSpeed);

  while (!Serial);
  Serial.println("Catch Pilot");
  Serial.println();

  Serial.println("Serial bereit");

  serialGSM.begin(SerialSpeed);
  while (!serialGSM);
  Serial.println("GSM bereit");

  serialGSM.println("AT+IPR=19200");  // AT Befehl zum setzen der Geschwindigkeit

  lcd.clear();
}

void loop() {

  getAkkuStatus();
  lcd.setCursor(9, 1);
  lcd.print(Akku);
  delay(50);

  SMSStatus = digitalRead(SchalterSMS);
  if ( SMSStatus == 1) {
    lcd.setCursor(0, 1);
    lcd.print("SMS on ");
  }
  else {
    lcd.setCursor(0, 1);
    lcd.print("SMS off");
  }

  DateTime now = RTC.now();

  // Datum
  memset(Datum, 0, sizeof(Datum));
  x = now.day();
  itoa(x, y, 10);
  if (x < 10)  {
    strcat (Datum, "0");
  }
  strcat (Datum, y);
  strcat (Datum, ".");
  x = now.month();
  itoa(x, y, 10);
  if (x < 10)  {
    strcat (Datum, "0");
  }
  strcat (Datum, y);
  strcat (Datum, ".");
  x = now.year();
  itoa(x, y, 10);
  strcat (Datum, y);

  lcd.setCursor(10, 0);
  lcd.print(Datum);

  //  Uhrzeit

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


  lcd.setCursor(0, 0);
  lcd.print(Uhrzeit);

  // ####  Tägliche Statusmeldungen  #########################

  if ( !strncmp ( Uhrzeit, "09:00:00", 8 ) || !strncmp ( Uhrzeit, "20:00:00", 8 )) {
    memset(Nachricht, 0, sizeof(Nachricht));  // löscht Nachricht
    getStatusGSM();
    strcpy(Nachricht, "Statusmeldung ");
    strcat(Nachricht, Uhrzeit);
    strcat(Nachricht, " : ");
    strcat(Nachricht, Akku);
    strcat(Nachricht, " : Empfang ");
    strcat(Nachricht, Empfang);
    strcat(Nachricht, " : Netz ");
    strcat(Nachricht, Netz);
    strcat(Nachricht, " : Guthaben ");
    strcat(Nachricht, Guthaben);
    strcat(Nachricht, " : Falle bereit");
    Serial.println(Telefonnummer);
    Serial.println(Nachricht);
    Serial.println();
    lcd.setCursor(0, 2);
    lcd.print("StatusSMS: ");
    lcd.print(Uhrzeit);

    if ( SMSStatus == 1 && FehlerCheck < 1500 ) {    // ###### Senden der SMS
      serialGSM.println("AT+CMGF=1");
      delay(1000);
      serialGSM.print(Telefonnummer);
      delay(1000);
      serialGSM.println(Nachricht);
      delay(100);
      serialGSM.println((char)26);
      delay(1000);
    }
    delay(1000);
  }


  // Abfrage Reed-Kontakt
  int FangStatus = digitalRead(ReedKontakt);

  if (FangStatus == 1)  {
    lcd.setCursor(0, 3);
    lcd.print("Falle bereit     ");
    Fang = 0;
  }

  else  {
    lcd.setCursor(0, 3);
    lcd.print("Fang um ");
    lcd.print(Fangzeit);
    if ( Fang == 0 ) {
      memcpy (Fangzeit, Uhrzeit, 9);
      memset(Nachricht, 0, sizeof(Nachricht));
      strcpy(Nachricht, "Falle ausgeloest um ");
      strcat(Nachricht, Fangzeit);
      strcat(Nachricht, " : ");
      strcat(Nachricht, Akku);
      Serial.print(Telefonnummer);
      Serial.print(" : ");
      Serial.print(Nachricht);
      Serial.println();
      Fang = 1;  // Verriegelung bis Falle wieder bereit
      if ( SMSStatus == 1 ) {    // ###### Senden der SMS
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


}
