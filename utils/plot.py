import pandas as pd
import matplotlib.pyplot as plt

'''
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
'''


df = pd.read_excel (r'data/latency/latency_nuvolaPunti.xlsx', sheet_name='Test1') #for an earlier version of Excel, you may need to use the file extension of 'xls'
df.plot.scatter(x='delay', y='latency')

latencyPoints = df['latency'].to_numpy()
maxPoints = df['max'].to_numpy()
minPoints = df['min'].to_numpy()

print(len(df['latency'].to_numpy()))
print(len(df['max'].to_numpy()))


plt.show()
