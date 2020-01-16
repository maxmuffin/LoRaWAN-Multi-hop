#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <DHT.h>
#include <DHT_U.h>
#include <LoRa.h>

// 0 for slave 1 for master
int initConf = 1;
// LoRaWAN end-device address (DevAddr)
static const u4_t DEVADDR = 0x26011032; // device 1
//static const u4_t DEVADDR = 0x0067295E; // device 2

// if deviceAddress starts with 2 zero, remove the first one
// or remove thefirst zero, lower letter
char myDeviceAddress [8] = "2611032\0";  // device 1
//char myDeviceAddress [8] = "067295e\0"; // device 2
//Set Debug = 1 to enable Output;
// -1 to debug synchronization
const int debug = 0;

int synched = 0;
int SyncInterval = 10000;

int pktSendDelay = 300; // 300ms, 500ms, 750ms, 1000ms, 1500ms, 2000ms
int delayForSendLW = 500;
int sleepTimeTest = 5000; //1000, 5000, 10000

// received rxOpen and rxClose of master, correspond of txOpen and txClose of slave

unsigned long currentTime, previousMillis, startTime;
unsigned long RXmode_startTime, RXmode_endTime, TXmode_startTime, TXmode_endTime;
unsigned long currentMillisRX;

int sleepTime, RTT, TX_interval, RX_interval;
long lastSendTime = 0;
int send_mode = -1;
int initOnStartup = 0;
int canSendLoRaWAN = 0;

int counter = 0;
long randNumber;

//sync header
byte idByte = 0xFF;

//********************************* RELAY
const byte freqArraySize = 2;
//array of frequencies valid for the application to change
long int frequencies[freqArraySize] = {433175000, 433375000};


/* ******* OPERATING MODE *******
    Mode -1 = wait for synch
    Mode  0 = receive packet
    Mode  1 = check if received packet is similar to previous send packet
    Mode  2 = send/forward received packet
    Mode 4 = sleep mode
*********** */



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
    randNumber = random(9999);

    if (randNumber < 10) {
      randNumber = randNumber * 1000;
    } else if (randNumber < 100) {
      randNumber = randNumber * 100;
    } else if (randNumber < 1000) {
      randNumber = randNumber * 10;
    }
    char payload[4];
    dtostrf(randNumber, 4, 0, payload);

    LMIC_setTxData2(1, (uint8_t*)payload, sizeof(payload), 0);
  }

  if (debug == 0) {
    counter++;
    Serial.print(counter);
    Serial.print(',');
    Serial.print(int(LMIC.dataLen));
    Serial.print(',');
    if (LMIC.dataLen) {
      // data received in rx slot after tx
      for (int i = 0; i < LMIC.dataLen; i++) {
        if (LMIC.frame[LMIC.dataBeg + i] < 0x10) {
          Serial.print(F("0"));

        }
        Serial.print(LMIC.frame[LMIC.dataBeg + i], HEX);
      }

    }
    Serial.print('\n');
  }


  if ( debug > 0 ) {
    Serial.println(F("Send pkt"));
  }
  //Serial.print(F("Send on freq: "));
  //Serial.println(LMIC.freq);

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
  //LMIC_shutdown();

  // need another reset?
  //return;
}

void setup() {
  Serial.begin(9600);

  randomSeed(analogRead(0));

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

  // initialize ratio at 433 MHz
  if (!LoRa.begin(433E6)) {
    if (debug > 0) Serial.println(F("LoRa init failed"));
    while (true);
  }
  if ( debug > 0) Serial.println(F("LoRa init succeeded"));

}


void initSync() {

  //SLAVE
  if (initConf == 0) {

    if (synched == 0) {
      syncWithMaster();
    } else {
      if (debug < 0) {
        Serial.println(F("Synched"));
      }

      // aspetta fino alla txMode del master e imposta la receiver mode
      currentTime = millis();

      // da quando ho inviato l'ack
      if (currentTime >= startTime + RTT + 300) {


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

        // passo in modalità invio
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

/*
  ########## SYNCHR0NIZATION ##########
*/

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

    if (debug < 0) Serial.println(F("sync send"));

    lastSendTime = millis();
    LoRa.onReceive(onReceiveSyncforMaster);
    LoRa.receive();
  }
}

// used from master read packet and if first byte is the syncByte than start
void onReceiveSyncforMaster(int pSize) {
  if (pSize == 0) {
    return;
  }
  byte idByteRcv = LoRa.read();
  // non è un messaggio di sync
  if (idByteRcv != idByte) {
    return;
  }

  startTime = millis();
  if (debug < 0) Serial.println(F("Ack rcv"));
  synched = 1;
  // ho ricevuto l'ack e passo in modalità invio
}



void loop() {

  if (send_mode == -1) {
    // wait for synchronization
    initSync();
  } else if (send_mode == 0) {
    receivePackets();
  } else if (send_mode == 2) {
    forwardPackets();
  } else {
    // nothing
  }

}

// receive packets, mon fa nulla
void receivePackets() {
  // da testare
  LoRa.sleep();

  //controllo fin quando sono nel range della tx del master per ricevere i suoi messaggi
  currentMillisRX = millis();

  if (currentMillisRX < RXmode_startTime + RX_interval) { // < RXmode_endTime


  } else { //finito il tempo di ricezione
    // aggiorno gli intervalli di rx
    // prossimo inizio ricezione
    RXmode_startTime = currentMillisRX + RTT;
    if (debug < 0) {
      Serial.println(F("sleepMode"));
    }

    send_mode = 4;
    delay(sleepTimeTest);

    TXmode_startTime = millis();

    send_mode = 2;
    if (debug < 0) {
      Serial.println(F("swap to send"));
    }
  }
}


// send his LoRaWAN packets
void forwardPackets() {
  // da testare
  LoRa.sleep();
  /*if (initOnStartup == 0){
    setupLoRaWAN();
    initOnStartup++;
    }*/

  //controllo fin quando sono nel range della rx del master per inviare i miei messaggi
  unsigned long currentMillisTX = millis();

  if (currentMillisTX < TXmode_startTime + TX_interval) {

    if (currentMillisTX < TXmode_startTime + 2000) {
      // aspetto 2 secondi prima di inviare
    } else {

      // used for repeat only one time
      if (canSendLoRaWAN == 0) {
        //for test send 2 times
        for (int k = 0; k < 3; k++) {
          delay(pktSendDelay); //inizialmente era 1000
          // take DHT values and send LoraWAN pkt
          setup_sendLoRaWAN();
          //Serial.println(F("s"));
        }
        canSendLoRaWAN++;
      }

    }
  } else { //scaduto l'intervallo per inviare

    // aggiorno gli intervalli di tx
    // prossimo inizio trasmissione
    TXmode_startTime = currentMillisTX + RTT;
    if (debug < 0) {
      Serial.println(F("sleepMode"));
    }
    //Aspetto per lo sleep
    send_mode = 4;
    delay(sleepTimeTest);

    // aggiorno il tempo di inizio ricezione
    RXmode_startTime = millis();
    if (debug < 0) {
      Serial.println(F("swap to rcv"));
    }

    canSendLoRaWAN = 0;
    //Passo in receive mode
    //LoRa.sleep();
    send_mode = 0;
    return;
  }
}
