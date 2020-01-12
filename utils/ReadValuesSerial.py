import serial
import csv
import datetime
import uuid
import time
import os
import codecs

# linux port
port1 = 'COM10'

serial_speed = 9600

opMode = str(input("Inserisci la modalità di utilizzo: "))

unique_filename = opMode+"_"+str(uuid.uuid4())+".csv"

with open("data/acquiredData/{}".format(unique_filename), 'w', newline='') as csvfile:
    filewriter = csv.writer(csvfile, delimiter=',')
    filewriter.writerow(['pktCounter', 'bytes', 'frame', 'base64', 'FRM Payload', 'timestamp_TX'])
    startTime = datetime.datetime.now()
    # read relevations from Arduino
    ser = serial.Serial(port1, serial_speed)
    for i in range(0,15): #15 * 7 times for SyncTest, 102 for End and Forward Test

        saveRow = True

        inputSer = ser.readline()

        decodedSer = inputSer.decode("utf-8")

        data = decodedSer.split(",")

        equalizedData = []
        index = 0

        for temp in data:
            try:
                tmp = temp.replace("\n","")
                if index == 2:
                    hex = tmp
                    b64 = codecs.encode(codecs.decode(hex, 'hex'), 'base64').decode().replace("\n","")
                    equalizedData.append(tmp)
                    equalizedData.append(b64)
                    equalizedData.append(b64[12:17])
                else:
                    equalizedData.append(tmp)
                index+=1
            except ValueError:
                saveRow = False
                print("Discard Row")


        date = datetime.datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%S.%f")
        equalizedData.append(date)

        if saveRow:
            filewriter.writerow(equalizedData)

        print("Write "+ str(i+1) +"° row")

    csvfile.close()

    endTime = datetime.datetime.now()
    delta = endTime - startTime

    print("Measurements keeped in time: "+ str(delta))
