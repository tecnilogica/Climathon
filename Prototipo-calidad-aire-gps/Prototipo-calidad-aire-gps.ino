#include <SD.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
static const int RXPin = 4, TXPin = 5;
static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);

#include "PMS.h"
#include <ESP8266WiFi.h>
bool updateGPS = false;
uint32_t timeStandby = 10000;

PMS pms(Serial);
PMS::DATA data;

#define PINSD 15
File logFile;
String dataFinal = "";

void setup()
{
  WiFi.forceSleepBegin();
  Serial.begin(9600);
  ss.begin(GPSBaud);
  pms.passiveMode();
  if (!SD.begin(PINSD)) {
    Serial.println("Inicialización fallida! Inserte SD...");
  } else {
    Serial.println("Inicialización OK!");
  }

  Serial.println();
  Serial.println(F("Sats HDOP  Latitude   Longitude   Fix  Date       Time     Date Alt"));
  Serial.println(F("           (deg)      (deg)       Age                      Age  (m)"));
  Serial.println(F("-----------------------------------------------------------------------"));
}

void loop()
{
  pms.wakeUp();
  while (!gps.location.isUpdated() && millis() < timeStandby) {
    Serial.print(printInt(gps.satellites.value(), gps.satellites.isValid(), 5));
    Serial.print(printFloat(gps.hdop.hdop(), gps.hdop.isValid(), 6, 1));
    Serial.print(printFloat(gps.location.lat(), gps.location.isValid(), 11, 6));
    Serial.print(printFloat(gps.location.lng(), gps.location.isValid(), 12, 6));
    Serial.print(printInt(gps.location.age(), gps.location.isValid(), 5));
    Serial.print(printDateTime(gps.date, gps.time));
    Serial.print(printFloat(gps.altitude.meters(), gps.altitude.isValid(), 7, 2));
    Serial.println();
    smartDelay(1000);
  }
  if (gps.location.isUpdated()) {
    updateGPS = true;
  }
  if (updateGPS) {
    dataFinal += printDateTime(gps.date, gps.time) + ";";
    dataFinal += printFloat(gps.location.lat(), gps.location.isValid(), 11, 6) + ";";
    dataFinal += printFloat(gps.location.lng(), gps.location.isValid(), 12, 6) + ";";
    dataFinal += printFloat(gps.altitude.meters(), gps.altitude.isValid(), 7, 2) + ";";
  } else {
    dataFinal += printInt(gps.location.age(), gps.location.isValid(), 5) + ";";
    dataFinal += printDateTime(gps.date, gps.time) + ";";
    dataFinal += "************;**************;";
    //Serial.println("Sin señal, durmiendo...");
    //ESP.deepSleep(300e6, WAKE_RF_DISABLED);
  }

  if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println(F("GPS sin señal"));
  }
  //******************************PRUEBA
  //pms.wakeUp();
  //delay(30000);
  Serial.println("Enviando petición de lectura...");
  pms.requestRead();
  Serial.println("Esperando para leer...");
  if (pms.readUntil(data))
  {
    Serial.print("PM 1.0 (ug/m3): ");
    Serial.println(data.PM_AE_UG_1_0);

    Serial.print("PM 2.5 (ug/m3): ");
    Serial.println(data.PM_AE_UG_2_5);

    dataFinal += (data.PM_AE_UG_1_0);
    dataFinal += (";");
    dataFinal += (data.PM_AE_UG_2_5);
    dataFinal += (";");
    dataFinal += (data.PM_AE_UG_10_0);

    Serial.print("PM 10.0 (ug/m3): ");
    Serial.println(data.PM_AE_UG_10_0);
  }
  else
  {
    Serial.println("No hay datos.");
  }
  logFile = SD.open("data.txt", FILE_WRITE);
  if (logFile) {
    Serial.print("Guardando datos: ");
    Serial.println(dataFinal);
    logFile.println("-------------LOG------------");
    logFile.println(dataFinal);
    logFile.println("----------------------------");
    logFile.close();
    Serial.println("Guardados!");
  }
  Serial.println("Duermiendo 30 seconds.");
  pms.sleep();
  ESP.deepSleep(30e6, WAKE_RF_DISABLED);
}

static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

String printFloat(float val, bool valid, int len, int prec)
{
  String text = "";
  if (!valid)
  {
    while (len-- > 1)
      text += ('*');
    text += (' ');
  }
  else
  {
    text += (val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i = flen; i < len; ++i)
      text += (' ');
  }
  smartDelay(0);
  return text;
}

String printInt(unsigned long val, bool valid, int len)
{
  String text = "";
  char sz[32] = "*****************";
  if (valid)
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i = strlen(sz); i < len; ++i)
    sz[i] = ' ';
  if (len > 0)
    sz[len - 1] = ' ';
  text += (sz);
  smartDelay(0);
  return text;
}

String printDateTime(TinyGPSDate &d, TinyGPSTime &t)
{
  String text = "";
  if (!d.isValid())
  {
    text += (F("********** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d ", d.month(), d.day(), d.year());
    text += (sz);
  }

  if (!t.isValid())
  {
    text += (F("******** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d:%02d:%02d ", t.hour() + 2, t.minute(), t.second());
    text += (sz);
  }

  printInt(d.age(), d.isValid(), 5);
  smartDelay(0);
  return text;
}
