#!/bin/bash

BYellow='\033[1;33m'
NC='\033[0m'

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
