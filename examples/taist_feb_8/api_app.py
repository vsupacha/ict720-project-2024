import os
import sys
import logging
from datetime import datetime
import json

from fastapi import FastAPI, Request
from fastapi.encoders import jsonable_encoder
from fastapi_mqtt import FastMQTT, MQTTConfig

from dotenv import load_dotenv
load_dotenv()

# logging configuration
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

# start instance
app = FastAPI()
mqtt_config = MQTTConfig(host=os.getenv('MQTT_BROKER', 'localhost'),
                         port=os.getenv('MQTT_PORT', 1883),
                         keepalive=60)
mqtt = FastMQTT(config=mqtt_config)
mqtt.init_app(app)

app.last_mqtt_msg = None

# mqtt event handler
@mqtt.on_connect()
async def on_mqtt_connect(mqtt_client, flags, rc, properties):
    pass

@mqtt.on_message()
async def on_mqtt_message(mqtt_client, topic, payload, qos, properties):
    logging.info(f"Received message from {topic}: {payload.decode()}")

@mqtt.subscribe(os.getenv('MQTT_SOUND_TOPIC', 'test/sound'))
async def on_sound_message(mqtt_client, topic, payload, qos, properties):
    logging.info(f"Received sound message: {payload.decode()}")
    json_msg = json.loads(payload.decode())
    if json_msg['status'] == 'silent':
        app.last_mqtt_msg = json_msg
        app.last_mqtt_msg['dev_id'] = topic.split('/')[-1]

@mqtt.subscribe(os.getenv('MQTT_HB_TOPIC', 'test/hb'))
async def on_hb_message(mqtt_client, topic, payload, qos, properties):
    logging.info(f"Received heartbeat message: {payload.decode()}")

@mqtt.subscribe(os.getenv('MQTT_ENV_TOPIC', 'test/sound'))
async def on_env_message(mqtt_client, topic, payload, qos, properties):
    logging.info(f"Received env message: {payload.decode()}")

@mqtt.on_disconnect()
async def on_mqtt_disconnect(mqtt_client, packet, exc=None):
    logging.warning(f"Disconnected: {exc}")

@app.get('/api/{dev_id}')
async def api(dev_id: str, request: Request):
    resp = {'status':'OK'}
    # 
    if app.last_mqtt_msg is not None:
        if app.last_mqtt_msg['dev_id'] == dev_id:
            resp['data'] = app.last_mqtt_msg
        else:
            resp['data'] = "No device found"
    else:
        resp['data'] = "None"
    return jsonable_encoder(resp)