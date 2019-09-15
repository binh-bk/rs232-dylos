#! /usr/bin/python3
# Binh Nguyen, September 15, 2019
# capture and store data in sqlite

import time
import json
import os
import paho.mqtt.client as mqtt
from sqlalchemy import create_engine

basedir = os.path.abspath(os.path.dirname(__file__))
logFile = os.path.join(basedir, '{}.log'.format(__file__))

# SHORTCUTS
mqtt_topic = "air/dylos"
mqtt_user = 'youruser'
mqtt_pw = 'your_mqtt_password'
broker_ip = '192.168.1.xx' #your MQTT server IP
db_string = f'sqlite:///{basedir}/dylos'
engine = create_engine(db_string)

def send_DB(cmd):
    try:
        conn = engine.connect()
        conn.execute(cmd)
        print(cmd)
    except Exception as e:
        with open(logFile, 'a') as f:
            time_ = time.ctime()
            f.write(f'{time_}: {e}')
            print(f'{time_}: {e}\n')
    return None


def takeTime():
    return time.strftime("%Y-%m-%d %H:%M:%S")

def on_connect(client, userdata, flags, rc):
    client.subscribe(mqtt_topic)
    print("Connected with result code "+str(rc))
    return None


def on_message(client, userdata, msg):
    time_ = takeTime()
    try:
        payload = msg.payload.decode('UTF-8')
        if len(payload) > 0:
            payload = json.loads(payload.lower())
        if msg.retain == 0:
            if payload['sensor'] in ['dc1100']:
                try:
                    small = int(payload['small'])
                    large = int(payload['large'])
                    data = {"sensor": "dylos","time": time_, 'uptime': payload['uptime'] ,'small': small, 'large': large}
                    cmd_ = """INSERT INTO {sensor} VALUES ('{time}', {uptime},{small},{large});""".format(
                        **data)
                    send_DB(cmd_)
                    print("-"*10)
                except Exception as e:
                    print(f'Error: {e}')
    except Exception as e:
        print("Exception: " + str(e))
        with open(logFile, 'a') as f:
            f.write('{}: {}\n'.format(time_, e))
    return None


def on_disconnect(client, userdata, rc):
    if rc != 0:
        print("Unexpected disconnection!")
    else:
        print("Disconnecting")
    return None


# Program starts here
# create_table = 'CREATE TABLE dylos (time timestamp, uptime int, small int, large int);'
# send_DB(create_table)

client = mqtt.Client()
client.username_pw_set(username=mqtt_user, password=mqtt_pw)
client.connect(broker_ip, 1883, 60)
client.on_connect = on_connect
client.on_message = on_message
client.on_disconnect = on_disconnect
time.sleep(1)
client.loop_forever()
