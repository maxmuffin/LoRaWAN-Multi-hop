
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

unsigned long startTime;
unsigned long currentTime;
const long interval = 1000; // send pkt every 1 second

const int debug = 1;

long randNumber;
int counter = 0;

const byte freqArraySize = 2;
//array of frequencies valid for the application to change
long int frequencies[freqArraySize] = {433175000, 433375000};

// LoRaWAN NwkSKey, network session key
static const PROGMEM u1_t NWKSKEY[16] = { 0x6D, 0x5F, 0x0F, 0xD1, 0xA6, 0x1F, 0xAD, 0xC3, 0xEC, 0x73, 0xB1, 0xC8, 0x42, 0xB5, 0x9E, 0xD1 };

// LoRaWAN AppSKey, application session key
static const u1_t PROGMEM APPSKEY[16] = { 0x34, 0xDC, 0x88, 0xCB, 0x1B, 0x0B, 0xE1, 0x27, 0xD6, 0xD2, 0x63, 0xD9, 0x92, 0x3C, 0x49, 0x40 };

// LoRaWAN end-device address (DevAddr)
static const u4_t DEVADDR = 0x26011032; // device 1

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
  if (debug > 0) {
    //Serial.print(os_getTime());
    //Serial.print(": ");
    switch (ev) {
      case EV_TXCOMPLETE:
        //Serial.println(F("EV_TXCOMPLETE"));
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
    if (debug > 0) {
      //Serial.println(F("OP_TXRXPEND"));
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
}


void setup() {
  Serial.begin(9600);

  randomSeed(analogRead(0));
  startTime = millis();

}

void loop() {
  currentTime = millis();

  if (currentTime - startTime > interval)
  {
    startTime = currentTime;
    setup_sendLoRaWAN();
  }else{
    // DO NOTHING
  }

}
