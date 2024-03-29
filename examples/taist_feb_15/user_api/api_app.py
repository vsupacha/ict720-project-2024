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

@app.get('/api/{dev_id}')
async def api(dev_id: str, request: Request):
    resp = {'status':'OK'}
    # 
    user_db = mongo_client.user_db
    user_col = user_db.users

    return jsonable_encoder(resp)