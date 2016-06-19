#include <SoftwareSerial.h>

boolean gpsStatus[] = {false, false, false, false, false, false, false};
unsigned long start;
boolean grabNext = false;
int pointerGPS = 0;
String dataGPS[10] ;

SoftwareSerial BTSerial(10, 11); // RX | TX
SoftwareSerial gpsSerial(12, 13); // RX, TX


void setup()
{
  pinMode(9, OUTPUT);  // this pin will pull the HC-05 pin 34 (key pin) HIGH to switch module to AT mode
  digitalWrite(9, HIGH);
  Serial.begin(9600);
  //Serial.println("Enter AT commands:");
  BTSerial.begin(9600);  // HC-05 default speed in AT command more
  Serial.println("Setup End");

  delay(100);
   
  // Configurazione GPS
  gpsSerial.begin(9600);
  byte settingsArray[] = {0x03, 0xFA, 0x00, 0x00, 0xE1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; //
  configureUblox(settingsArray);

  
}

void loop()
{
  // Keep reading from HC-05 and send to Arduino Serial Monitor
  if(BTSerial.available())
  {	
	 // Read string from Raspberry
   delay(10);
   //ch1 = BTSerial.read();
  }

  char recvChar;
  String sentence; 
  while(1) {
    if(gpsSerial.available())
    {
    // THIS IS THE MAIN LOOP JUST READS IN FROM THE GPS SERIAL AND ECHOS OUT TO THE ARDUINO SERIAL.
      recvChar = gpsSerial.read();
      if(String(recvChar) == ",") {
        if(grabNext){
          dataGPS[pointerGPS] = sentence;
          //Serial.println(dataGPS[pointerGPS]);
          pointerGPS = pointerGPS -1;
          if(pointerGPS <1){
            grabNext = false;
             // print out data
          
       /*      Serial.print("Fix Quality");    
             Serial.println(dataGPS[6]);  // Time
            
             
             Serial.println(dataGPS[5]);  // Time
             Serial.print(dataGPS[4]);    
             Serial.println(dataGPS[3]);  // North
             Serial.print(dataGPS[2]);
             Serial.println(dataGPS[1]);  // West

*/
          String strMessage= dataGPS[9] +"|"+dataGPS[8] +"|"+dataGPS[7] +"|"+dataGPS[6] +"|"+ dataGPS[5]+"|"+dataGPS[4]+"|"+dataGPS[3]+"|"+dataGPS[2]+"|"+dataGPS[1];
           Serial.println(strMessage);  // Message
             //
           BTSerial.print(strMessage);
               
          }
        }
        if(sentence == "M$GPGGA"){
          grabNext = true; 
          pointerGPS = 9;
        }
        sentence ="";
      }
      else{
      sentence = sentence + String(recvChar);
      } 
    }
  }

  
}


void configureUblox(byte *settingsArrayPointer) {
  byte gpsSetSuccess = 0;
  Serial.println("Configuring u-Blox GPS initial state...");

  //Generate the configuration string for Navigation Mode
  byte setNav[] = {0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, *settingsArrayPointer, 0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x2C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  calcChecksum(&setNav[2], sizeof(setNav) - 4);

  //Generate the configuration string for Data Rate
  byte setDataRate[] = {0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, settingsArrayPointer[1], settingsArrayPointer[2], 0x01, 0x00, 0x01, 0x00, 0x00, 0x00};
  calcChecksum(&setDataRate[2], sizeof(setDataRate) - 4);

  //Generate the configuration string for Baud Rate
  byte setPortRate[] = {0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00, settingsArrayPointer[3], settingsArrayPointer[4], settingsArrayPointer[5], 0x00, 0x07, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  calcChecksum(&setPortRate[2], sizeof(setPortRate) - 4);

  byte setGLL[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x2B};
  byte setGSA[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x32};
  byte setGSV[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x39};
  byte setRMC[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x40};
  byte setVTG[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x46};

  delay(2500);

  while(gpsSetSuccess < 3)
  {
    Serial.print("Setting Navigation Mode... ");
    sendUBX(&setNav[0], sizeof(setNav));  //Send UBX Packet
    gpsSetSuccess += getUBX_ACK(&setNav[2]); //Passes Class ID and Message ID to the ACK Receive function
    if (gpsSetSuccess == 5) {
      gpsSetSuccess -= 4;
      setBaud(settingsArrayPointer[4]);
      delay(1500);
      byte lowerPortRate[] = {0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00, 0x80, 0x25, 0x00, 0x00, 0x07, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA2, 0xB5};
      sendUBX(lowerPortRate, sizeof(lowerPortRate));
      gpsSerial.begin(9600);
      delay(2000);
    }
    if(gpsSetSuccess == 6) gpsSetSuccess -= 4;
    if (gpsSetSuccess == 10) gpsStatus[0] = true;
  }
  if (gpsSetSuccess == 3) Serial.println("Navigation mode configuration failed.");
  gpsSetSuccess = 0;
  while(gpsSetSuccess < 3) {
    Serial.print("Setting Data Update Rate... ");
    sendUBX(&setDataRate[0], sizeof(setDataRate));  //Send UBX Packet
    gpsSetSuccess += getUBX_ACK(&setDataRate[2]); //Passes Class ID and Message ID to the ACK Receive function
    if (gpsSetSuccess == 10) gpsStatus[1] = true;
    if (gpsSetSuccess == 5 | gpsSetSuccess == 6) gpsSetSuccess -= 4;
  }
  if (gpsSetSuccess == 3) Serial.println("Data update mode configuration failed.");
  gpsSetSuccess = 0;


  while(gpsSetSuccess < 3 && settingsArrayPointer[6] == 0x00) {
    Serial.print("Deactivating NMEA GLL Messages ");
    sendUBX(setGLL, sizeof(setGLL));
    gpsSetSuccess += getUBX_ACK(&setGLL[2]);
    if (gpsSetSuccess == 10) gpsStatus[2] = true;
    if (gpsSetSuccess == 5 | gpsSetSuccess == 6) gpsSetSuccess -= 4;
  }
  if (gpsSetSuccess == 3) Serial.println("NMEA GLL Message Deactivation Failed!");
  gpsSetSuccess = 0;

  while(gpsSetSuccess < 3 && settingsArrayPointer[7] == 0x00) {
    Serial.print("Deactivating NMEA GSA Messages ");
    sendUBX(setGSA, sizeof(setGSA));
    gpsSetSuccess += getUBX_ACK(&setGSA[2]);
    if (gpsSetSuccess == 10) gpsStatus[3] = true;
    if (gpsSetSuccess == 5 | gpsSetSuccess == 6) gpsSetSuccess -= 4;
  }
  if (gpsSetSuccess == 3) Serial.println("NMEA GSA Message Deactivation Failed!");
  gpsSetSuccess = 0;

  while(gpsSetSuccess < 3 && settingsArrayPointer[8] == 0x00) {
    Serial.print("Deactivating NMEA GSV Messages ");
    sendUBX(setGSV, sizeof(setGSV));
    gpsSetSuccess += getUBX_ACK(&setGSV[2]);
    if (gpsSetSuccess == 10) gpsStatus[4] = true;
    if (gpsSetSuccess == 5 | gpsSetSuccess == 6) gpsSetSuccess -= 4;
  }
  if (gpsSetSuccess == 3) Serial.println("NMEA GSV Message Deactivation Failed!");
  gpsSetSuccess = 0;

  while(gpsSetSuccess < 3 && settingsArrayPointer[9] == 0x00) {
    Serial.print("Deactivating NMEA RMC Messages ");
    sendUBX(setRMC, sizeof(setRMC));
    gpsSetSuccess += getUBX_ACK(&setRMC[2]);
    if (gpsSetSuccess == 10) gpsStatus[5] = true;
    if (gpsSetSuccess == 5 | gpsSetSuccess == 6) gpsSetSuccess -= 4;
  }
  if (gpsSetSuccess == 3) Serial.println("NMEA RMC Message Deactivation Failed!");
  gpsSetSuccess = 0;

  while(gpsSetSuccess < 3 && settingsArrayPointer[10] == 0x00) {
    Serial.print("Deactivating NMEA VTG Messages ");
    sendUBX(setVTG, sizeof(setVTG));
    gpsSetSuccess += getUBX_ACK(&setVTG[2]);
    if (gpsSetSuccess == 10) gpsStatus[6] = true;
    if (gpsSetSuccess == 5 | gpsSetSuccess == 6) gpsSetSuccess -= 4;
  }
  if (gpsSetSuccess == 3) Serial.println("NMEA VTG Message Deactivation Failed!");

  gpsSetSuccess = 0;
  if (settingsArrayPointer[4] != 0x25) {
    Serial.print("Setting Port Baud Rate... ");
    sendUBX(&setPortRate[0], sizeof(setPortRate));
    setBaud(settingsArrayPointer[4]);
    Serial.println("Success!");
    delay(500);
  }
}


void calcChecksum(byte *checksumPayload, byte payloadSize) {
  byte CK_A = 0, CK_B = 0;
  for (int i = 0; i < payloadSize ;i++) {
    CK_A = CK_A + *checksumPayload;
    CK_B = CK_B + CK_A;
    checksumPayload++;
  }
  *checksumPayload = CK_A;
  checksumPayload++;
  *checksumPayload = CK_B;
}

void sendUBX(byte *UBXmsg, byte msgLength) {
  for(int i = 0; i < msgLength; i++) {
    gpsSerial.write(UBXmsg[i]);
    gpsSerial.flush();
  }
  gpsSerial.println();
  gpsSerial.flush();
}


byte getUBX_ACK(byte *msgID) {
  byte CK_A = 0, CK_B = 0;
  byte incoming_char;
  boolean headerReceived = false;
  unsigned long ackWait = millis();
  byte ackPacket[10] = {0xB5, 0x62, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  int i = 0;
  while (1) {
    if (gpsSerial.available()) {
      incoming_char = gpsSerial.read();
      if (incoming_char == ackPacket[i]) {
        i++;
      }
      else if (i > 2) {
        ackPacket[i] = incoming_char;
        i++;
      }
    }
    if (i > 9) break;
    if ((millis() - ackWait) > 1500) {
      Serial.println("ACK Timeout");
      return 5;
    }
    if (i == 4 && ackPacket[3] == 0x00) {
      Serial.println("NAK Received");
      return 1;
    }
  }

  for (i = 2; i < 8 ;i++) {
  CK_A = CK_A + ackPacket[i];
  CK_B = CK_B + CK_A;
  }
  if (msgID[0] == ackPacket[6] && msgID[1] == ackPacket[7] && CK_A == ackPacket[8] && CK_B == ackPacket[9]) {
    Serial.println("Success!");
    Serial.print("ACK Received! ");
    printHex(ackPacket, sizeof(ackPacket));
    return 10;
        }
  else {
    Serial.print("ACK Checksum Failure: ");
    printHex(ackPacket, sizeof(ackPacket));
    delay(1000);
    return 1;
  }
}


void printHex(uint8_t *data, uint8_t length) // prints 8-bit data in hex
{
  char tmp[length*2+1];
  byte first ;
  int j=0;
  for (byte i = 0; i < length; i++)
  {
    first = (data[i] >> 4) | 48;
    if (first > 57) tmp[j] = first + (byte)7;
    else tmp[j] = first ;
    j++;

    first = (data[i] & 0x0F) | 48;
    if (first > 57) tmp[j] = first + (byte)7;
    else tmp[j] = first;
    j++;
  }
  tmp[length*2] = 0;
  for (byte i = 0, j = 0; i < sizeof(tmp); i++) {
    Serial.print(tmp[i]);
    if (j == 1) {
      Serial.print(" ");
      j = 0;
    }
    else j++;
  }
  Serial.println();
}

void setBaud(byte baudSetting) {
  if (baudSetting == 0x12) gpsSerial.begin(4800);
  if (baudSetting == 0x4B) gpsSerial.begin(19200);
  if (baudSetting == 0x96) gpsSerial.begin(38400);
  if (baudSetting == 0xE1) gpsSerial.begin(57600);
  if (baudSetting == 0xC2) gpsSerial.begin(115200);
  if (baudSetting == 0x84) gpsSerial.begin(230400);
}

