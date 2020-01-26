import datetime
import random
import csv

date1 = "2020-01-12T15:17:50.680058000Z"
date2 = "2020-01-12T15:15:50.680058000Z"
startDate = "2020-01-21T11:44:08.555966"

filename = "test"
def getTimeStamp():

    #print(len(date1))

    #date0 = date1[:-4]
    #dateOK = date2[:-4]

    #print(len(date0))
    Dates = datetime.datetime.strptime(startDate, '%Y-%m-%dT%H:%M:%S.%f')

    Milliseconds = random.randint(821000,989752)
    fulldate = Dates + datetime.timedelta(microseconds=Milliseconds)

    print(fulldate)

    print(str(fulldate)+str(random.randint(123,978))+"Z")


#getTimeStamp()

with open(filename+'.csv','r') as csvinput:
    with open(filename+'nuovo.csv', 'w') as csvoutput:
        writer = csv.writer(csvoutput)
        csv_file = csv.reader(csvinput)


        cleanedData = []

        for row in csv_file:
            try:
                dateTX = datetime.datetime.strptime(row[0], '%Y-%m-%dT%H:%M:%S.%f')


                Milliseconds = random.randint(821000,989752)
                fulldate = dateTX + datetime.timedelta(microseconds=Milliseconds)


                newDate =str(fulldate)+str(random.randint(123,978))+"Z"

                strDate = str(newDate)+ '\n'
                cleanedData.append(strDate)

            except:

                loss +=1
                continue

        writer.writerow(cleanedData)

csvoutput.close()
