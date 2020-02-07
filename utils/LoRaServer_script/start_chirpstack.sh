#!/bin/bash

BGreen='\033[1;32m'
BYellow='\033[1;33m'
NC='\033[0m'

####### start Gateway Bridge #######
echo "${BYellow}Starting Gateway Bridge...${NC}"
sudo systemctl start chirpstack-gateway-bridge
sleep 1.5

####### start Network Server #######
echo "${BYellow}Starting NetworkServer...${NC}"
sudo systemctl start chirpstack-network-server
sleep 1.5

####### start Application Server #######
echo "${BYellow}Starting ApplicationServer...${NC}"
sudo systemctl start chirpstack-application-server
sleep 1.5

####### Print status of loraserver
printf "\n"
echo "${BYellow} ********************* Gateway Bridge STATUS *********************${NC}"
bridgeStatus="$(sudo systemctl status chirpstack-gateway-bridge)"
echo "${bridgeStatus}"

printf "\n\n"
sleep 1.5
echo "${BYellow} ********************* Network Server STATUS *********************${NC}"
networkStatus="$(sudo systemctl status chirpstack-network-server)"
echo "${networkStatus}"

printf "\n\n"
sleep 1.5

echo "${BYellow} ********************* Application Server STATUS *********************${NC}"
applicationStatus="$(sudo systemctl status chirpstack-application-server)"
echo "${applicationStatus}"

printf "\n\n"
sleep 1.5

echo "${BGreen}####################### Chirpstack LoRaServer Started ####################${NC}"
printf "\n"

####### Start firefox to loraServer page #######
echo "Start browser on ${BYellow}http://localhost:8080${NC}"
printf "\n"
