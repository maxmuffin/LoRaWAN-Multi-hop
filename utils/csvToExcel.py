import pandas
import os

# Read csv file use pandas module.
def read_csv_file_by_pandas(csv_file):
    data_frame = None
    if(os.path.exists(csv_file)):
        data_frame = pandas.read_csv(csv_file)
        #data_frame = data_frame.set_index('index')
    else:
        print(csv_file + " do not exist.")
    return data_frame

# Write pandas.DataFrame object to a csv file.
def write_to_csv_file_by_pandas(csv_file_path, data_frame):
    data_frame.to_csv(csv_file_path)
    #print(csv_file_path + ' has been created.')

# Write pandas.DataFrame object to an excel file.
def write_to_excel_file_by_pandas(excel_file_path, data_frame):
    excel_writer = pandas.ExcelWriter(excel_file_path, engine='xlsxwriter')
    #data_frame.to_excel(excel_file_path, sheet_name='Sheet1', index=False, engine='xlsxwriter')
    data_frame.to_excel(excel_writer, sheet_name=name)
    excel_writer.save()
    #print(excel_file_path + ' has been created.')

# Sort the data in DataFrame object by name that data type is string.
def sort_data_frame_by_string_column(data_frame):
    data_frame = data_frame.sort_values(['timestamp'], inplace=True)


if __name__ == '__main__':

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
    filename = 'latency'+name+'EXCEL'


    #path='../dataset/merged/'+folder+'/outputPosture'+folder
    path = 'data/latency/Test'+test+'/Graph/'+filename
    excelPath = 'data/latency/Test'+test+'/Graph/xslx/'+filename

    data_frame = read_csv_file_by_pandas(path+'.csv')

    print("Converting initial csv to xslx")
    write_to_excel_file_by_pandas(excelPath+'.xlsx', data_frame)
    #sort_data_frame_by_string_column(data_frame)
    #print("Sorting initial csv")
    #write_to_csv_file_by_pandas('../dataset/merged/'+folder+'/SORTED_outputPosture'+folder+'.csv', data_frame)
    #print("Converting sorted csv to xslx")
    #write_to_excel_file_by_pandas('../dataset/merged/'+folder+'/SORTED_outputPosture'+folder+'.xlsx', data_frame)
    print("###### DONE ######")
