#!/bin/bash

BGreen='\033[1;32m'
BYellow='\033[1;33m'
NC='\033[0m'

####### stop Gateway Bridge #######
echo -e "${BYellow}Stopping Gateway Bridge...${NC}"
sudo systemctl stop chirpstack-gateway-bridge
sleep 1.5

####### stop Network Server #######
echo -e "${BYellow}Stopping NetworkServer...${NC}"
sudo systemctl stop chirpstack-network-server
sleep 1.5

####### stop Application Server #######
echo -e "${BYellow}Stopping ApplicationServer...${NC}"
sudo systemctl stop chirpstack-application-server
sleep 1.5

####### Print status of loraserver
printf "\n"
echo -e "${BYellow} ********************* Gateway Bridge STATUS *********************${NC}"
bridgeStatus="$(sudo systemctl status chirpstack-gateway-bridge)"
echo "${bridgeStatus}"

printf "\n\n"
sleep 1.5
echo -e "${BYellow} ********************* Network Server STATUS *********************${NC}"
networkStatus="$(sudo systemctl status chirpstack-network-server)"
echo "${networkStatus}"

printf "\n\n"
sleep 1.5

echo -e "${BYellow} ********************* Application Server STATUS *********************${NC}"
applicationStatus="$(sudo systemctl status chirpstack-application-server)"
echo "${applicationStatus}"

printf "\n\n"
sleep 1.5

echo -e "${BRed}####################### LoRaServer Stopped ####################${NC}"
