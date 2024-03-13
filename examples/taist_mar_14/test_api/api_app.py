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

# start instance
app = FastAPI()

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

@app.get('/api/mockup')
async def api(request: Request):
    resp = {'status':'OK'}
    # 
    dev_db = mongo_client.dev_db
    dev_col = dev_db.devices
    dev_log = dev_db.device_log
    user_db = mongo_client.user_db
    user_col = user_db.users

    return jsonable_encoder(resp)

@app.get('/api/query_devs/{dev_id}')
async def api(dev_id: str, request: Request):
    resp = {'status':'OK'}
    # 
    dev_db = mongo_client.dev_db
    dev_col = dev_db.devices
    dev_log = dev_db.device_log

    return jsonable_encoder(resp)

@app.get('/api/query_users/{user_name}')
async def api(user_name: str, request: Request):
    resp = {'status':'OK'}
    # 
    user_db = mongo_client.user_db
    user_col = user_db.users
    data = user_col.find_one({'user_name':user_name}, {'_id':False})
    resp['data'] = data
    return jsonable_encoder(resp)

@app.get('/api/tear_down')
async def api(request: Request):
    resp = {'status':'OK'}
    # 
    dev_db = mongo_client.dev_db
    dev_col = dev_db.devices
    dev_log = dev_db.device_log
    user_db = mongo_client.user_db
    user_col = user_db.users

    return jsonable_encoder(resp)