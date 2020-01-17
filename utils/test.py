import datetime
date1 = "2020-01-12T15:17:50.680058000Z"
date2 = "2020-01-12T15:15:50.680058000Z"

def getTimeStamp():

    print(len(date1))

    date0 = date1[:-4]
    dateOK = date2[:-4]

    print(len(date0))
    Dates = datetime.datetime.strptime(date0, '%Y-%m-%dT%H:%M:%S.%f')



    DateNow = datetime.datetime.utcnow()
    ciao = datetime.datetime.strftime(DateNow, '%Y-%m-%dT%H:%M:%S.%f')
    ciao2 = datetime.datetime.strptime(dateOK, '%Y-%m-%dT%H:%M:%S.%f')
    print(Dates)
    print(ciao2)

    if ciao2 < Dates:
        print("Negativo")
        end = Dates - ciao2
    else:
        print("Stappost")
        end = ciao2 - Dates

    print(end)
getTimeStamp()
