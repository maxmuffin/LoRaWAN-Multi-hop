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


//array of frequencies valid for the application to change
long int frequencies[3] = {433175000, 433375000, 433575000};
//controls the current frequency index in the array
int indexFreq = 0;
int indexFreqTmp = 0;
int receivedCount = 0;
String packet ;

// define servers
//String SERVER = "router.au.thethings.network"; // The Things Network
//String Port="1700";//ttn
// Set center frequency
uint32_t freq, freq0, freq1, freq2;
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

void checkFrequency()
{
  //the first byte of the packet has the frequency index the sender is using,
  //we copy this value into CMD variable to do the checks
  //String cmd = String(packet.charAt(0));
  //if the received index is different from the current index, then we change the frequency
  /*if(cmd.toInt() != indexFreq)
  {
      if (!LoRa.begin(frequencies[cmd.toInt()]))
      {
        Serial.println("Starting LoRa failed!");
        while (1);
      }
      indexFreq = cmd.toInt();
      Serial.println("Frequenza cambiata");
      //LoRa.setFrequency(frequencies[indexFreq]);
      //LoRa.enableCrc();
      show_config();
  }*/

  // For incremental index order
  // END NODE AND FORWARDING NODE NEED TO BE SYNCHRONIZED
/*
  indexFreqTmp=receivedCount%3;
  if(indexFreqTmp == 2){
    indexFreq=1;
  }else{
    if(indexFreqTmp == 1){
      indexFreq=2;
    }else{
      indexFreq=0;
    }
  }
*/
  //if is not useful to use incremental order
  indexFreq=receivedCount%3;
  show_config();
}
void read_fre() {
  //freq=atol(fre1);
  freq = frequencies[indexFreq];

}
void read_SF() {

  //SF=atoi(sf1);
  SF = 7;

}
void read_CR() {
  //Denominator=atoi(cr1);
  Denominator=5;

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
  Serial.print("The frequency is ");Serial.print(frequencies[indexFreq]);Serial.println("Hz");
  Serial.print("The spreading factor is ");Serial.println(SF);
  Serial.print("The coderate is ");Serial.println(Denominator);
  Serial.print("The single bandwith is ");Serial.print(SBW);Serial.println("Hz");
  Serial.print("Index: ");Serial.println(indexFreq);
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
      
      //packet += (char) LoRa.read();
    }
    Serial.print("'");
    Serial.println("");
    Serial.print("Received ");
    Serial.print(packetSize);
    Serial.println(" bytes");
    receivedCount++;

    Serial.println("");
    Serial.print("**** Waiting for next trasmission --> trasmission n° ");Serial.print(receivedCount+1);Serial.println(" ****");

    //delay(1000);
    //send the messages
    //sendudp(String(LoRa.packetRssi()), String(packetSize),String((double)freq/1000000));
    checkFrequency();
  }
}

void setup() {
  Serial.begin(9600);

  while (!Serial);
  Serial.println("LoRa Receiver");
  read_config();
  /*
  if (!LoRa.begin(freq)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }*/
  if (!LoRa.begin(frequencies[indexFreq]))
  {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.enableCrc();

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
      //Serial.println("");
      //Serial.println("Update Status");
      old_time = new_time;
      //sendstat();
      //mkfile();
  }
}
/*
void mkfile()
{
  FileSystem.begin();
  File file_name = FileSystem.open("/tmp/iot/data3",FILE_WRITE);
  file_name.print((char *)message);
  file_name.close();
}*/
