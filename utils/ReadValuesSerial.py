import serial
import csv
import datetime
import uuid
import time
import os
import datetime

# linux port
port1 = 'COM10'

serial_speed = 9600

opMode = str(input("Inserisci la modalità di utilizzo: "))

unique_filename = opMode+"_"+str(uuid.uuid4())+".csv"

with open("data/{}".format(unique_filename), 'w', newline='') as csvfile:
    filewriter = csv.writer(csvfile, delimiter=',')
    filewriter.writerow(['pktCounter', 'bytes', 'timestamp'])
    startTime = datetime.datetime.now()
    # read relevations from Arduino
    ser = serial.Serial(port1, serial_speed)
    for i in range(0,20):

        saveRow = True

        inputSer = ser.readline()

        decodedSer = inputSer.decode("utf-8")

        data = decodedSer.split(",")

        equalizedData = []

        for temp in data:
            try:
                tmp = temp.replace("\n","")
                equalizedData.append(tmp)
            except ValueError:
                saveRow = False
                print("Discard Row")


        date = datetime.datetime.now().strftime("%Y-%m-%dT%H:%M:%S.%f")
        equalizedData.append(date)

        if saveRow:
            filewriter.writerow(equalizedData)

        print("Write "+ str(i+1) +"° row")

    csvfile.close()

    endTime = datetime.datetime.now()
    delta = endTime - startTime

    '''
    correct = input("Correct measurement? [Y/n]: ")
    if correct in ["Y","y",""] :
        print("Measurement keeped in time: "+ str(delta))

    else:
        os.remove("data/{}".format(unique_filename))
        print("Measurement discarded")
    '''
    print("Measurements keeped in time: "+ str(delta))
