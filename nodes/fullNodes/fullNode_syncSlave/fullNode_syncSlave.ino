// received rxOpen and rxClose of master, correspond of txOpen and txClose of slave
unsigned long time1, time2;
int sleepTime, RTT, TX_interval, RX_interval;
unsigned long currentTime;
unsigned long previousMillis;
unsigned long startTime = millis();
unsigned long RXmode_startTime, RXmode_endTime, TXmode_startTime, TXmode_endTime;

int send_mode = -1;
int receivedCount = 0;

void setup() {
  Serial.begin(9600);
}

void initSync() {
  

  // Ricevi il messaggio lora e splittalo

  time1 = startTime+10000UL; //tra 10 secondi
  time2 = startTime+15000UL;
  sleepTime = 10*1000; //30 secondi sleep

  TX_interval = time2 - time1;
  RX_interval = TX_interval;

  RTT = (TX_interval + sleepTime) * 2;

  // aspetta fino alla txMode del master e imposta la receiver mode
  currentTime = millis();
    Serial.print("Sincronizzo   ");
    Serial.print(currentTime);
    Serial.print("    ");
    Serial.println(startTime + time2 + sleepTime);

  if (currentTime >= startTime + time2 + sleepTime) {


    // passo in modalità ricezione
    send_mode = 0;
    Serial.println("Passo in ricezione");
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
    //LoRa.setSpreadingFactor(SF);
    //LoRa.receive(0);
    if ((currentMillisRX - previousMillis ) >= RX_interval / 10 ) {
      previousMillis = currentMillisRX;

      // LEGGI IL PACCHETTO
      // CONTROLLA DEVADDR
      // SE è DIVERSO ALLORA
          receivedCount++;
          send_mode = 1;
          Serial.println("Analizzo");
      // SE è UGUALE ALLORA send_mode = 0
    }
  } else { //finito il tempo di ricezione
    // aggiorno gli intervalli di rx
    // prossimo inizio ricezione
    RXmode_startTime = currentMillisRX + RTT;

    Serial.println("Aspetto 10 secondi");
    // se è stato ricevuto almeno un messaggio 
        delay(sleepTime);

        TXmode_startTime = millis();
        send_mode = 2;
        Serial.println("Passo in invio");
    //altrimenti si aspetta per un tx_interval
        //delay(sleepTime+tx_interval+sleepTime);
        //send_mode = 0;
  }
}

void checkPreviousPackets() {
  if (receivedCount == 1) {
    Serial.println("ok");
    //copia il messaggio sia nel buffer che nel fwdBuffer
    // il contatore del fwdBuffer torna sempre a 0 dopo il reset
    send_mode = 0;
  } else {
    Serial.println("ok");
    // Se il messaggio ricevuto è nel buffer
          // allora torno alla receiver mode send_mode = 0;
    // altrimenti copia il messaggio sia nel buffer che nel fwdBuffer poi send_mode = 0;
    // il contatore del fwdBuffer torna sempre a 0 dopo il reset quindi buffer e fwdBuffer hanno indici diversi
    send_mode = 0;
  }
  //return;
}

void forwardPackets() {
  //controllo fin quando sono nel range della rx del master per inviare i miei messaggi
  unsigned long currentMillisTX = millis();
  if (currentMillisTX < TXmode_startTime + TX_interval) {

    //invio tutti i messaggi nel fwdBuffer
        //se iniziano per * salta alla riga successiva
        //leggi fin quando non trovi * quindi prendi la dimensione e invia

    //invio il mio messaggio LoRaWAN
    //reset del fwdBuffer con tutti: *,*,*,*,*,*, ecc
    Serial.println("Invio");
    
  }else{//scaduto l'intervallo per inviare
    // aggiorno gli intervalli di tx
    // prossimo inizio trasmissione
    TXmode_startTime = currentMillisTX + RTT;
    Serial.println("aspetto 10 secondi");
    delay(sleepTime);
    // inoltro tutti i pachetti nel buffer

    // aggiorno il tempo di inizio ricezione
    RXmode_startTime = millis();
    Serial.println("Passo in ricezione");
    send_mode = 0;
  }
}
