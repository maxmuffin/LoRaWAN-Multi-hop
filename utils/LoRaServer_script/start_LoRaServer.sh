#!/bin/bash

BGreen='\033[1;32m'
BYellow='\033[1;33m'
NC='\033[0m'

####### start LORA SERVER #######
echo -e "${BYellow}Starting LoRaServer...${NC}"
sudo systemctl start loraserver
sleep 1.5

####### start LORA APP SERVER #######
echo -e "${BYellow}Starting LoRaAppServer...${NC}"
sudo systemctl start lora-app-server
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
printf "\n\n"
sleep 1.5

echo -e "${BGreen}####################### LoRaServer Started ####################${NC}"
printf "\n"

####### Start firefox to loraServer page #######
echo -e "Start browser on ${BYellow}http://localhost:8080${NC}"
printf "\n"


