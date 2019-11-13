#include <Process.h>
#include <SPI.h>
#include <LoRa.h>
const String Sketch_Ver = "relayNode_multiFreq_nobuffer";

// Controlla se il pacchetto ricevuto è uguale al precedente
// Se si non lo inoltra

// GESTIRE DOWNLINK IN CASO DI GATEWAY DUAL CHANNEL
// USARE GATEWAY SINGLE CHANNEL

//array of frequencies valid for the application to change
long int frequencies[3] = {433175000, 433375000};
//controls the current frequency index in the array
int indexFreq = 0;

// used for have different frequencies for RX and TX
bool swapRX_TXFreq = true;
// used for change frequencies after transmission
bool changeFreq = true;

static float freq, txfreq;
static int SF, CR, txsf;
static long BW, preLen;
static long old_time = millis();
static long new_time;
static unsigned long newtime;
const long sendpkt_interval = 15000;  // 15 seconds for replay.
unsigned long previousMillis = millis();
unsigned long previousMillis_1 = millis();

void getRadioConf();//Get LoRa Radio Configure
void setLoRaRadio();//Set LoRa Radio

void receivePacket();// receive packet
void checkPreviousPacket();
void forwardPacket(); //forward received packet

void copyMessage();

void show_config();
void showPreviousMessage();
void printChangedMode();

/* ***********************************************

       Mode 0 = receive packet
       Mode 1 = check if received packet is similar to previous send packet
       Mode 2 = send/forward received packet

************************************************ */

static uint8_t packet[256];
static uint8_t message[256];
static uint8_t previous_packet[256];

static int send_mode = 0; /* define mode default receive mode */

//Set Debug = 1 to enable Output;
const int debug = 1;
static int packetSize;
int receivedCount = 0;


void setup() {
  Serial.begin(9600);

  if ( debug > 0 ) {
    Serial.print(F("Sketch Version:"));
    Serial.println(Sketch_Ver);
    Serial.println(F("Start LoRaWAN Single Channel Forwarding Node"));
    Serial.println("");
  }

  //Get Radio configure
  getRadioConf();

  if ( debug > 0 ) {

    show_config();
    //Serial.print(F("PreambleLength: "));
    //Serial.println(preLen);
  }

  if (!LoRa.begin(freq))
    if ( debug > 0 ) Serial.println(F("init LoRa failed"));
  setLoRaRadio();// Set LoRa Radio to Semtech Chip
  delay(1000);
}

void loop() {
  if (!send_mode) {
    receivePacket();          /* received message and wait server downstream */
  } else if (send_mode == 1) {
    checkPreviousPacket();
  } else {
    forwardPacket(); // SEND Packet
  }
}

//Get LoRa Radio Configuration
void getRadioConf() {
  read_freq();
  read_txfreq();
  read_SF();
  read_txSF();
  read_CR();
  read_SBW();
}

// Read receiver frequency from frequencies array
void read_freq() {
  freq = frequencies[indexFreq];
}

// Read transmission frequency from frequencies array
// if swapRX_TXFreq is True than use different frequencies for RX and TX
// else use the same frequency for RX and TX
void read_txfreq() {
  if (swapRX_TXFreq == true) {
    if (indexFreq == 1) {
      txfreq = frequencies[0];
    } else {
      txfreq = frequencies[indexFreq + 1];
    }
  } else {
    txfreq = frequencies[indexFreq];
  }
}

// Read Spreading Factor for receiving
void read_SF() {
  SF = 7;
}

// read Spreading Factor for transmit
void read_txSF() {
  txsf = 9;
}

// Read Coding rate 4/CR
void read_CR() {
  CR = 5;
}

// Read Single Bandwith for the transmission
void read_SBW() {
  int b1;

  b1 = 7;
  switch (b1)
  {
    case 0: BW = 7.8E3; break;
    case 1: BW = 10.4E3; break;
    case 2: BW = 15.6E3; break;
    case 3: BW = 20.8E3; break;
    case 4: BW = 31.25E3; break;
    case 5: BW = 41.7E3; break;
    case 6: BW = 62.5E3; break;
    case 7: BW = 125E3; break;
    case 8: BW = 250E3; break;
    case 9: BW = 500E3; break;
    default: break;
  }
}

// Set LoRa receiving and transmission parameters
void setLoRaRadio() {
  LoRa.setFrequency(freq);
  LoRa.setSpreadingFactor(SF);
  LoRa.setSignalBandwidth(BW);
  LoRa.setCodingRate4(CR);
  LoRa.setSyncWord(0x34);
  //LoRa.setPreambleLength(preLen);
}

// Print LoRa setting configurations
void show_config() {
  if (receivedCount == 0) {
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
  Serial.println("----------------------------------------------------------");
}

// Used for update index of frequencies
// if changeFreq is False than use always the same frequency after transmission
void checkFrequency() {
  // Update frequencies index
  if (changeFreq == true) {
    indexFreq = receivedCount % 2;
  }
  getRadioConf();
}

// Receiver LoRa packets and change mode to 1 --> CheckPreviousPacket
void receivePacket() {
  // try to parse packet
  LoRa.setSpreadingFactor(SF);
  LoRa.receive(0);

  unsigned long currentMillis_1 = millis();
  if ((currentMillis_1 - previousMillis_1 ) >= sendpkt_interval ) {

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

      if ( debug > 0 ) Serial.println("");

      /* process Data down */
      char devaddr[12] = {'\0'};
      sprintf(devaddr, "%x%x%x%x", message[4], message[3], message[2], message[1]);
      if (strlen(devaddr) > 8) {
        for (i = 0; i < strlen(devaddr) - 2; i++) {
          devaddr[i] = devaddr[i + 2];
        }
      }
      devaddr[i] = '\0';

      if (debug > 0) {
        Serial.print(F("Devaddr:"));
        Serial.println(devaddr);

        // Increment received packet count
        receivedCount++;

        if (receivedCount > 1) {
          send_mode = 1;
        } else {
          // skip to mode 2 (SEND received packet)
          send_mode = 2;
        }


        printChangedMode();
        return; /* exit the receive loop after received data from the node */
      } /* end of if packetsize than 1 */
    } /* end of receive loop */

  }
}

// Clone previous received message after transmission complete
void copyMessage() {
  int i = 0, j = 0;

  while (i <= 6) {
    previous_packet[i] = packet[i];
    i++;
  }

}

// Print previous messages into buffer
void showPreviousMessage() {
  Serial.print(F("Previous Packet: "));
  Serial.print(F("["));
  for (int j = 0; j <= 6; j++) {
    Serial.print(previous_packet[j], HEX);
    Serial.print(F(" "));
  }
  Serial.println(F("]"));
}

// check if received message is contained into previous_packet
// if yes than change mode to next RX without send it

// [num] [device address] [pktCount]
// 1byte    5 bytes         1 byte
void checkPreviousPacket() {

  int equal = 0;
  showPreviousMessage();
  for (int j = 0; j <= 6; j++) {
    if (previous_packet[j] == message[j]) {
      equal++;
    }
  }

  //equal = 7; //USED FOR TEST SIMILAR PACKET RECEIVED
  // Controllo se anche packet count (message[6]) è uguale
  if (equal > 6) {
    Serial.println("==========================================================");
    Serial.println("Già inoltrato, non invio, mi metto in ascolto di ricevere nuovi pacchetti");
    Serial.println("");
    checkFrequency();
    send_mode = 0;
    Serial.println(F("Waiting for new incoming packets using: "));
    show_config();

  } else { // pacchetto non ancora inoltrato e lo invio
    Serial.println("Pacchetto diverso dal precedente");
    send_mode = 2;
  }

}

// Forward packet to next neighbour
void forwardPacket() {
  int i = 0, j = 0;

  while (i < packetSize) {
    packet[i] = message[i];
    i++;
  }
  if ( debug > 0 ) {

    Serial.print(F("Sending Message: "));
    Serial.print(F("["));
    for (j = 0; j < i; j++) {
      Serial.print(packet[j], HEX);
      Serial.print(F(" "));
    }
    Serial.print(F("]"));
    Serial.print("  ");
    Serial.print(i);
    Serial.println(" bytes");
    Serial.println("");

  }


  for (j = 0; j < 1; j++) {     // send data down two times every frequency

    LoRa.setFrequency(txfreq);
    //LoRa.setSpreadingFactor(txfreq);

    LoRa.beginPacket();
    LoRa.write(packet, i);
    LoRa.endPacket();

    delay(20);
    checkFrequency();
    delay(500);

    if (debug > 0) {
      Serial.print(F("[transmit] Packet forwarded successfully."));
      Serial.print("\tTransmission n°: ");
      Serial.println(receivedCount);
      Serial.println("==========================================================");
      Serial.println("");
    }
    copyMessage();

    send_mode = 0; //back to receive mode
    printChangedMode();

  }

}

// Print status of operation mode of relay
void printChangedMode() {
  if (send_mode == 2) {
    Serial.println(F("Sending received packet"));
  } else if (send_mode == 0) {
    Serial.println(F("Waiting for new incoming packets using: "));
    show_config();
  } else {
    Serial.println(F("Checking new incoming message with previous message"));
  }
}
