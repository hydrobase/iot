import csv
import time
from pubnub import Pubnub

#Create + Open + Writer headers for CSV document

dataFile_name = str((time.localtime(time.time())).tm_mon)+'_'+str((time.localtime(time.time())).tm_mday)+'_'+str((time.localtime(time.time())).tm_hour)+'_'+str((time.localtime(time.time())).tm_min)+'pubNub_data.csv'
print dataFile_name
dataFile = open(dataFile_name, 'w')
fieldnames = ['month', 'day', 'year', 'hour', 'min', 'sec', 'lux', 'humidity', 'airTemp', 'waterTemp', 'pH', 'EC', 'TDS', 'PS']
writer = csv.DictWriter(dataFile, fieldnames=fieldnames)
writer.writeheader()
dataFile.close()

#State management for Water On/Off messages
waterOnSent = False
waterOffSent = False
lightOnSent = False
lightOffSent = False

#Create PubNub instance
pubnub = Pubnub(publish_key="pub-c-93c6c384-e1a0-412f-87cf-e626aaab6a00", subscribe_key="sub-c-8ec9d89e-e4aa-11e5-a4f2-0619f8945a4f", auth_key="40ed6434-1991-4f7a-8034-20a072abde43")

def callback(message, channel):
    global waterOnSent
    global waterOffSent

    #dictionary for conditions, toggeling between on and off
    on_off15min = {'on1': (time.localtime(time.time())).tm_min < 15,
                'off1': (time.localtime(time.time())).tm_min >= 15 and (time.localtime(time.time())).tm_min < 30,
                'on2': (time.localtime(time.time())).tm_min >= 30 and (time.localtime(time.time())).tm_min < 45,
                'off2': (time.localtime(time.time())).tm_min >= 45}

    #ONLY for debugging
    #on_off15 = {'on1': (time.localtime(time.time())).tm_min < 38,
    #            'off1': (time.localtime(time.time())).tm_min >= 38 and (time.localtime(time.time())).tm_min < 39,
    #            'on2': (time.localtime(time.time())).tm_min >= 39 and (time.localtime(time.time())).tm_min < 40,
    #            'off2': (time.localtime(time.time())).tm_min >= 40}

    on_off12hours = {'on': [(15 <= (time.localtime(time.time())).tm_hour),
                            ((time.localtime(time.time())).tm_hour) < 3],
                    'off': (3 <= (time.localtime(time.time())).tm_hour < 15),
                     }

    timeNow = time.localtime(time.time())

    #ONLY for debugging
    print timeNow
    print "\n"
    print "on_off15 on1: ", on_off15min['on1']
    print "on_off15 off1: ", on_off15min['off1']
    print "on_off15 on2: ", on_off15min['on2']
    print "on_off15 off2: ", on_off15min['off2']
    print "\n"
    print "water on sent:", waterOnSent
    print "water off sent:", waterOffSent
    print "\n"
    print "on_off12Hours on: ", on_off12hours['on']
    print "on_off12Hours off: ", on_off12hours['off']
    print "\n"
    print "light on sent:", lightOnSent
    print "light off sent:", lightOffSent
    print "\n"

    if type(message) is dict:
        if 'sender' in message:
            if message['sender']['name'] == u'Arduino--Mario':
                ecData = str("EC: " + str(message[u'EC']))
                ecData = ecData.split()
                ecData = ecData[1].split(',')
                dataFile = open(dataFile_name, 'a')
                writer = csv.DictWriter(dataFile, fieldnames=fieldnames)
                writer.writerow({
                    'month': (time.localtime(time.time())).tm_mon,
                    'day': (time.localtime(time.time())).tm_mday,
                    'year': (time.localtime(time.time())).tm_year,
                    'hour': (time.localtime(time.time())).tm_hour,
                    'min': (time.localtime(time.time())).tm_min,
                    'sec': (time.localtime(time.time())).tm_sec,
                    'lux': message[u'lux'],
                    'humidity': message[u'humidity'],
                    'airTemp': message[u'airTemp'],
                    'waterTemp': message[u'waterTemp'],
                    'pH': message[u'pH'], 'EC': ecData[0],
                    'TDS': ecData[1],
                    'PS': ecData[2]})
                dataFile.close()
            else:
                pass
        else:
            print "This message is meant for someone else: ", message
            print "\n"

    if (on_off15min['on1'] or on_off15min['on2']) and timeNow.tm_sec > 30 and timeNow.tm_sec < 35 and waterOnSent is False:
        publishWaterOn(channel)

    elif (on_off15min['off1'] or on_off15min['off2']) and timeNow.tm_sec > 30 and timeNow.tm_sec < 35 and waterOffSent is False:
        publishWaterOff(channel)

    if (True in on_off12hours['on']) and timeNow.tm_sec > 30 and timeNow.tm_sec < 35 and lightOnSent is False:
        publishLightsOn(channel)

    elif (on_off12hours['off']) and timeNow.tm_sec > 30 and timeNow.tm_sec < 35 and lightOffSent is False:
        publishLightsOff(channel)

def publishWaterOn(channel):
    time.sleep(2)
    global waterOnSent
    global waterOffSent
    print pubnub.publish(channel=channel, message={"actuators": {"32": 0}})
    waterOnSent = True
    waterOffSent = False


def publishWaterOff(channel):
    time.sleep(2)
    global waterOnSent
    global waterOffSent
    print pubnub.publish(channel=channel, message={"actuators": {"32": 255}})
    waterOffSent = True
    waterOnSent = False

def publishLightsOn(channel):
    time.sleep(2)
    global lightOnSent
    global lightOffSent
    print pubnub.publish(channel=channel, message={"actuators": {"30": 255, "31": 255}})
    lightOnSent = True
    lightOffSent = False

def publishLightsOff(channel):
    time.sleep(2)
    global lightOnSent
    global lightOffSent
    print pubnub.publish(channel=channel, message={"actuators": {"30": 0, "31": 0}})
    lightOnSent = False
    lightOffSent = True

def error(message):
    print("ERROR : " + str(message))

def connect(message):
    print("CONNECTED")
    print pubnub.publish(channel='anubhav', message='Hello from the PubNub Python SDK')

def reconnect(message):
    print("RECONNECTED")

def disconnect(message):
    print("DISCONNECTED")

pubnub.subscribe(channels='anubhav', callback=callback, error=callback, reconnect=reconnect, disconnect=disconnect)