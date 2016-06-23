#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <ADXL345.h>


const float alpha = 0.5;

double fXg = 0;
double fYg = 0;
double fZg = 0;

double refXg = 0;
double refYg = 0;
double refZg = 0;

int i = 0;
ADXL345 acc;


/*
   This sample sketch demonstrates the normal use of a TinyGPS++ (TinyGPSPlus) object.
   It requires the use of SoftwareSerial, and assumes that you have a
   4800-baud serial GPS device hooked up on pins 4(rx) and 3(tx).
*/
static const int RXPin = 12, TXPin = 13;
static const int GPSBaud = 9600;

// The TinyGPS++ object
TinyGPSPlus gps;

SoftwareSerial BTSerial(10, 11); // RX | TX

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

void setup()
{
  pinMode(9, OUTPUT);  // this pin will pull the HC-05 pin 34 (key pin) HIGH to switch module to AT mode
  digitalWrite(9, HIGH);
  Serial.begin(9600);
  //Serial.println("Enter AT commands:");
  BTSerial.begin(9600);  // HC-05 default speed in AT command more
  Serial.println("Setup End");

  delay(500);


  // Serial.begin(115200);
  ss.begin(GPSBaud);

  Serial.println(F("DeviceExample.ino"));
  Serial.println(F("A simple demonstration of TinyGPS++ with an attached GPS module"));
  Serial.print(F("Testing TinyGPS++ library v. ")); Serial.println(TinyGPSPlus::libraryVersion());
  Serial.println(F("by Mikal Hart"));
  Serial.println();

  acc.begin();


}

void loop()
{


  // This sketch displays information every time a new sentence is correctly encoded.
  while (ss.available() > 0)
  {
    if (gps.encode(ss.read()))
    {
      displayInfo();
    }
    /*  else
      {
        String strMessage = "STOP|0|N|0|E|0|0|0|0|0|0|0|";
        Serial.println(strMessage);  // Message
        //
        BTSerial.print(strMessage);
        delay(1000);

      }*/
  }
  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));
    while (true);
  }
}

void displayInfo()
{
  /* ARRAY DEFINITION:

     0  - START
     1  - Latitude
     2  - N (Nord)
     3  - Longitude
     4  - E (East)
     5  - month
     6  - day
     7  - year
     8  - speed (Km/h)
     9  - altitude (m)
     10 - satellites (number of satellites)
     11 - hdop (number of satellites in use)
     12 - roll
     13 - pitch

  */

  String strMessage = "";
//  Serial.print(F("Location: "));
  if (gps.location.isValid())
  {
    strMessage = "START|";
  //  Serial.print(gps.location.lat(), 6);
  //  Serial.print(F(","));
  //  Serial.print(gps.location.lng(), 6);
    strMessage += gps.location.lat(), 6;
    strMessage += "|N|";
    strMessage += gps.location.lng(), 6;
    strMessage += "|E|";

  }
  else
  {
  //  Serial.print(F("INVALID"));
    strMessage = "START|";
    strMessage += "INVALID";
    strMessage += "|N|";
    strMessage += "INVALID";
    strMessage += "|E|";

  }

 // Serial.print(F("  Date/Time: "));
  if (gps.date.isValid())
  {
 //   Serial.print(gps.date.month());
 //   Serial.print(F("/"));
 //   Serial.print(gps.date.day());
  //  Serial.print(F("/"));
  //  Serial.print(gps.date.year());

    strMessage += gps.date.month();
    strMessage += "|";
    strMessage += gps.date.day();
    strMessage += "|";
    strMessage += gps.date.year();
    strMessage += "|";

  }
  else
  {
  //  Serial.print(F("INVALID"));
    strMessage += "INVALID";
    strMessage += "|";
    strMessage += "INVALID";
    strMessage += "|";
    strMessage += "INVALID";
    strMessage += "|";
  }



  strMessage += gps.speed.kmph();
  strMessage += "|";




  strMessage += gps.altitude.meters();
  strMessage += "|";



//  Serial.println(gps.satellites.value()); // Number of satellites in use (u32)
  strMessage += gps.satellites.value();
  strMessage += "|";


 // Serial.println(gps.hdop.value()); // Number of satellites in use (u32)
  strMessage += gps.hdop.value();
  strMessage += "|";




  double pitch, roll, Xg, Yg, Zg;
  acc.read(&Xg, &Yg, &Zg);

  // Calibrazione
  if (i == 0)
  {
    refXg = Xg; refYg = Yg; refZg = Zg;
    i = 1;
  }

  Xg = Xg - refXg;
  Yg = Yg - refYg;
  Zg = Zg - refZg + 1;

  fXg = Xg * alpha + (fXg * (1.0 - alpha));
  fYg = Yg * alpha + (fYg * (1.0 - alpha));
  fZg = Zg * alpha + (fZg * (1.0 - alpha));

  // Roll & Pitch Equations
  roll  = (atan2(-fYg, fZg) * 180.0) / M_PI;
  pitch = (atan2(fXg, sqrt(fYg * fYg + fZg * fZg)) * 180.0) / M_PI;

  strMessage += roll;
  strMessage += "|";
  strMessage += pitch;
  strMessage += "|";

  Serial.println(strMessage);  // Message
  //


  BTSerial.print(strMessage);

 // Serial.println();
  delay(400);
}

