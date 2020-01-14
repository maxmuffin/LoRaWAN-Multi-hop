import csv
import datetime

got = 0
loss = 0

filename = 'Test1_750ms'
with open('data/'+filename+'.csv','r') as csvinput:
    with open('data/latency'+filename+'.csv', 'w') as csvoutput:
        writer = csv.writer(csvoutput)
        csv_file = csv.reader(csvinput)

        header = next(csv_file)

        cleanedData = []
        header.append("latency RX-TX")
        writer.writerow(header)

        for row in csv_file:
            cleanedData.append(row[0])
            cleanedData.append(row[1])
            cleanedData.append(row[2])
            cleanedData.append(row[3])
            cleanedData.append(row[4])
            try:
                dateTX = datetime.datetime.strptime(row[5], '%Y-%m-%dT%H:%M:%S.%f')

                if len(row[6]) > 26: #for cut javascript datetime and standardize with python datetime
                    javascriptDatetime = row[6]
                    correctDatetime = javascriptDatetime[:26]
                    dateRX = datetime.datetime.strptime(correctDatetime, '%Y-%m-%dT%H:%M:%S.%f')
                else:
                    dateRX = datetime.datetime.strptime(row[6], '%Y-%m-%dT%H:%M:%S.%f')

                if dateRX < dateTX:
                    row[5] = dateRX
                    row[6] = dateTX
                    delta = dateTX - dateRX

                else:
                    row[5] = dateTX
                    row[6] = dateRX
                    delta = dateRX - dateTX

                #print(str(delta))
                got +=1

                diff= str(delta)+ '\n'
                cleanedData.append(row[5])
                cleanedData.append(row[6])
                cleanedData.append(diff)

            except:

                diff = '0:00:00.000000\n'
                cleanedData.append(row[5])
                cleanedData.append(row[6])
                cleanedData.append(diff)
                #print(diff)
                loss +=1
                continue

        tot = got + loss
        print("Got pkt:"+ str(got) + " Lost pkt:" +str(loss) + " Tot:" + str(tot) )
        writer.writerow(cleanedData)

csvoutput.close()
