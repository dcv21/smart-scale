from flask import Flask, request
import asyncio, cv2, json, numpy as np, torch, os, json, uuid
from utils_func import calc_price, write_receipt, generate_receipt

app = Flask(__name__)

@app.route('/image', methods = ['POST'], )
def image():
    file = request.files['imageFile']
    # convert string of image data to uint8
    nparr = np.frombuffer(file.read(), np.uint8)
    # decode image
    img = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
    cv2.imwrite(f'images/{file.filename}', img)
    return 'success', 200


@app.route('/predict', methods = ['POST'], )
def predict():
    if (request.is_json):
        data = request.get_json()
        # imgdata = base64.b64decode(str(data['image']))
        # data.pop('image', None)
        # nparr = np.frombuffer(imgdata, np.uint8)
        # decode image
        img = cv2.imread(f"images/{data['id']}.jpg")[..., ::-1]
        # cv2.imwrite('image.jpg', img)

        results = model([img], size=640)

        df = results.pandas().xyxy[0]
        pred_lst = df['name'].unique()

        if (len(pred_lst) == 1):
            data['type'] = pred_lst[0]
            data['price'] = calc_price(data['type'], data['weight'])
        elif (len(pred_lst) < 1):
            data['type'] = 'No Detection'
            del data['weight']
        else:
            data['type'] = 'Multiple Detections'
            del data['weight']

        if data['mode'] == 1 and not (data['type'] == 'No Detection' or data['type'] == 'Multiple Detections'):
            filename = 'receipts.json'
            json_object = None
            with open(filename, 'r') as openfile:
                json_object = json.load(openfile)
                data['id'] = str(data['id'])
                if not (data['id'] in json_object):
                    json_object[data['id']] = []
                json_object[data['id']].append([data['type'], data['weight'], data['price']])
            tempfile = os.path.join(os.path.dirname(filename), str(uuid.uuid4()))
            with open(tempfile, 'w') as f:
                json.dump(json_object, f, indent=4)
            os.replace(tempfile, filename)
            
        return json.dumps(data), 200
    else:
        return 'failed', 400
    
@app.route('/receipt', methods = ['POST'], )
def receipt():
    if (request.is_json):
        data = request.get_json()
        filename = 'receipts.json'
        json_object = None
        with open(filename, 'r') as openfile:
            json_object = json.load(openfile)
            data['id'] = str(data['id'])
            index = 0
            while (index < len(json_object[data['id']]) - 1):
                i = index + 1
                while(i < len(json_object[data['id']])):
                    if (json_object[data['id']][index][0] == json_object[data['id']][i][0]):
                        json_object[data['id']][index][1] += json_object[data['id']][i][1]
                        json_object[data['id']][index][2] += json_object[data['id']][i][2]
                        del json_object[data['id']][i]
                    else:
                        i += 1
                index += 1

            asyncio.run(write_receipt(data, json_object))
            
            data['receipt'] = generate_receipt(data, json_object)

            del json_object[data['id']]
        
        tempfile = os.path.join(os.path.dirname(filename), str(uuid.uuid4()))
        with open(tempfile, 'w') as f:
            json.dump(json_object, f, indent=4)
        os.replace(tempfile, filename)

        return json.dumps(data), 200
    else:
        return 'failed', 400
    
@app.route('/remove', methods = ['POST'], )
def remove():
    if (request.is_json):
        data = request.get_json()
        filename = 'receipts.json'
        json_object = None
        with open(filename, 'r') as openfile:
            json_object = json.load(openfile)
            data['id'] = str(data['id'])
            if not (data['id'] in json_object):
                return 'failed', 400
            json_object[data['id']].pop()

        tempfile = os.path.join(os.path.dirname(filename), str(uuid.uuid4()))
        with open(tempfile, 'w') as f:
            json.dump(json_object, f, indent=4)
        os.replace(tempfile, filename)
        return 'success', 200
    else:
        return 'failed', 400

if __name__ == '__main__':
    model = torch.hub.load('yolov5', 'custom', path='best.pt', source='local')
    app.run(debug=True, host='0.0.0.0', port=8000)