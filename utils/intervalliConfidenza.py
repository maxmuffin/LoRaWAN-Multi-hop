import csv
import datetime
import pandas as pd
import math
import scipy as sp
import scipy.stats


ok = 0
empty = 0
filename = 'Test1'

date0 = '00:00:00.000000'
dateZero = datetime.datetime.strptime(date0, "%H:%M:%S.%f")

def confidenceInterval(mean, list, devS, interval = 0.95, method = 't'):
  mean_val = mean
  n = len(list)
  stdev = devS
  if method == 't':
    test_stat = sp.stats.t.ppf((interval + 1)/2, n)
  elif method == 'z':
    test_stat = sp.stats.norm.ppf((interval + 1)/2)
  print(lower_bound = mean_val - test_stat * stdev / math.sqrt(n))
  print(upper_bound = mean_val + test_stat * stdev / math.sqrt(n))

with open(filename+'.csv','r') as csvinput:
    #with open('output'+filename+'.csv', 'a') as csvoutput:
    #writer = csv.writer(csvoutput)
    csv_file = csv.reader(csvinput)
    next(csv_file, None)
    timeList = []
    for row in csv_file:
        delay = datetime.datetime.strptime(row[7], '%H:%M:%S.%f')
        if delay != dateZero:
            #timestamp = datetime.timestamp(delay)
            stringa = str(delay)
            s1 = stringa[11:]
            print(s1)
            timeList.append(s1)

            ok+=1
        else:
            empty+=1

        tot = ok + empty
    print("Got pkt:"+ str(ok) + " Lost pkt:" +str(empty) + " Tot:" + str(tot) )

    timeAVG = str(pd.Series(pd.to_timedelta(timeList)).mean())
    secondsAVG = timeAVG[13:15]
    nanosecAVG = timeAVG[16:]
    averageTime = secondsAVG +"."+nanosecAVG
    floatAvg = float(averageTime)

    devStd = str(pd.Series(pd.to_timedelta(timeList)).std())
    secondsDevStd = devStd[13:15]
    nanosecDevStd = devStd[16:]
    StandardDeviation = secondsDevStd +"."+nanosecDevStd
    flotDevStd = float(StandardDeviation)

    confidenceInterval(floatAvg, timeList, flotDevStd)

    print("Average time:\t\t"+ str(floatAvg)+"s")
    print("Standard Deviation:\t" +str(flotDevStd))
