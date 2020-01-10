import csv
import datetime


i = 0
j = 0
filename = 'Test2RelayCleanedData'
with open(filename+'.csv','r') as csvinput:
    with open('output'+filename+'.csv', 'a') as csvoutput:
        writer = csv.writer(csvoutput)
        csv_file = csv.reader(csvinput)
        next(csv_file, None)

        differenceTime = []

        for row in csv_file:
            try:
                dateTX = datetime.datetime.strptime(row[5], '%Y-%m-%dT%H:%M:%S.%f')
                dateRX = datetime.datetime.strptime(row[6], '%Y-%m-%dT%H:%M:%S.%f')
                delta = dateRX - dateTX

                print(str(delta))
                i+=1

                diff= str(delta) + "\n"
                differenceTime.append(diff)

            except:

                diff = '0:00:00.000000\n'
                differenceTime.append(diff)
                print(diff)
                j+=1
                continue

        k = i + j
        print("Got pkt:"+ str(i) + " Lost pkt:" +str(j) + " Tot:" + str(k) )
        writer.writerow(differenceTime)

csvoutput.close()
