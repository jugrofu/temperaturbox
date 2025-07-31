
/************************************************
 *  Temperaturchecker
 *
 *  (c) 20250605	chhof, jugrofu
 *  v 0.4
 *************************************************/

#include "DHT.h"
#include "EEPROM.h"

#define RELAISPIN	7
#define DHTPIN 		11
#define LEDPIN 		13
#define DHTTYPE 	DHT22   // DHT 22 (AM2302), AM2321

#define BANNER "\
############################################################ \n\r\
# Willkommen beim HJP-Temperatursensor                     # \n\r\
# (c) Ch Hof, Ju Gro Fu (06.06.2025)                       # \n\r\
#                                                          # \n\r\
# Kommandos:                                               # \n\r\
# - temp TT: Setze Alarm-Temperatur auf TT (wird           # \n\r\
#            nichtflüchtig im EEPROM gespeichert)          # \n\r\
# - peri PP: Setze Messintervall auf PP Sekunden (wird     # \n\r\
#            nichtflüchtig im EEPROM gespeichert)          # \n\r\
# - test:    Triggert Alarmzustand                         # \n\r\
# - RETURN:  Ausgabe der aktuellen Messwerte und Parameter # \n\r\
#                                                          # \n\r\
# Anmerkungen:                                             # \n\r\
# - AlarmTemp:  Kritische Temperatur                       # \n\r\
# - Periode:    Messintervall (Default: 60s)               # \n\r\
# - AlarmCount: Anzahl der Perioden mit T>Tc bis Alarm     # \n\r\
############################################################ \n\r\
"


DHT dht(DHTPIN, DHTTYPE);
uint8_t AlarmCount = 0;
uint8_t AlarmTemp = 50;
float ActTemp;
unsigned long lastcheck = 0;
uint16_t Period = 60;

uint8_t EEPROM_update(int address, uint8_t value) {
  if (EEPROM.read(address) != value) {
    EEPROM.write(address, value);
  }
  return value;
}


template <typename T> 
T &EEPROM_get(int idx, T &t) {
  uint8_t *ptr = (uint8_t*) &t;
  for (unsigned int count = 0; count < sizeof(T); ++count) {
    *ptr++ = EEPROM.read(idx++);
  }
  return t;
}


template <typename T> 
const T &EEPROM_put(int idx, const T &t) {
  const uint8_t *ptr = (const uint8_t*) &t;
  for (unsigned int count = 0; count < sizeof(T); ++count) {
    EEPROM_update(idx++, *ptr++);
  }
  return t;
}


void PrintData() {
  Serial.print("Aktuelle Werte: t = ");
  Serial.print(millis()/1000);
  Serial.print("s   T = ");
  Serial.print(ActTemp);
  Serial.print("C   AlrmCnt = ");
  Serial.print(AlarmCount);
  Serial.print("   Tc = ");
  Serial.print(AlarmTemp);
  Serial.print("C   Period = ");
  Serial.print(Period);
  Serial.print("s");
  Serial.println();
  Serial.flush();
}


void SwitchPins(int level) {
  digitalWrite(RELAISPIN, level);
  digitalWrite(LEDPIN, level);
}


void CheckTemp() {
  ActTemp = dht.readTemperature();
  if (isnan(ActTemp) || ActTemp >= AlarmTemp) {
    AlarmCount++;
    if (AlarmCount > 2) {
      SwitchPins(HIGH);
      AlarmCount--;
    }
  } else {
    AlarmCount = 0;
    SwitchPins(LOW);
  }
}


// zu Testzwecken
// extern volatile unsigned long timer0_millis;

void setup() {

  // zu Testzwecken
  // noInterrupts();
  // timer0_millis += pow(2,32) - 100000;
  // interrupts();

  digitalWrite(RELAISPIN, LOW);
  pinMode(RELAISPIN, OUTPUT);
  digitalWrite(RELAISPIN, LOW);

  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, LOW);

  AlarmTemp = EEPROM_get(0, AlarmTemp);
  Period = EEPROM_get(1, Period);

  dht.begin();

  Serial.begin(9600);
  Serial.setTimeout(10000);
  Serial.println(F(BANNER));	// String nicht im RAM ablegen!

  CheckTemp();
  PrintData();
}


void loop() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\r');
    if (cmd.startsWith("temp")) {
      uint8_t value = (uint8_t)cmd.substring(4).toInt();
      if (value >= 20 && value <= 80) {
        AlarmTemp = value;
        EEPROM_put(0, AlarmTemp);
        value = EEPROM_get(0, value);
        if (AlarmTemp == value) {
          Serial.print("Neue Alarm-Temperatur Tc = ");
          Serial.print(AlarmTemp);
          Serial.println(" erfolgreich gespeichert.");
        } else {
          Serial.println("Fehler beim Speichern der Alarmtemperatur - Bitte mit Administrator in Verbindung setzen.");
        }
      } else {
        Serial.println("Wert muss zwischen 20 und 80 liegen!");
      }

    } else if (cmd.startsWith("peri")) {
    uint16_t value = (uint16_t)cmd.substring(4).toInt();
      if (value >= 10 && value <= 3600) {
        Period = value;
        EEPROM_put(1, Period);
        value = EEPROM_get(1, value);
        if (Period == value) {
          Serial.print("Neue Periodendauer ");
          Serial.print(Period);
          Serial.println("s erfolgreich gespeichert.");
        } else {
          Serial.println("Fehler beim Speichern der Alarmtemperatur - Bitte mit Administrator in Verbindung setzen.");
        }
      } else {
        Serial.println("Wert muss zwischen 10 und 3600 liegen!");
      }

    } else if (cmd.startsWith("test")) {
      Serial.println(" ==> Alarm an...");
      SwitchPins(HIGH);
      delay(1000);
      Serial.println(" ==> Alarm aus...");
      SwitchPins(LOW);
    } else {
      CheckTemp();
      PrintData();
    }
  }


  if (millis() - lastcheck > 1000*Period || millis() < lastcheck) {
    lastcheck = millis();
    CheckTemp();
    PrintData();
  }

  if (AlarmCount >= 1) {
    if (millis() % 250 < 125) {
      digitalWrite(LEDPIN, HIGH);
    }
    else {
      digitalWrite(LEDPIN, LOW);
    }
  }
  else {
    digitalWrite(LEDPIN, HIGH);
  }
  delay(10);
}


