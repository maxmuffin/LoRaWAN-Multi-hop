#include <Process.h>
#include <SPI.h>
#include <LoRa.h>
const String Sketch_Ver = "relayNode_v2";

// GESTIRE DOWNLINK IN CASO DI GATEWAY DUAL CHANNEL
// USARE GATEWAY SINGLE CHANNEL

//array of frequencies valid for the application to change
long int frequencies[3] = {433175000, 433375000, 433575000};
//controls the current frequency index in the array
int indexFreq = 0;

static float freq, txfreq;
static int SF, CR, txsf;
static long BW, preLen;
static long old_time = millis();
static long new_time;
static unsigned long newtime;
const long sendpkt_interval = 15000;  // 15 seconds for replay.
const long interval = 60000;          //1min for feeddog.
unsigned long previousMillis = millis();
unsigned long previousMillis_1 = millis();

void getRadioConf();//Get LoRa Radio Configure from LG01
void setLoRaRadio();//Set LoRa Radio
void receivepacket();// receive packet
void sendpacket(); //send join accept payload
void emitpacket(); //send ddata down
void writeVersion();
void feeddog();

/* ***********************************************
 *
 *     Mode 0 = receive packet
 *     Mode 1 = prepare packet for send // join request if is new device
 *     Mode 2 = send/forward received packet
 *
************************************************ */

static uint8_t packet[256];
static uint8_t message[256];
static uint8_t packet1[64];
static int send_mode = 0; /* define mode default receive mode */

//Set Debug = 1 to enable Output;
const int debug = 1;
static int packetSize;
int receivedCount = 0;
static char dwdata[32] = {'\0'};  // for data down payload


void setup(){
    Serial.begin(9600);

    if ( debug > 0 ){
        Serial.print(F("Sketch Version:"));
        Serial.println(Sketch_Ver);
        Serial.println(F("Start LoRaWAN Single Channel Forwarding Node"));
        Serial.println("");
      }

    //Get Radio configure
    getRadioConf();

    if ( debug > 0 ){

        show_config();
        //Serial.print(F("PreambleLength: "));
        //Serial.println(preLen);
    }

    if (!LoRa.begin(freq))
        if ( debug > 0 ) Serial.println(F("init LoRa failed"));
    setLoRaRadio();// Set LoRa Radio to Semtech Chip
    delay(1000);
}

void loop(){
    if (!send_mode) {
        receivepacket();          /* received message and wait server downstream */
    } else if (send_mode == 1) { //JOIN MODE
        sendpacket();
    } else {
        emitpacket(); // SEND Packet
    }
    /*unsigned long currentMillis = millis();
    if ((currentMillis - previousMillis ) >= interval){
      previousMillis = currentMillis;
        //feeddog();

    }*/
}

//Get LoRa Radio Configure from LG01
void getRadioConf() {

    read_freq();
    read_txfreq();
    read_SF();
    read_txSF();
    read_CR();
    read_SBW();
}
void show_config(){
  if(receivedCount == 0){
    Serial.println("Initial configuration. Listening on: ");
  }
  Serial.println("==========================================================");
  Serial.print(F("RX Frequency: "));
  Serial.print(freq);
  Serial.print(F("\tTX Frequency: "));
  Serial.println(txfreq);
  Serial.print(F("Spreading Factor: SF"));
  Serial.print(SF);
  Serial.print(F("\t\tTX Spreading Factor: SF"));
  Serial.println(txsf);
  Serial.print(F("Coding Rate: 4/"));
  Serial.print(CR);
  Serial.print(F("\t\tBandwidth: "));
  Serial.println(BW);
  Serial.println("==========================================================");
}

void checkFrequency()
{
  // Update frequencies index
  indexFreq=receivedCount%3;
  getRadioConf();
  //show_config();
}

void read_freq() { freq = frequencies[indexFreq]; }

void read_txfreq() {
  if (indexFreq ==2){
    txfreq = frequencies[0];
  }else{
    txfreq = frequencies[indexFreq+1];
  }
}

void read_SF() { SF = 7; }

void read_txSF(){ txsf = 9; }
void read_CR() {
  // 4/CR
  CR=5;
}

void read_SBW() {
  int b1;

  b1=7;
  switch(b1)
    {
      case 0:BW=7.8E3;break;
      case 1:BW=10.4E3;break;
      case 2:BW=15.6E3;break;
      case 3:BW=20.8E3;break;
      case 4:BW=31.25E3;break;
      case 5:BW=41.7E3;break;
      case 6:BW=62.5E3;break;
      case 7:BW=125E3;break;
      case 8:BW=250E3;break;
      case 9:BW=500E3;break;
      default:break;
    }
}

void setLoRaRadio() {
    LoRa.setFrequency(freq);
    LoRa.setSpreadingFactor(SF);
    LoRa.setSignalBandwidth(BW);
    LoRa.setCodingRate4(CR);
    LoRa.setSyncWord(0x34);
    //LoRa.setPreambleLength(preLen);
}

//Receiver LoRa packets and forward it
void receivepacket() {
    // try to parse packet
    LoRa.setSpreadingFactor(SF);
    LoRa.receive(0);

   unsigned long currentMillis_1 = millis();
    if ((currentMillis_1 - previousMillis_1 ) >= sendpkt_interval ){

     previousMillis_1 = currentMillis_1;
     packetSize = LoRa.parsePacket();

     if (packetSize) {   // Received a packet
       if ( debug > 0 ) {
         Serial.println();
         Serial.print(F("Get Packet: "));
         Serial.print(packetSize);
         Serial.print(F(" Bytes  "));
         Serial.print("RSSI: ");
         Serial.print(LoRa.packetRssi());
         Serial.print("  SNR: ");
         Serial.print(LoRa.packetSnr());
         Serial.print(" dB  FreqErr: ");
         Serial.println(LoRa.packetFrequencyError());

      }
        // read packet
        int i = 0;

        //memset(message, 0, sizeof(message)); /* make sure message is empty */
        Serial.print(F("Uplink Message: "));
        Serial.print(F("["));
        while (LoRa.available() && i < 256) {
            message[i] = LoRa.read();

          if ( debug > 0 )  {
            Serial.print(message[i], HEX);
            Serial.print(F(" "));
          }

          i++;
        }    /* end of while lora.available */
        Serial.print(F("]"));

        // Increment received packet count
        receivedCount++;

        if ( debug > 0 ) Serial.println("");

        if ((int)message[0] == 0) {      /* Join Request */
          send_mode = 1;  /* change the mode */
          return;
        }

        /* process Data down */
        char devaddr[12] = {'\0'};
        sprintf(devaddr, "%x%x%x%x", message[4], message[3], message[2], message[1]);
        if (strlen(devaddr) > 8) {
          for (i = 0; i < strlen(devaddr) - 2; i++) {
            devaddr[i] = devaddr[i + 2];
          }
        }
        devaddr[i] = '\0';

        //memset(dwdata, 0, sizeof(dwdata));
        //snprintf(dwdata, sizeof(dwdata), "/var/iot/%s", devaddr);

        if (debug > 0) {
          Serial.print(F("Devaddr:"));
          Serial.println(devaddr);
          //Serial.print(F("Dwdata:"));
          //Serial.println(dwdata);
        }
        /*
        int res = FileSystem.exists(dwdata);
        if (res) {
          send_mode = 2;
          if (debug > 0) {
            Console.print(dwdata);
            Console.println(F(" Exists"));
          }
        }*/

        // skip to mode 2
        send_mode = 2;

        printChangedMode();
        //Serial.print(F("Received Packet, skip to mode:"));
        //if(send_mode)
        //Serial.println(send_mode, DEC);
        return; /* exit the receive loop after received data from the node */
      } /* end of if packetsize than 1 */
    } /* end of receive loop */

}

void sendpacket()
{

  int i = 0;

  old_time = millis();
  new_time = old_time;

  while (new_time - old_time < sendpkt_interval) { /* received window may be closed after 10 seconds */

    new_time = millis();

    memset(packet, 0, sizeof(packet));
    i = 0;

    // for test --> Correct: i < packetSize
    //Copy received message into new message for send
    while (i < packetSize) {
      packet[i] = message[i];
      i++;
    }

    if (i < 3) {
      delay(200);
      continue;
    }

    if ( debug > 0 ) {
      int j;
      Serial.print(F("Downlink Message: "));
      Serial.print(F("["));
      for (j = 0; j < i; j++) {
        //Serial.print(F("["));
        //Serial.print(j);
        //Serial.print(F("]"));
        Serial.print(packet[j], HEX);
        Serial.print(F(" "));
      }
      Serial.print(F("]"));
      Serial.println();
    }

    new_time = millis();


    while (new_time - old_time < sendpkt_interval - 2000) {   // 8 seconds for sending packet to node
      LoRa.beginPacket();
      LoRa.write(packet,i);
      LoRa.endPacket();
      delay(1);
      new_time = millis();
    }

    LoRa.setFrequency(txfreq);
    LoRa.setSpreadingFactor(txsf);    /* begin send data to the lora node, lmic use the second receive window, and SF default to 9 */
    delay(2);

    while (new_time - old_time < sendpkt_interval + 2000) {   // 12 seconds for sending packet to node

      LoRa.beginPacket();
      Serial.println(" INVIOOOOO");
      LoRa.write(packet, i);
      LoRa.endPacket();
      delay(1);
      new_time = millis();
    }
    LoRa.setFrequency(freq);
    LoRa.setSpreadingFactor(SF);    /* reset SF to receive message */

    if (debug > 0) Serial.println(F("[transmit] END"));
    break;
  }

  send_mode = 0;
  printChangedMode();
}

void emitpacket()
{
  int i = 0, j = 0;


  memset(packet, 0, sizeof(packet));

  while (i < packetSize) {
    packet[i] = message[i];
    i++;
  }

  if (i < 3)
    return;

  if ( debug > 0 ) {

    Serial.print(F("Downlink Message: "));
    Serial.print(F("["));
    for (j = 0; j < i; j++) {
      //Serial.print(F("["));
      //Serial.print(j);
      //Serial.print(F("]"));
      Serial.print(packet[j], HEX);
      Serial.print(F(" "));
    }
    Serial.print(F("]"));
    Serial.print("  ");
    Serial.print(packetSize);
    Serial.println(" bytes");
    Serial.println("");
  }

  for (j = 0; j < 1; j++) {     // send data down two times every frequency
    LoRa.beginPacket();
    //Serial.println("Invio");
    LoRa.write(packet, i);
    LoRa.endPacket();
    delay(10);

    LoRa.setFrequency(txfreq);
    LoRa.setSpreadingFactor(txsf);    /* begin send data to the lora node, lmic use the second receive window, and SF default to 9 */
    delay(20);

    LoRa.beginPacket();
    LoRa.write(packet, i);
    LoRa.endPacket();

    delay(20);

    checkFrequency();

    LoRa.setFrequency(freq);
    LoRa.setSpreadingFactor(SF);    /* reset SF to receive message */
    delay(500);
  }

  if (debug > 0){
    Serial.print(F("[transmit] Data Down END"));
    Serial.print("  Transmission n°: ");
    Serial.println(receivedCount);
    Serial.println("");
  }


  send_mode = 0; //back to receive mode
  printChangedMode();
}

void printChangedMode(){
  if(send_mode == 2){
      Serial.println(F("Received Packet, skip to mode: FORWARD PACKET"));
  }else{
    if(send_mode == 0){
      Serial.println(F("Waiting for incoming packets using: "));
      show_config();
    }else{
      Serial.println(F("Skipping to mode: JOIN REQUEST"));
      }
   }

}

void feeddog(){
    int k = 0;
    memset(packet1, 0, sizeof(packet1));

    Process p;    // Create a process
    p.begin("date");
    p.addParameter("+%s");
    p.run();
    while (p.available() > 0 && k < 32) {
        packet1[k] = p.read();
        k++;
    }
    newtime = atol(packet1);


}
