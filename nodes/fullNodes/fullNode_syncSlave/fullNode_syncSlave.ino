#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <DHT.h>
#include <DHT_U.h>
#include <LoRa.h>

// received rxOpen and rxClose of master, correspond of txOpen and txClose of slave
unsigned long time1, time2;
int sleepTime, RTT, TX_interval, RX_interval;
unsigned long currentTime;
unsigned long previousMillis;
unsigned long startTime;
unsigned long RXmode_startTime, RXmode_endTime, TXmode_startTime, TXmode_endTime;

int send_mode = -1;
int receivedCount = 0;

//********************************* RELAY
const byte freqArraySize = 2;
//array of frequencies valid for the application to change
long int frequencies[freqArraySize] = {433175000, 433375000};
//controls the current frequency index in the array
int indexFreq = 0;

// used for have different frequencies for RX and TX
bool swapRX_TXFreq = false;
// used for change frequencies after transmission
bool changeFreq = false;

static float freq, txfreq;
static int SF, CR, txsf;
static long BW, preLen;

/* ******* OPERATING MODE *******
    Mode -1 = wait for synch
    Mode  0 = receive packet
    Mode  1 = check if received packet is similar to previous send packet
    Mode  2 = send/forward received packet
*********** */

//Set Debug = 1 to enable Output;
const int debug = 0;
static int packetSize;
static uint8_t packet[256]; // current received packet
//static uint8_t message[256]; //used for send message
static uint8_t previous_packet[256];

const byte rowBuffer = 3;
const byte dimPreviousMessBuffer = 15;
const byte dimFwdBuffer = 30;
int index_fwdBuffer;

// buffer for previous received and send message
int prevMessBuffer[rowBuffer][dimPreviousMessBuffer];
int fwdBuffer[rowBuffer][dimFwdBuffer];

//********************************* LORAWAN

// LoRaWAN NwkSKey, network session key
static const PROGMEM u1_t NWKSKEY[16] = { 0x6D, 0x5F, 0x0F, 0xD1, 0xA6, 0x1F, 0xAD, 0xC3, 0xEC, 0x73, 0xB1, 0xC8, 0x42, 0xB5, 0x9E, 0xD1 };

// LoRaWAN AppSKey, application session key
static const u1_t PROGMEM APPSKEY[16] = { 0x34, 0xDC, 0x88, 0xCB, 0x1B, 0x0B, 0xE1, 0x27, 0xD6, 0xD2, 0x63, 0xD9, 0x92, 0x3C, 0x49, 0x40 };

// LoRaWAN end-device address (DevAddr)
//static const u4_t DEVADDR = 0x26011032; // device 1
static const u4_t DEVADDR = 0x0067295E; // device 2

// if deviceAddress starts with 2 zero, remove the first one
// or remove thefirst zero, lower letter
//char myDeviceAddress [8] = "2611032\0";  // device 1
char myDeviceAddress [8] = "067295e\0"; // device 2
int canSendLoRaWAN = 0;
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
  if ( debug > 0 ) {
    switch (ev) {
      case EV_TXCOMPLETE:
        //Serial.println(F("EV_TXCOMPLETE"));

        //if (LMIC.txrxFlags & TXRX_ACK)
        //Serial.println(F("ack"));
        //if (LMIC.dataLen) {
        //Serial.println(F("Received "));
        //Serial.println(LMIC.dataLen);
        //Serial.println(F(" bytes of payload"));
        //}
        break;
      default:
        //Serial.println(F("Unknown event"));
        break;
    }
  }
}

void do_send(osjob_t* j) {
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    if ( debug > 0 ) {
      Serial.println(F("OP_TXRXPEND"));
    }
  } else {
    byte payload[2]; //on device 1 send payload[4], on device 2 payload [2]
    payload[0] = highByte(random(1, 9));
    payload[1] = lowByte(random(1, 9));
    //payload[2] = highByte(random(1, 9));
    //payload[3] = lowByte(random(1, 9));

    LMIC_setTxData2(1, (uint8_t*)payload, sizeof(payload), 0);
    if ( debug > 0 ) {
      Serial.println(F("Send pkt"));
    }
    Serial.println(F("Send pkt"));
    //Serial.print(F("Send on freq: "));
    //Serial.println(LMIC.freq);
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

void setup() {
  Serial.begin(9600);
  initPreviousMessagesBuffer();
  initFwdBuffer();

  startTime = millis();
}

void set_relay_config() {
  //Get Radio configure
  getRadioConf();

  if (!LoRa.begin(freq))
    if ( debug > 0 ) Serial.println(F("init LoRa failed"));
  setLoRaRadio();// Set LoRa Radio to Semtech Chip
  delay(500);
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
    if (indexFreq == freqArraySize - 1) {
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

// Used for update index of frequencies
// if changeFreq is False than use always the same frequency after transmission
void checkFrequency() {
  // Update frequencies index
  if (changeFreq == true) {
    indexFreq = receivedCount % freqArraySize;
  }
  getRadioConf();
}

void initSync() {


  // Ricevi il messaggio lora e splittalo

  // solo per i test
  time1 = startTime + 10000UL; //tra 10 secondi
  time2 = startTime + 30000UL;
  sleepTime = 10 * 1000; //30 secondi sleep

  TX_interval = time2 - time1;
  RX_interval = TX_interval;

  RTT = (TX_interval + sleepTime) * 2;

  // aspetta fino alla txMode del master e imposta la receiver mode
  currentTime = millis();
  Serial.print(F("Sincronizzo   "));
  Serial.print(currentTime);
  Serial.print(F("    "));
  Serial.println(startTime + time2 + sleepTime);



  if (currentTime >= startTime + time2 + sleepTime) {

    //setup of radio configuration for relay
    set_relay_config();

    // passo in modalità ricezione
    send_mode = 0;

    Serial.println(F("Passo in ricezione"));
    RXmode_startTime = millis();
    previousMillis = RXmode_startTime;
  }

}

void loop() {

  if (send_mode == -1) {
    // wait for receive info from master and synchronize
    initSync();
  } else if (send_mode == 0) {
    receivePackets();
  } else if (send_mode == 1) {
    checkPreviousPackets();
  } else {
    forwardPackets();
  }

}

void receivePackets() {

  //controllo fin quando sono nel range della tx del master per ricevere i suoi messaggi
  unsigned long currentMillisRX = millis();
  if (currentMillisRX < RXmode_startTime + RX_interval) { // < RXmode_endTime

    // try to parse packet
    LoRa.setSpreadingFactor(SF);
    LoRa.receive(0);
    if ((currentMillisRX - previousMillis ) >= RX_interval / 10 ) {
      previousMillis = currentMillisRX;

      packetSize = LoRa.parsePacket();

      if (packetSize) {   // Received a packet
        //if ( debug > 0 ) {
        Serial.println();
        Serial.print(F("Get Packet: "));
        Serial.print(packetSize);
        Serial.print(F(" Bytes  "));
        Serial.print(F("RSSI: "));
        Serial.print(LoRa.packetRssi());
        //Serial.print(F("  SNR: "));
        //Serial.print(F(LoRa.packetSnr()));
        //Serial.print(F(" dB  FreqErr: "));
        //Serial.println(F(LoRa.packetFrequencyError()));
        Serial.println();

        //}

        // read packet
        int i = 0;
        if ( debug > 0 ) {
          Serial.print(F("Uplink pkt: "));
          Serial.print(F("["));
        }
        while (LoRa.available() && i < 256) {
          packet[i] = LoRa.read();

          if ( debug > 0 )  {
            Serial.print(packet[i], HEX);
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
          sprintf(devaddr, "%x%x%x%x", packet[4], packet[3], packet[2], packet[1]);
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
        // Check if received packet has my device info then not forward it
        if (myDeviceSimilarities == strlen(myDeviceAddress)) {
          /*if (debug > 0){
            Serial.println(F("Pacchetto inviato da me non inoltro"));
            }*/
          //Serial.println(F("NOOP"));

          send_mode = 0;


        } else { //non è inviato da me

          // Increment received packet count
          receivedCount++;
          Serial.print(F("Analizzo  "));
          send_mode = 1;

        }

        return; /* exit the receive loop after received data from the node */
      }

    }
  } else { //finito il tempo di ricezione
    // aggiorno gli intervalli di rx
    // prossimo inizio ricezione
    RXmode_startTime = currentMillisRX + RTT;

    Serial.println(F("Aspetto 10 secondi"));


    // se è stato ricevuto almeno un messaggio
    delay(sleepTime);

    TXmode_startTime = millis();
    send_mode = 2;
    Serial.println(F("Passo in invio"));
    //showFwdBuffer();
    //altrimenti si aspetta per un tx_interval
    //delay(sleepTime+tx_interval+sleepTime);
    //send_mode = 0;
  }
}

void checkPreviousPackets() {

  if (receivedCount == 1) {
    //copia il messaggio sia nel buffer che nel fwdBuffer
    copyMessageintoBuffers();

    // il contatore del fwdBuffer torna sempre a 0 dopo il reset
    send_mode = 0;
  } else {
    // Se il messaggio ricevuto è nel buffer
    // allora torno alla receiver mode send_mode = 0;
    // altrimenti copia il messaggio sia nel buffer che nel fwdBuffer poi send_mode = 0;
    // il contatore del fwdBuffer torna sempre a 0 dopo il reset quindi buffer e fwdBuffer hanno indici diversi

    int equal1 = 0, equal2 = 0, equal3 = 0;

    //showPreviousMessages();

    for (int i = 0; i < rowBuffer; i++) {
      for (int j = 0; j <= dimPreviousMessBuffer; j++) {
        if (prevMessBuffer[i][j] == packet[j]) {
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
      //if (debug > 0) {
      Serial.println(F("Già inoltrato"));
      //}
      if (changeFreq == true || swapRX_TXFreq == true) {
        checkFrequency();
      }

      send_mode = 0;

    } else { // pacchetto non ancora inoltrato e lo aggiungo ai buffers
      /*if (debug > 0) {
        Serial.println(F("Pacchetto diverso dai precedenti"));
        }*/
      Serial.println(F("Diverso"));
      copyMessageintoBuffers();
      send_mode = 0;
    }
  }
  //return;

}

void forwardPackets() {
  //controllo fin quando sono nel range della rx del master per inviare i miei messaggi
  unsigned long currentMillisTX = millis();
  
  if (currentMillisTX < TXmode_startTime + TX_interval) {

    //invio tutti i messaggi nel fwdBuffer
    //se iniziano per /0 salta alla riga successiva
    //leggi fin quando non trovi /0 quindi prendi la dimensione e invia


    if (canSendLoRaWAN == 0) {
      setup_sendLoRaWAN();
      canSendLoRaWAN++;
    }


  } else { //scaduto l'intervallo per inviare

    //reset del fwdBuffer
    //initFwdBuffer();

    showPreviousMessages();
    Serial.println();
    showFwdBuffer();
    // aggiorno gli intervalli di tx
    // prossimo inizio trasmissione
    TXmode_startTime = currentMillisTX + RTT;
    Serial.println(F("aspetto 10 secondi"));
    delay(sleepTime);
    // inoltro tutti i pachetti nel buffer

    // aggiorno il tempo di inizio ricezione
    RXmode_startTime = millis();
    Serial.println(F("Passo in ricezione"));
    //setup of radio configuration for relay
    set_relay_config();
    canSendLoRaWAN = 0;
    send_mode = 0;
  }
}

void initPreviousMessagesBuffer() {
  for (int i = 0; i < rowBuffer; i++) {
    for (int j = 0; j <= dimPreviousMessBuffer; j++) {
      prevMessBuffer[i][j] = '0';
    }
  }
}

void initFwdBuffer() {
  index_fwdBuffer = 0;
  for (int i = 0; i < rowBuffer; i++) {
    for (int j = 0; j <= dimFwdBuffer; j++) {
      fwdBuffer[i][j] = '/0';
    }
  }
  Serial.println(F("reset fwdBuffer"));
}

// Clone previous received message into buffer after transmission complete
void copyMessageintoBuffers() {
  int i = 0, j = 0;

  // Copy received message into previous Messages Buffer
  while (i <= dimPreviousMessBuffer) {
    prevMessBuffer[receivedCount % rowBuffer][i] = packet[i];
    i++;
  }

  // Copy received message into forward Buffer
  while (j <= packetSize) {
    fwdBuffer[index_fwdBuffer % rowBuffer][j] = packet[j];
    j++;
  }
  index_fwdBuffer++;


}


// Print previous messages into buffer
void showPreviousMessages() {
  Serial.println(F("Buffer: "));
  for (int i = 0; i < rowBuffer; i++) {
    Serial.print(F("["));
    for (int j = 0; j <= dimPreviousMessBuffer; j++) {
      Serial.print(prevMessBuffer[i][j], HEX);
      Serial.print(F(" "));
    }
    Serial.println(F("]"));
  }
}

// Print fwdBuffer
void showFwdBuffer() {

  Serial.println(F("FWD Buffer: "));
  for (int i = 0; i < rowBuffer; i++) {
    Serial.print(F("["));
    for (int j = 0; j <= dimFwdBuffer; j++) {
      Serial.print(fwdBuffer[i][j], HEX);
      Serial.print(F(" "));
    }
    Serial.println(F("]"));
  }
}
