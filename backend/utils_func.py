# import xlsxwriter module
import xlsxwriter
from datetime import datetime

def calc_price(pred, weight):
    prices = {
        'Apple': 50000,
        'Banana': 25000,
        'Orange': 72000,
        'Pineapple': 25000,
        'Watermelon': 12000,
        'Grape': 64000
    }

    return prices[pred] * weight

#############################################################

async def write_receipt(data, json_object):
    now = datetime.now()
    date_time = now.strftime('%d_%m_%Y_%H_%M_%S')

    workbook_name = f'receipts/{date_time}_{data["id"]}.xlsx'
    workbook = xlsxwriter.Workbook(workbook_name)
    worksheet = workbook.add_worksheet()

    worksheet.write('A1', 'Type')
    worksheet.write('B1', 'Weight')
    worksheet.write('C1', 'Price')

    for i in range(0, len(json_object[data['id']])):
        worksheet.write(f'A{i + 2}', json_object[data['id']][i][0])
        worksheet.write(f'B{i + 2}', json_object[data['id']][i][1])
        worksheet.write(f'C{i + 2}', json_object[data['id']][i][2])

        if (i == len(json_object[data['id']]) - 1):
            worksheet.write(f'B{i + 3}', 'Total:')
            worksheet.write(f'C{i + 3}', f'=SUM(C2:C{i + 2})')
    
    workbook.close()

#############################################################

def generate_receipt(data, json_object):
    tmp = ''
    total = 0
    for i in range(0, len(json_object[data['id']])):
        total += json_object[data['id']][i][2]
        tmp += f"{json_object[data['id']][i][0]} {json_object[data['id']][i][1]}KG\r\n{json_object[data['id']][i][2]}VND\r\n"
    tmp += 'Total: ' + str(total) + 'VND' + ('\r\n' * 5)
    return tmp