import csv
import datetime
import pandas as pd
import math
import xlsxwriter

got = 0
loss = 0

test = input("Inserisci numero Test (1 - 3): ")
delay = input("Inserisci il delay [300ms, 500ms, 750ms, 1000ms, 1500ms, 2000ms]: ")

if test == "3":
    sleepTime = input("Inserisci lo sleepTime [500ms, 5s, 10s]: ")
    if sleepTime == "500":
        name = 'Test'+test+' '+delay+'ms '+sleepTime+'ms'
    else:
        name = 'Test'+test+' '+delay+'ms '+sleepTime+'sec'
else:
    name = 'Test'+test+' '+delay+'ms'

#name = 'Test'+test+'_'+delay+'ms-'+sleepTme+'ms'
filename = 'latency'+name

date0 = '0:00:00.000000'
dateZero = datetime.datetime.strptime(date0, "%H:%M:%S.%f")

def intervalliConfidenza(mean, list, devS, interval = 0.95):
    mean_val = mean
    n = len(list)
    stdev = devS
    errore_standard = stdev/(math.sqrt(n))
    marginError = 1.96 * errore_standard
    upper_margin = mean_val + marginError
    lower_margin = mean_val - marginError

    return errore_standard, marginError, upper_margin, lower_margin

with open('data/latency/Test'+test+'/'+filename+'.csv','r') as csvinput:
    csv_file = csv.reader(csvinput)
    #header = next(csv_file)
    header = next(csv_file)
    timeList = []
    for row in csv_file:

        latency = datetime.datetime.strptime(row[7], '%H:%M:%S.%f')
        if latency != dateZero:
            latencyString = str(latency)
            formattedLatency = latencyString[11:]
            timeList.append(formattedLatency)

            got+=1
        else:
            loss+=1

        tot = got + loss
    PDR = (got/tot)*100
    print("Got pkt:"+ str(got) + " Lost pkt:" +str(loss) + " Tot:" + str(tot)+ "\t PDR:"+ str(PDR)[:5]+"%")

    timeAVG = str(pd.Series(pd.to_timedelta(timeList)).mean())
    secondsAVG = timeAVG[13:15]
    nanosecAVG = timeAVG[16:]
    averageTime = secondsAVG +"."+nanosecAVG
    floatAvg = float(averageTime)

    devStd = str(pd.Series(pd.to_timedelta(timeList)).std())
    #print(devStd)
    secondsDevStd = devStd[13:15]
    nanosecDevStd = devStd[16:]
    StandardDeviation = secondsDevStd +"."+nanosecDevStd
    floatDevStd = float(StandardDeviation)

    stdErr, marginErr, upMargin, lowMargin = intervalliConfidenza(floatAvg, timeList, floatDevStd)

    print("Average time:\t\t"+ str(floatAvg)+"s\n")
    print("Standard Deviation:\t" +str(floatDevStd))
    print("Errore standard:\t"+str(stdErr))
    print("Margine Errore:\t"+str(marginErr))
    print("intervalli: \t+"+ str(upMargin)+" \t"+str(floatAvg)+"\t -"+str(lowMargin))

with open('data/latency/Test'+test+'/'+filename+'.csv','r') as csvinput:
    with open('data/latency/Test'+test+'/Graph/'+filename+'EXCEL.csv', 'w') as csvoutput:
        writer = csv.writer(csvoutput)
        csv_file = csv.reader(csvinput)

        header = next(csv_file)

        cleanedData = []
        newHeader = ["latency","max","min"]
        #header.append("latency RX-TX")
        writer.writerow(newHeader)

        for row in csv_file:
            if row[7] != date0:
                timeStr = str(row[7])[5:]
                floatTime = float(timeStr)
                cleanedData.append(floatTime)
                cleanedData.append(floatTime+marginErr)
                cleanedData.append(floatTime-marginErr)
                cleanedData.append('\n')

        writer.writerow(cleanedData)

csvoutput.close()
