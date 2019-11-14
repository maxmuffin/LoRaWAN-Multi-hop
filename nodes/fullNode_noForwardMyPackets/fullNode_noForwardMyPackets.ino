#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <DHT.h>
#include <DHT_U.h>
#include <LoRa.h>

unsigned long startTime;
unsigned long currentTime;
const unsigned long interval = 20000UL; // 60 seconds of relay than switch to end-node
const long sendpkt_interval = 10000;  // 10 seconds for replay received message --> forward message every t seconds.
unsigned long previousMillis = millis();

//********************************* RELAY

//array of frequencies valid for the application to change
long int frequencies[2] = {433175000, 433375000};
//controls the current frequency index in the array
int indexFreq = 0;

// used for have different frequencies for RX and TX
bool swapRX_TXFreq = false;
// used for change frequencies after transmission
bool changeFreq = false;

int bufferSize = 3;

static float freq, txfreq;
static int SF, CR, txsf;
static long BW, preLen;


void getRadioConf();//Get LoRa Radio Configure
void setLoRaRadio();//Set LoRa Radio

void receivePacket();// receive packet
void checkPreviousPacket();
void forwardPacket(); //forward received packet

void copyMessage();

void set_relay_config(); //set radio configuration of relay
void show_config();
void showPreviousMessages();
//void printChangedMode();


/* *******

       Mode 0 = receive packet
       Mode 1 = check if received packet is similar to previous send packet
       Mode 2 = send/forward received packet

*********** */

static uint8_t packet[256]; // current received packet
static uint8_t message[256]; //used for send message
static uint8_t previous_packet[256];

static int send_mode = 0; /* define mode default receive mode */

//Set Debug = 1 to enable Output;
const int debug = 1;
static int packetSize;
int receivedCount = 0;


// buffer for previous received and send message
int bufferMatrix[3][15] = {
  {0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },

  {0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },

  {0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }

};

//********************************* LORAWAN

// LoRaWAN NwkSKey, network session key
static const PROGMEM u1_t NWKSKEY[16] = { 0x6D, 0x5F, 0x0F, 0xD1, 0xA6, 0x1F, 0xAD, 0xC3, 0xEC, 0x73, 0xB1, 0xC8, 0x42, 0xB5, 0x9E, 0xD1 };

// LoRaWAN AppSKey, application session key
static const u1_t PROGMEM APPSKEY[16] = { 0x34, 0xDC, 0x88, 0xCB, 0x1B, 0x0B, 0xE1, 0x27, 0xD6, 0xD2, 0x63, 0xD9, 0x92, 0x3C, 0x49, 0x40 };

// LoRaWAN end-device address (DevAddr)
//static const u4_t DEVADDR = 0x26011032;
static const u4_t DEVADDR = 0x0067295E;

// if deviceAddress starts with 2 zero, remove the first one
// or remove thefirst zero, lower letter
//char myDeviceAddress [8] = "2611032\0";
char myDeviceAddress [8] = "067295e\0";

// These callbacks are only used in over-the-air activation, so they are left empty here (we cannot leave them out completely unless DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static osjob_t sendjob;

// Pin mapping
const lmic_pinmap lmic_pins = {
  .nss = 10,// Connected to pin D10
  .rxtx = LMIC_UNUSED_PIN,// For placeholder only, Do not connected on RFM92/RFM95
  .rst = 9,
  .dio = {2, 6, 7},// Specify pin numbers for DIO0, 1, 2
  // connected to D2, D6, D7
};

void onEvent (ev_t ev) {
  Serial.print(os_getTime());
  Serial.print(": ");
  switch (ev) {
    case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE"));
      if (LMIC.txrxFlags & TXRX_ACK)
        Serial.println(F("ack"));
      if (LMIC.dataLen) {
        Serial.println(F("Received "));
        //Serial.println(LMIC.dataLen);
        //Serial.println(F(" bytes of payload"));
      }
      break;
    default:
      Serial.println(F("Unknown event"));
      break;
  }
}

void do_send(osjob_t* j) {
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.println(F("OP_TXRXPEND"));
  } else {
    byte payload[4];
    payload[0] = highByte(random(1, 9));
    payload[1] = lowByte(random(1, 9));
    payload[2] = highByte(random(1, 9));
    payload[3] = lowByte(random(1, 9));

    LMIC_setTxData2(1, (uint8_t*)payload, sizeof(payload), 0);
    Serial.print(F("Send on freq: "));
    Serial.println(LMIC.freq);
  }
  return;
}

void setup_sendLoRaWAN() {

  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();


  // Set static session parameters. Instead of dynamically establishing a session by joining the network, precomputed session parameters are be provided.
#ifdef PROGMEM
  // On AVR, these values are stored in flash and only copied to RAM once. Copy them to a temporary buffer here, LMIC_setSession will copy them into a buffer of its own again.
  uint8_t appskey[sizeof(APPSKEY)];
  uint8_t nwkskey[sizeof(NWKSKEY)];
  memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
  memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
  LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
#else
  // If not running an AVR with PROGMEM, just use the arrays directly
  LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
#endif

#if defined(CFG_eu868)
  // Set up the channels used by the Things Network, which corresponds to the defaults of most gateways.
  LMIC_setupChannel(0, frequencies[0], DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(1, frequencies[1], DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
  LMIC_setupChannel(2, 433575000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  //LMIC_setupChannel(3, 433775000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  //LMIC_setupChannel(4, 433975000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  //LMIC_setupChannel(5, 434175000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  //LMIC_setupChannel(6, 434375000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  //LMIC_setupChannel(7, 434575000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  //LMIC_setupChannel(8, 434775000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band

#elif defined(CFG_us915)
  LMIC_selectSubBand(1);
#endif

  // Disable channel 1 and 2 for use only one freq
  LMIC_disableChannel(1);
  LMIC_disableChannel(2);

  // Disable link check validation
  LMIC_setLinkCheckMode(0);

  // TTN uses SF9 for its RX2 window.
  LMIC.dn2Dr = DR_SF9;
  // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
  LMIC_setDrTxpow(DR_SF7, 14);

  // Let LMIC offset for +/- 10% clock error
  LMIC_setClockError (MAX_CLOCK_ERROR * 10 / 100);

  // Start job
  do_send(&sendjob);

  // Disabling LMIC after send
  LMIC_shutdown();

  // need another reset?

  return;
}

void setup()
{
  Serial.begin(9600);

  startTime = millis();

  //setup of radio configuration for relay
  set_relay_config();
}

void set_relay_config() {
  //Get Radio configure
  getRadioConf();

  if ( debug > 0 ) {

    if (receivedCount == 0) {
      show_config();
    } else {
      Serial.println("Listening");
    }
    //Serial.print(F("PreambleLength: "));
    //Serial.println(preLen);
  }

  if (!LoRa.begin(freq))
    if ( debug > 0 ) Serial.println(F("init LoRa failed"));
  setLoRaRadio();// Set LoRa Radio to Semtech Chip
  delay(500);
}

void loop() {

  currentTime = millis();

  if (currentTime - startTime < interval)  //test whether the period has elapsed
  {
    if (!send_mode) {
      receivePacket();
    } else if (send_mode == 1) {
      checkPreviousPacket();
    } else {
      forwardPacket(); // SEND Packet
    }

  }
  else { //after interval of time switch relay to end-node, send LoRaWAN packet and return to relay mode
    if ( debug > 0 ) {
      Serial.print("END NODE\t");
    }
    setup_sendLoRaWAN();

    delay(500);
    if ( debug > 0 ) {
      Serial.println("\nReset LMIC");
    }
    //delay(4000);

    // restore relay configuration
    set_relay_config();
    startTime = millis();
    return;

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

/* Read transmission frequency from frequencies array
   if swapRX_TXFreq is True than use different frequencies for RX and TX
   else use the same frequency for RX and TX */
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
  int b1 = 7;

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
  /*if (receivedCount == 0) {
    Serial.println("Initial configuration. Listening on: ");
    }*/
  //Serial.println("==========================================================");
  Serial.print(F("RX Freq: "));
  Serial.print(freq);
  Serial.print(F("\tTX Freq: "));
  Serial.println(txfreq);
  Serial.print(F("Spreading Factor: "));
  Serial.print(SF);
  //Serial.print(F("\t\tTX Spreading Factor: SF"));
  //Serial.println(txsf);
  Serial.print(F("\tCR: 4/"));
  Serial.print(CR);
  Serial.print(F("\tBandwidth: "));
  Serial.println(BW);
  //Serial.println("----------------------------------------------------------");
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

  unsigned long currentMillis = millis();
  if ((currentMillis - previousMillis ) >= sendpkt_interval ) {

    previousMillis = currentMillis;
    packetSize = LoRa.parsePacket();

    if (packetSize) {   // Received a packet
      if ( debug > 0 ) {
        Serial.println();
        Serial.print(F("Get Packet: "));
        Serial.print(packetSize);
        Serial.print(F(" Bytes  "));
        Serial.print("RSSI: ");
        Serial.print(LoRa.packetRssi());
        //Serial.print("  SNR: ");
        //Serial.print(LoRa.packetSnr());
        //Serial.print(" dB  FreqErr: ");
        //Serial.println(LoRa.packetFrequencyError());
        Serial.println();

      }

      // read packet
      int i = 0;
      if ( debug > 0 ) {
        Serial.print(F("Uplink pkt: "));
        Serial.print(F("["));
      }
      while (LoRa.available() && i < 256) {
        message[i] = LoRa.read();

        if ( debug > 0 )  {
          Serial.print(message[i], HEX);
          Serial.print(F(" "));
        }

        i++;
      }
      if ( debug > 0 ) {
        Serial.print(F("]"));
        Serial.println("");
      }

      char devaddr[12] = {'\0'};
      if (debug > 0) {
        sprintf(devaddr, "%x%x%x%x", message[4], message[3], message[2], message[1]);
      }
      if (strlen(devaddr) > 8) {
        for (i = 0; i < strlen(devaddr) - 2; i++) {
          devaddr[i] = devaddr[i + 2];
        }
      }
      devaddr[i] = '\0';

      if (debug > 0) {
        Serial.print(F("Devaddr:"));
        Serial.println(devaddr);
      }

      int myDeviceSimilarities = 0;
      for (int i = 0; i < strlen(myDeviceAddress); i++) {
        if (myDeviceAddress[i] == devaddr[i]) {
          myDeviceSimilarities++;


        }
      }
      //Serial.print("---- ");
      //Serial.println(myDeviceSimilarities);

      // Check if received packet has my device info then not forward it
      if (myDeviceSimilarities == strlen(myDeviceAddress)) {
        /*if (debug > 0){
          Serial.println("Pacchetto inviato da me non inoltro");
          }*/
        //Serial.println("NOOP");
        //send_mode = 0;

      } else { //non è inviato da me

        // Increment received packet count
        receivedCount++;

        if (receivedCount > 1) {
          send_mode = 1;
        } else {
          // skip to mode 2 (SEND received packet)
          send_mode = 2;
        }


      }
      /*if (debug > 0) {
        printChangedMode();
        }*/
      return; /* exit the receive loop after received data from the node */
    }
  }

}

// Clone previous received message into buffer after transmission complete
void copyMessage() {
  int i = 0, j = 0;

  while (i <= 15) {
    bufferMatrix[receivedCount % bufferSize][i] = packet[i];
    i++;
  }

}

// Print previous messages into buffer
void showPreviousMessages() {
  Serial.println(F("Buffer: "));
  for (int i = 0; i < bufferSize; i++) {
    Serial.print(F("["));
    for (int j = 0; j <= 15; j++) {
      Serial.print(bufferMatrix[i][j], HEX);
      Serial.print(F(" "));
    }
    Serial.println(F("]"));
  }
}

// check if received message is contained into buffer
// if yes than change mode to next RX without send it

// [num] [device address] [pktCount]
// 1byte    5 bytes         1 byte
void checkPreviousPacket() {

  int equal1 = 0, equal2 = 0, equal3 = 0;
  showPreviousMessages();
  for (int i = 0; i < bufferSize; i++) {
    for (int j = 0; j <= 15; j++) {
      if (bufferMatrix[i][j] == message[j]) {
        if (i == 0) {
          equal1++;
        }
        if (i == 1) {
          equal2++;
        }
        if (i == 2) {
          equal3++;
        }
      }
    }
  }

  //equal3 = 7; //USED FOR TEST SIMILAR PACKET RECEIVED
  // Controllo se anche packet count (message[6]) è uguale
  if (equal1 > 15 || equal2 > 15 || equal3 > 15) {
    if (debug > 0) {
      //Serial.println("==========================================================");
      //Serial.println("Già inoltrato");
    }
    if (changeFreq == true || swapRX_TXFreq == true) {
      checkFrequency();
    }

    send_mode = 0;

    /*if ( debug > 0) {
      Serial.println(F("Waiting for new incoming packets using: "));
      show_config();
      }*/

  } else { // pacchetto non ancora inoltrato e lo invio
    /*if (debug > 0) {
      Serial.println("Pacchetto diverso dai precedenti");
      }*/
    //Serial.println("Diverso");
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
  /*
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

    }*/


  for (j = 0; j < 1; j++) {     // send data down one time

    LoRa.setFrequency(txfreq);
    //LoRa.setSpreadingFactor(txfreq);

    LoRa.beginPacket();
    LoRa.write(packet, i);
    LoRa.endPacket();

    delay(20);

    if (changeFreq == true || swapRX_TXFreq == true) {
      checkFrequency();
    }

    delay(100);
    Serial.println(F("FWD pkt"));
    /*if (debug > 0) {
      //Serial.print(F("[transmit] Packet forwarded successfully."));
      Serial.print("\tTransmission n°: ");
      Serial.println(receivedCount);
      //Serial.println("==========================================================");
      Serial.println("");
      }*/
    copyMessage();

    send_mode = 0; //back to receive mode

    /*if (debug > 0) {
      printChangedMode();
      }*/
  }

}

/*
  // Print status of operation mode of relay
  void printChangedMode() {
  if (send_mode == 2) {
    //Serial.println(F("Sending received packet"));
  } else if (send_mode == 0) {
    //Serial.println(F("Waiting for new incoming packets using: "));
    show_config();
  } else {
    //Serial.println(F("Checking new incoming message with previous message"));
  }
  }
*/
