#! /usr/bin/python3
# Binh Nguyen, September 15, 2019

import serial
import time
import socket
from paho.mqtt import publish, subscribe
import json
import os

mqtt = '192.168.1.xx'
ser = serial.Serial(port='/dev/ttyUSB0', baudrate=9600, timeout=2)

def get_time():
    '''record timestamp'''
    time_ = time.strftime('%x %X', time.localtime(time.time()))
    return time_

def prepare_mesg(head_, payload):
    '''make a nice dictionary as payload'''
    head_ = head_.split(',')
    payload = payload.split(',')
    if len(head_) == len(payload):
        dict_ = dict()
        for col, value in zip(head_, payload):
            dict_[col] = value
        return dict_
    else:
        print('either header or payload missing data')
        raise SystemExit


def internet_ready():
    '''check if internet connect is ready'''
    try:
        _ = socket.create_connection((mqtt, 1883), 2)
        return True
    except OSError:
        print("Internet is not available.")
    return False

def push_MQTT(mesg):
    '''push a string of data via MQTT server'''
    topic = 'air/dylos'
    auth = {'username': 'username', 'password': 'your_mqtt_password'}
    mesg = json.dumps(mesg)
    try:
        publish.single(topic, mesg, hostname=mqtt, auth=auth)
    except Exception as e:
        print('Error: {}'.format(e))
        pass
    return None


if __name__ == '__main__':
    '''the main program, it can run without pushing data to an MQTT'''
    push_mqtt = False
    head_ = 'sensor,time,small,large'
    basedir = os.path.abspath(os.path.dirname(__file__))
    logFile = os.path.join(basedir, 'dyloslog.csv')
    while True:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf8')
            output = line.split(',')
            output = [x.strip() for x in output]
            small = output[0]
            large = output[-1]
            data = ','.join(['dylos', get_time(), small, large])
           
            with open(logFile, 'a+') as f:
                f.write(f'{data}\n')
            if push_MQTT:
                mesg = prepare_mesg(head_, data)
                push_MQTT(mesg)
                print(f'Pushed {mesg}')
        else:
            time.sleep(1)
