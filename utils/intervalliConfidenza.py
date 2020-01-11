import csv
import datetime
import pandas as pd
import math
import xlsxwriter



got = 0
loss = 0
filename = 'Test3'

date0 = '00:00:00.000000'
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

with open(filename+'.csv','r') as csvinput:
    csv_file = csv.reader(csvinput)
    next(csv_file, None)
    timeList = []
    for row in csv_file:
        delay = datetime.datetime.strptime(row[7], '%H:%M:%S.%f')
        if delay != dateZero:
            stringa = str(delay)
            s1 = stringa[11:]
            #print(s1)
            timeList.append(s1)

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

    data = []
    data.append(filename)
    data.append(got)
    data.append(loss)
    data.append(tot)
    data.append(str(PDR)[:5])
    data.append(floatAvg)
    data.append(floatDevStd)
    data.append(stdErr)
    data.append(marginErr)
    data.append(upMargin)
    data.append(lowMargin)

# Create File and Save data on it
workbook = xlsxwriter.Workbook(filename+'.xlsx')
worksheet = workbook.add_worksheet()

row = 0
column = 0
header = ["Test", "gotPkt", "lossPkt", "totPkt", "PDR(%)","mean", "StdDev", "StdErr",
                    "marginErr95", "upMargin", "lowMargin"]
# Write Header
for item in header :
    worksheet.write(row, column, item)
    column += 1

row = 1
column = 0
# Write data
for item in data:
    worksheet.write(row, column, item)
    column +=1

workbook.close()
