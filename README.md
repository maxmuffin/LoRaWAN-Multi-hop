# LoRaWAN-Multi-hop
multi-hop implementation of LoRaWAN using single-channel gateway

This project concerns an implementation of LoRaWAN node for Multi-Hop developed for my Master's Degree in Computer Science @UniBo  
The repo contains:
### Nodes:
This folder contains different kind of nodes:
- **End Nodes**: contains normal version of LoRaWAN end-node using ABP or OTA autentications and using or not DHT11 for take temperature and humidity values.
- **Relay Nodes**: contains implementation of relay-node that forward received LoRaWAN packets.  It is developed in two versions:  
  - One using buffer for checking if received packet was already forwarded.  
  - One not using buffer and check if previous forwarded packet is the same that is received.
- **Full Nodes**: contains the core for Multi-Hopping in LoRaWAN. It combines LoRa and LoRaWAN stacks for receive and forward received packets and after specific interval of time send his LoRaWAN packet like an end-node.  
Contains different variant of full-node:
  - __fullNode_simple__: this is the simplest implementation that have 4 operation mode. It starts in receiver mode (mode 0) and after receiving a package it checks that it has not already been sent (mode 1). If it is present in the buffer it will not forward it and it will return to receiver mode otherwise it will forward it (mode 2). After a specific time interval the node goes into end-node mode to send its LoRaWAN packet.
  - __fullNode_noForwardMyPackets__: this is an optimization of fullNode_simple that after receive a packet analize the header for check if the sender Device Address is the same as yours. If yes than rest in receiver mode otherwise switch to mode 1 for checking if message is already forwarded.
  - __fullNode_noForwardMyPackets_DHT__: this version send LoRaWAN message including DHT11 data (temperature, humidity).
  - __fullNode_noForwardMyPackets_LED__: this version uses RGB led for easly understand the state of full-node without using debugging.
  
For the hardware have been used Arduino UNO and Dragino Shield v1.4

### Gateway:
- **single_chan_pkt_fwd**: an implementation of a Single Channel LoRaWAN Gateway mainteined by Thomas Telkamp thomas@telkamp.eu.  
This folder was forked by @jlesech https://github.com/tftelkamp/single_chan_pkt_fwd to add json configuration file
then forked by @hallard https://github.com/hallard/single_chan_pkt_fwd  
Modified for use **433MHz** frequency. Run using: 
```console For run
 sudo ./single_chan_pkt_fwd
```
- **dual_channel_pkt_fwd**: an implementation of a Dual Channel LoRaWAN Gateway mainteined by Thomas Telkamp thomas@telkamp.eu
Was forked by @jlesech https://github.com/tftelkamp/single_chan_pkt_fwd to add json configuration file
then forked by @hallard https://github.com/hallard/single_chan_pkt_fwd then forked by @bokse001 https://github.com/bokse001/dual_chan_pkt_fwd to add dual channel support, configurable network interface and uputronics Raspberry Pi+ LoRa(TM) Expansion Board.  
Modified for use **433.175MHz** and **433375MHz**  frequencies. Run using: 
```console For run
 sudo ./dual_chan_pkt_fwd
```

### Utils:
Contains:
- used libraries: **Arduino LoRa** https://github.com/sandeepmistry/arduino-LoRa  and **LMIC** https://github.com/matthijskooijman/arduino-lmic edited for **433MHz**.
- wirings folder that contains fritzing schema for connect Futura Elettronica LoRa Shield to Raspberry 3B+ used for gateway.
- sh scripts for easly start, stop and see status of **LoRaServer**. https://www.chirpstack.io/  
See this Link for installation: https://www.chirpstack.io/guides/debian-ubuntu/
