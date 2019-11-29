#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <DHT.h>
#include <DHT_U.h>
#include <LoRa.h>

// 0 for slave 1 for master
int initConf = 0;
// LoRaWAN end-device address (DevAddr)
//static const u4_t DEVADDR = 0x26011032; // device 1
static const u4_t DEVADDR = 0x0067295E; // device 2

// if deviceAddress starts with 2 zero, remove the first one
// or remove thefirst zero, lower letter
//char myDeviceAddress [8] = "2611032\0";  // device 1
char myDeviceAddress [8] = "067295e\0"; // device 2
//Set Debug = 1 to enable Output;
// -1 to debug synchronization
const int debug = -1;
int initRcv = 0;


int synched = 0;
int SyncInterval = 10000;
// received rxOpen and rxClose of master, correspond of txOpen and txClose of slave

unsigned long currentTime, previousMillis, startTime;

unsigned long RXmode_startTime, RXmode_endTime, TXmode_startTime, TXmode_endTime;

unsigned long currentMillisRX;

int sleepTime, RTT, TX_interval, RX_interval;
long lastSendTime = 0;
int send_mode = -1;
int receivedCount;
int canForward = 0;
int canSendLoRaWAN = 0;

//sync header
byte idByte = 0xFF;

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

// Packets
static int packetSize;
static uint8_t packet[50]; // current received packet
static uint8_t message[50]; //used for forward messages into fwdBuffer

// Buffers
const byte rowBuffer = 3;
const byte dimPreviousMessBuffer = 15;
const byte dimFwdBuffer = 22;
int index_fwdBuffer;

// used for keep dimensions of received packets
int dimRcvMess[rowBuffer];
int indexRCV, tmp;

// buffer for previous received and send message
int prevMessBuffer[rowBuffer][dimPreviousMessBuffer];
int fwdBuffer[rowBuffer][dimFwdBuffer];

//********************************* LORAWAN

// LoRaWAN NwkSKey, network session key
static const PROGMEM u1_t NWKSKEY[16] = { 0x6D, 0x5F, 0x0F, 0xD1, 0xA6, 0x1F, 0xAD, 0xC3, 0xEC, 0x73, 0xB1, 0xC8, 0x42, 0xB5, 0x9E, 0xD1 };

// LoRaWAN AppSKey, application session key
static const u1_t PROGMEM APPSKEY[16] = { 0x34, 0xDC, 0x88, 0xCB, 0x1B, 0x0B, 0xE1, 0x27, 0xD6, 0xD2, 0x63, 0xD9, 0x92, 0x3C, 0x49, 0x40 };

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

#define DHT_PIN 3
#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);

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
    if (initConf == 0) {
      byte payload[2]; //on device 1 send payload[4], on device 2 payload [2]
      payload[0] = highByte(random(1, 9));
      payload[1] = lowByte(random(1, 9));

      LMIC_setTxData2(1, (uint8_t*)payload, sizeof(payload), 0);

    } else { //take values from DHT and send it
      float h = dht.readHumidity();
      float t = dht.readTemperature();

      // encode float as int
      int16_t tempInt = round(t * 100);
      int16_t humInt = round(h * 100);

      byte payload[4];
      /*
        payload[0] = highByte(tempInt);
        payload[1] = lowByte(tempInt);
        payload[2] = highByte(humInt);
        payload[3] = lowByte(humInt);*/
      payload[0] = highByte(random(1, 9));
      payload[1] = lowByte(random(1, 9));
      payload[2] = highByte(random(1, 9));
      payload[3] = lowByte(random(1, 9));
      // need to add random values?
      LMIC_setTxData2(1, (uint8_t*)payload, sizeof(payload), 0);
    }

    if ( debug > 0 ) {
      Serial.println(F("Send pkt"));
    }
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

  //initialize buffers
  initPreviousMessagesBuffer();
  initFwdBuffer();

  // if i'm master set tx and rx interval
  if (initConf == 1 ) {
    TX_interval = 20000;
    TXmode_startTime = 20000;
    TXmode_endTime = TXmode_startTime + TX_interval;

    RXmode_endTime = TXmode_endTime;
    RXmode_startTime = TXmode_startTime;

    RX_interval =  TX_interval;

    sleepTime = 10000; //10 secondi sleep
    RTT = (TX_interval + sleepTime) * 2;
  }
  receivedCount = 0;
  // initialize ratio at 433 MHz
  if (!LoRa.begin(433E6)) {
    if (debug > 0) Serial.println(F("LoRa init failed"));
    while (true);
  }
  if ( debug > 0) Serial.println(F("LoRa init succeeded"));

}

// Set relay's radio configurations
void set_relay_config() {
  //Get Radio configure
  getRadioConf();

  if (!LoRa.begin(freq))
    if ( debug > 0 ) Serial.println(F("init LoRa failed"));
  setLoRaRadio();// Set LoRa Radio to Semtech Chip

  // test it
  //delay(200);
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

  //SLAVE
  if (initConf == 0) {

    if (synched == 0) {
      //Serial.println(F("Slave"));
      syncWithMaster();
    } else {
      if (debug < 0) {
        Serial.println(F("Synched"));
      }

      // aspetta fino alla txMode del master e imposta la receiver mode
      currentTime = millis();

      // da quando ho inviato l'ack
      if (currentTime >= startTime + RTT + 300) {                         //Da vedere bene!!
        //setup of radio configuration for relay
        set_relay_config();

        // passo in modalità ricezione
        send_mode = 0;
        if (debug < 0) {
          Serial.println(F("swap in rcv"));
        }

        RXmode_startTime = millis();
        previousMillis = RXmode_startTime;
      }

    }
  } else { //This is for Master
    //Serial.println(F("Master"));
    if (synched == 0) {
      syncWithSlave();
    } else {
      if (debug < 0) {
        Serial.println(F("Synched"));
      }
      // aspetta fino alla rxMode dello slave e poi invia
      currentTime = millis();
      // da quando ho ricevuto l'ack
      if (currentTime >= startTime + RTT) {
        //setup of radio configuration for relay
        set_relay_config();
        // passo in modalità invio
        //delay(300);
        send_mode = 2;
        if (debug < 0) {
          Serial.println(F("swap in send"));
        }

        TXmode_startTime = millis();
        previousMillis = TXmode_startTime;
      }

    }
  }
}

void loop() {

  if (send_mode == -1) {
    // wait for synchronization
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
  currentMillisRX = millis();


  if (currentMillisRX < RXmode_startTime + RX_interval) { // < RXmode_endTime
    // try to parse packet
    //if (currentMillisRX - previousMillis >= 500 ) {
    //previousMillis = currentMillisRX;
    //Serial.println("crist");

    capturePackets();
    //}
  } else { //finito il tempo di ricezione
    // aggiorno gli intervalli di rx
    // prossimo inizio ricezione
    RXmode_startTime = currentMillisRX + RTT;
    if (debug < 0) {
      Serial.println(F("wait 10s"));
    }
    // se è stato ricevuto almeno un messaggio
    delay(sleepTime); //cambiare modo in sleep (send_mode = 4) così non ascolta onReceive

    TXmode_startTime = millis();

    //LoRa.end();
    initRcv = 1;
    //memset(packet, 0, sizeof packet);
    //packetSize = 0;
    send_mode = 2;
    if (debug < 0) {
      Serial.println(F("swap to send"));
    }
    //showFwdBuffer();
  }
}

// controllo il pacchetto ricevuto con quelli presenti nel PrevMessBuffer
// se è il primo pacchetto ricevuto non lo controllo e lo inserisco nei buffers
void checkPreviousPackets() {

  tmp = packetSize;

  if (tmp == 0) {

    Serial.println(F("null packet"));
    LoRa.sleep();
    send_mode = 0;
    return;
  }

  if (receivedCount == 1) {

    
    //dimRcvMess[indexRCV] = tmp;
    //indexRCV++;

    //copia il messaggio sia nel buffer che nel fwdBuffer
    copyMessageintoBuffers();


    //currentMillisRX = millis();

    // il contatore del fwdBuffer torna sempre a 0 dopo il reset
    LoRa.sleep();
    send_mode = 0;
    return;
  } else {
    // se il messaggio ricevuto è nel buffer
    //  allora torno alla receiver mode send_mode = 0;
    // altrimenti copia il messaggio sia nel buffer che nel fwdBuffer poi send_mode = 0;
    // il contatore del fwdBuffer torna sempre a 0 dopo il reset quindi buffer e fwdBuffer hanno indici diversi

    int equal1 = 0, equal2 = 0, equal3 = 0;

    //showPreviousMessages();

    //currentMillisRX = millis();


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
    if (equal1 >= 14 || equal2 >= 14 || equal3 >= 14) {
      if (debug > 0 || debug < 0) {
        Serial.println(F("Già inoltrato"));
      }
      if (changeFreq == true || swapRX_TXFreq == true) {
        checkFrequency();
      }

      //memset(packet, 0, sizeof packet);
      //packetSize = 0;
      LoRa.sleep();
      send_mode = 0;
      return;

    } else { // pacchetto non ancora inoltrato e lo aggiungo ai buffers
      if (debug > 0 || debug < 0) {
        Serial.println(F("diverso"));
      }

      //dimRcvMess[indexRCV] = tmp;
      //indexRCV++;

      copyMessageintoBuffers();

      
      //packetSize = 0; //da controllare SONO TUTTI RIMOSSI E COMMENTATI
      LoRa.sleep();
      send_mode = 0;

      return;
    }
  }
  return;

}

// forward all packets in the fwdBuffer when is time to fwd and send his LoRaWAN packets
void forwardPackets() {

  //controllo fin quando sono nel range della rx del master per inviare i miei messaggi
  unsigned long currentMillisTX = millis();
  /*if (receivedCount != 0) { //Per il primo forward del master
    delay(2000);
    }*/

  if (currentMillisTX < TXmode_startTime + TX_interval) {

    if (currentMillisTX < TXmode_startTime + 2000) {
      //Serial.println("Aspetto 2 secondi");
    } else {
      //invio tutti i messaggi nel fwdBuffer
      //se iniziano per /0 salta alla riga successiva
      //leggi fin quando non trovi /0 quindi prendi la dimensione e invia

      if (canForward == 0) { //deve avvenire una sola volta, dopo il primo inoltro di tutti i pkt nel buffer imposta canForward a 1
        //LoRa.end();

        //showFwdBuffer();

        for (int i = 0; i < rowBuffer; i++) {

          //int dim = 0;
          bool jump = false; //used for jump row if is empty
          if (dimRcvMess[i] == 0){
            jump = true;
          }

          for (int j = 0; j <= dimFwdBuffer; j++) {
            if (fwdBuffer[i][0] == '/0' && fwdBuffer[i][1] == '/0' && fwdBuffer[i][2] == '/0') {
              jump = true;
            } else if (fwdBuffer[i][j] != '/0') {
              //copy fwdBuffer row into message
              message[j] = fwdBuffer[i][j];
              //Serial.print(message[j]);
              //Serial.print(F(" "));
            }


          }
          //Serial.println("");

          if (jump == false) {

            // Forward using LoRa
            LoRa.setFrequency(txfreq);
            //LoRa.setSpreadingFactor(txfreq);

            delay(1000);

            if (dimRcvMess[i] > 0 ){

              LoRa.beginPacket();
              LoRa.write(message, dimRcvMess[i]);
              LoRa.endPacket();
              if (debug > 0 || debug < 0) {
              Serial.print(F("FWD pkt "));
              Serial.println(dimRcvMess[i]);
            }
            }else{
              Serial.println(F("not send 0 pkt"));
              //showFwdBuffer();
              //showDimRcvMess();
            }

            //memset(message, 0, sizeof message);




            if (changeFreq == true || swapRX_TXFreq == true) {
              checkFrequency();
            }
            
          } else {
            delay(1000);
            if (debug > 0 || debug < 0) {
              Serial.println(F("Jump"));
            }
          }
        }
        canForward++;
      }


      // used for repeat only one time
      if (canSendLoRaWAN == 0) {
        // radio in standby
        //LoRa.end();
        delay(1000);

        //LoRa.end();
        //for test send 3 times
        for (int k = 0; k < 2; k++) {
          delay(1000);
          setup_sendLoRaWAN();
          Serial.println(F("s"));
        }
        canSendLoRaWAN++;
      }

    }
  } else { //scaduto l'intervallo per inviare

    //reset del fwdBuffer
    initFwdBuffer();

    //showPreviousMessages();
    //Serial.println();
    //showFwdBuffer();
    // aggiorno gli intervalli di tx
    // prossimo inizio trasmissione
    TXmode_startTime = currentMillisTX + RTT;
    if (debug < 0) {
      Serial.println(F("wait 10s"));
    }
    //Aspetto per lo sleep
    delay(sleepTime); //cambiare modo in sleep (send_mode = 4)

    // aggiorno il tempo di inizio ricezione
    RXmode_startTime = millis();
    if (debug < 0) {
      Serial.println(F("swap to rcv"));
    }
    //setup of radio configuration for relay
    set_relay_config();
    initRcv = 0;
    canForward = 0;
    canSendLoRaWAN = 0;
    //Passo in receive mode
    LoRa.sleep();
    send_mode = 0;
    return;
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
  indexRCV = 0;
  for (int i = 0; i < rowBuffer; i++) {
    for (int j = 0; j <= dimFwdBuffer; j++) {
      fwdBuffer[i][j] = '/0';
    }
    dimRcvMess[i] = 0;
  }

  if (debug > 0) {
    Serial.println(F("reset fwdB"));
  }
}

// Clone previous received message into buffer after transmission complete
void copyMessageintoBuffers() {
  int i = 0, j = 0;

  // Copy received message into previous Messages Buffer
  while (i <= dimPreviousMessBuffer) {
    prevMessBuffer[receivedCount % rowBuffer][i] = packet[i];
    i++;
  }

  if (index_fwdBuffer < rowBuffer && packetSize > 0) { //da controllare bene, l'index non deve sforare il fwdBuffer
    // Copy received message into forward Buffer
    while (j <= packetSize) {
      fwdBuffer[index_fwdBuffer][j] = packet[j]; //modificato anche qui prima era index_fwdBuffer%rowBuffer
      j++;
    }

    // copio la dimensione neln'array delle dimensioni
    dimRcvMess[index_fwdBuffer] = j-1;

    //memset(packet, 0, sizeof packet);
    //packetSize = 0;
    index_fwdBuffer++;
  }
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

void showDimRcvMess(){
  for (int i = 0; i< rowBuffer; i++){
    Serial.println(dimRcvMess[i]);
  }
}

void capturePackets() {
  if (send_mode == 0) {
    LoRa.setSpreadingFactor(SF);
    //if (initRcv == 0) {
    LoRa.onReceive(listenOnRF);
    //}
    LoRa.receive();
  }
}

void listenOnRF(int pSize) {

  if (send_mode == 2 ) {
    Serial.println(F("stop"));
    return;
  } else {
    //Serial.println(F("opplà"));

    //packetSize = LoRa.parsePacket();  //CONTROLLARE QUI CHE VENGANO AGGIORNATI I PACCHETTI
    // PROBLEMA QUANDO SI RICEVE UN MY PACKET
    // LEGGE SOLO IL PRIMO, I SUCCESSIVI SONO UGUALI
    pSize = LoRa.parsePacket();
    //packetSize = pSize;

    //memset(packet, 0, sizeof(packet));

    //Serial.print(F("a "));
    //Serial.print(pSize);
    //Serial.print(F("  "));
    //Serial.println(packetSize);
    //if (pSize>0) {   // Received a packet

    if ( debug > 0 ) {
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
    }

    // read packet
    int i = 0;
    if ( debug < 0 ) {
      Serial.print(F("Uplink pkt: "));
      Serial.print(F("["));
    }
    while (LoRa.available() && i < 256) {
      packet[i] = LoRa.read();

      if ( debug < 0 )  {
        Serial.print(packet[i], HEX);
        Serial.print(F(" "));
      }

      i++;
    }
    if ( debug < 0 ) {
      Serial.print(F("]"));
      Serial.println("");
    }
    packetSize = i;


    char devaddr[12] = {'\0'};
    // take devide address of received packets
    sprintf(devaddr, "%x%x%x%x", packet[4], packet[3], packet[2], packet[1]);
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
      if (debug > 0 || debug < 0) {
        Serial.println(F("my packet"));
      }

      //memset(packet, 0, sizeof packet);
      //memset(devaddr, 0, sizeof devaddr);
      //packetSize = 0;
      LoRa.sleep();
      send_mode = 0;
      return;


    } else { //non è inviato da me

      // Increment received packet count
      receivedCount++;
      if (debug < 0) {
        Serial.println(F("Analize"));
      }
      send_mode = 1;
      return;

    }
  }

  //return; /* exit the receive loop after received data from the node */
  //}

}

// used for slave, wait for receive sincronization message
void syncWithMaster() {
  LoRa.onReceive(onReceiveSyncforSlave);
  LoRa.receive();
}

// used for slave, read received packets. if is a syncronization message get values
void onReceiveSyncforSlave(int pSize) {
  if (pSize == 0) return;

  byte idByteRcv = LoRa.read();
  int dimPacket = LoRa.read();
  String payload = "";

  while ( LoRa.available()) {
    payload += (char)LoRa.read();
  }

  // if the recipient isn't this device or broadcast,
  if (idByteRcv != idByte) {
    if (debug > 0) {
      Serial.println(F("This message is not for me"));
    }
    return;
  }

  // controllo se la dimensione del messaggio è uguale a quella contenuta nel pacchetto
  if (dimPacket != payload.length()) {
    if (debug > 0) {
      Serial.println(F("message length doesn't match rcv length"));
    }
    return;
  }

  int ind1 = payload.indexOf(',');
  RXmode_startTime = payload.substring(0, ind1).toInt();
  int ind2 = payload.indexOf(',', ind1 + 1 );
  RXmode_endTime = payload.substring(ind1 + 1, ind2 + 1).toInt();
  int ind3 = payload.indexOf(',', ind2 + 1 );
  sleepTime = payload.substring(ind2 + 1).toInt();
  /*
    Serial.print("Received Message: ");
    Serial.print(idByteRcv, HEX);
    Serial.print(" ");
    Serial.print(RXmode_startTime);
    Serial.print(" ");
    Serial.print(RXmode_endTime);
    Serial.print(" ");
    Serial.print(sleepTime);
  */
  TXmode_endTime = RXmode_endTime;
  TXmode_startTime = RXmode_startTime;

  RX_interval = RXmode_endTime - RXmode_startTime;
  TX_interval =  RX_interval;

  RTT = (RX_interval + sleepTime) * 2;

  startTime = millis();

  // Reply with ACK
  LoRa.beginPacket();
  LoRa.write(idByte);
  LoRa.endPacket();

  if (debug < 0) Serial.println(F("Ack"));
  synched = 1;

  // da questo momento aspetto un tempo sleep e mi metto in ascolto

}

// used from master send message with rx/tx info and wait ack for start
void syncWithSlave() {

  if (millis() - lastSendTime > SyncInterval) {

    String payload = "";

    payload.concat(TXmode_startTime);
    payload.concat(',');
    payload.concat(TXmode_endTime);
    payload.concat(',');
    payload.concat(sleepTime);

    LoRa.beginPacket();
    LoRa.write(idByte);
    LoRa.write(payload.length());
    LoRa.print(payload);
    LoRa.endPacket();
    /*
      Serial.print("Messaggio inviato: ");
      Serial.print(idByte, HEX);
      Serial.print(" ");
      Serial.print(payload.length());
      Serial.print(" ");
      Serial.println(payload);
    */
    if (debug < 0) Serial.println(F("sync send"));

    lastSendTime = millis();
    LoRa.onReceive(onReceiveSyncforMaster);
    LoRa.receive();
  }
}


void onReceiveSyncforMaster(int pSize) {
  if (pSize == 0) {
    return;
  }
  byte idByteRcv = LoRa.read();
  // non è un messaggio di sync
  if (idByteRcv != idByte) {
    return;
  }
  /*
    Serial.print("Received ack: ");
    Serial.println(idByteRcv, HEX);*/

  startTime = millis();
  if (debug < 0) Serial.println(F("Ack rcv"));
  synched = 1;
  // ho ricevuto l'ack e passo in modalità invio
}
