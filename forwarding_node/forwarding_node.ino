/*
  LoRaWAN packet forwarder example :
  Support Devices: LG01. 
  Require Library:
  https://github.com/sandeepmistry/arduino-LoRa 
  
  Example sketch showing how to get LoRaWAN packets from LoRaWAN node and forward it to LoRaWAN Server
  It is designed to work with the other example Arduino_LMIC
  modified 10 May 2017
  by Dragino Tech <support@dragino.com>
  Original Topic from http://qiita.com/openwave-co-jp/items/7edb3661ab5703e38e7c 
*/
#include <SPI.h>
#include <LoRa.h>
#include <Console.h>
#include <Process.h>
#include <FileIO.h>

// define servers
//String SERVER = "router.au.thethings.network"; // The Things Network
//String Port="1700";//ttn
// Set center frequency
uint32_t freq;
int SF,Denominator;
long SBW;
long old_time=millis();
long new_time;
long status_update_interval=30000;  //update Status every 300 seconds. 
char message[256];

/*void sendudp(String rssi, String packetSize, String freq) {
  Process p;
  delay(3000);
  p.begin("python");
  p.addParameter("/root/gwstat.py");
  p.addParameter(SERVER);
  p.addParameter(Port);
  p.addParameter(rssi);
  p.addParameter(packetSize);
  p.addParameter(freq);
  p.run();
  while (p.running());
  while (p.available()) {
    char c = p.read();
    Console.print(c);
  }
  Console.flush();
}

//Update Gateway Status to IoT Server
void sendstat() {
  sendudp("stat","",String((double)freq/1000000));
}
*/
void read_fre() {
  //freq=atol(fre1);
  freq = 433175000;
  //Console.flush();
}
void read_SF() {
  
  //SF=atoi(sf1);
  SF = 7;
  
}
void read_CR() {
  //Denominator=atoi(cr1);
  Denominator=5;
  //Console.flush();
}
void read_SBW() {
  int b1;

  b1=7;
  switch(b1)
    {
      case 0:SBW=7.8E3;break;
      case 1:SBW=10.4E3;break;
      case 2:SBW=15.6E3;break;
      case 3:SBW=20.8E3;break;
      case 4:SBW=31.25E3;break;
      case 5:SBW=41.7E3;break;
      case 6:SBW=62.5E3;break;
      case 7:SBW=125E3;break;
      case 8:SBW=250E3;break;
      case 9:SBW=500E3;break;
      default:break;
    }
  //Console.flush();
}

void read_config()
{
  read_fre();
  read_SF();
  read_CR();
  read_SBW();
}

void show_config()
{
  Serial.print("The frequency is ");Serial.print(freq);Serial.println("Hz");
  Serial.print("The spreading factor is ");Serial.println(SF);
  Serial.print("The coderate is ");Serial.println(Denominator);
  Serial.print("The single bandwith is ");Serial.print(SBW);Serial.println("Hz");
}

//Receiver LoRa packets and forward it 
void receivepacket() {
  // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packet
    Serial.print("Received packet '");

    // read packet
    int i = 0;
    //char message[256];
    while (LoRa.available() && i < 256) {
      message[i]=LoRa.read();
      Serial.print(message[i],HEX);
      i++;
    }
    Serial.print("'");
    Serial.println("");
        
    FileSystem.begin();
    File dataFile = FileSystem.open("/root/data/bin", FILE_WRITE);
    for(int j=0;j<i;j++)
        dataFile.print(message[j]);
    dataFile.close();
        
    delay(1000);
    //send the messages
    //sendudp(String(LoRa.packetRssi()), String(packetSize),String((double)freq/1000000));
  }
}

void setup() {
  Serial.begin(9600);
 
  while (!Serial);
  Serial.println("LoRa Receiver");
  read_config();
  if (!LoRa.begin(freq)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.setSpreadingFactor(SF);
  LoRa.setSignalBandwidth(SBW);
  LoRa.setCodingRate4(Denominator);
  LoRa.setSyncWord(0x34); 
  LoRa.receive(0);
  show_config();       
  //sendstat();
}

void loop() {
  receivepacket();
  new_time = millis();

  // Update Gateway Status
  if( (new_time-old_time) > status_update_interval){
      Serial.println("");
      Serial.println("Update Status");
      old_time = new_time;
      //sendstat();
      mkfile();
  }
}
void mkfile()
{
  FileSystem.begin();
  File file_name = FileSystem.open("/tmp/iot/data3",FILE_WRITE);
  file_name.print((char *)message);
  file_name.close();
}
