import os
import sys
import logging
from datetime import datetime
import json

from fastapi import FastAPI, Request
from fastapi.encoders import jsonable_encoder

from pymongo import MongoClient

# logging configuration
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

# pymongo configuration
mongo_host = os.getenv('MONGO_HOST', None)
if mongo_host is None:
    logging.error('MONGO_HOST undefined.')
    sys.exit(1)
mongo_port = os.getenv('MONGO_PORT', None)
if mongo_port is None:
    logging.error('MONGO_PORT undefined.')
    sys.exit(1)
mongo_client = MongoClient(mongo_host, int(mongo_port))

# start instance
app = FastAPI()

@app.get('/api/register/{dev_id}')
async def on_register(dev_id: str, request: Request):
    resp = {'status':'OK'}
    # create database entities
    dev_db = mongo_client.dev_db
    dev_col = dev_db.devices
    new_dev = {
        'dev_id': dev_id,
        'created_at': datetime.now(),
        'user_id': None,
        'registered_at': None
    }
    dev_id = dev_col.insert_one(new_dev).inserted_id
    resp['dev_id'] = str(dev_id)
    return jsonable_encoder(resp)

@app.get('/api/list')
async def on_list(request: Request):
    resp = {'status':'OK'}
    # query and return all devices
    dev_db = mongo_client.dev_db
    dev_col = dev_db.devices
    resp['devices'] = list( dev_col.find({}, {'_id': False}) )
    return jsonable_encoder(resp)

@app.get('/api/log/{dev_id}')
async def on_log(dev_id: str, request: Request):
    resp = {'status':'OK'}
    # query and return all logs for a device
    dev_db = mongo_client.dev_db
    dev_log = dev_db.device_log
    resp['dev_id'] = dev_id
    resp['log'] = list( dev_log.find({'dev_id': dev_id}, {'_id': False}) )
    return jsonable_encoder(resp)