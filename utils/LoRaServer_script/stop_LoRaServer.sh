#!/bin/bash

BRed='\033[1;31m'
BYellow='\033[1;33m'
NC='\033[0m'

####### stop LORA SERVER #######
echo -e "${BYellow}Stopping LoRaServer...${NC}"
sudo systemctl stop loraserver
sleep 1.5

####### stop LORA APP SERVER #######
echo -e "${BYellow}Stopping LoRaAppServer...${NC}"
sudo systemctl stop lora-app-server
sleep 1.5

####### Print status of loraserver
printf "\n"
echo -e "${BYellow} ********************* LoraServer STATUS *********************${NC}"
serverStatus="$(sudo systemctl status loraserver)"
echo "${serverStatus}"

printf "\n\n"
sleep 1.5
####### Print status of lora-app-server
echo -e "${BYellow} ******************** LoraAppServer STATUS *******************${NC}"
appServerStatus="$(sudo systemctl status lora-app-server)"

echo "${appServerStatus}"
printf "\n"
sleep 1.5

echo -e "${BRed}####################### LoRaServer Stopped ####################${NC}"
printf "\n"
