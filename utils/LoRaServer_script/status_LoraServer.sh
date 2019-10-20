#!/bin/bash

BYellow='\033[1;33m'
NC='\033[0m'

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


