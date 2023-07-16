import network
import socket
from time import sleep
from picozero import pico_temp_sensor, pico_led
import machine
from umqtt.simple import MQTTClient

ssid = "RPI-SD"
password = "seadrive"

adc1 = machine.ADC(26)
adc2 = machine.ADC(27)


def connect():
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    wlan.connect(ssid, password)
    while wlan.isconnected() == False:
        print('Waiting for connection...')
        sleep(1)
    ip = wlan.ifconfig()[0]
    print(f'Connected on {ip}')
    return ip

def mqtt():
    client = MQTTClient("pico_w", "SD.local")
    client.connect()
    
    # client.publish("test", "Hello, world!")  # "Hello" is the topic and "world" is the message.
    
    adc1_value = adc1.read_u16()
    adc2_value = adc2.read_u16()
    
    client.publish("A1", str(adc1_value))
    client.publish("A2", str(adc2_value))
    
    
    client.disconnect()

def main():
    connect()
    
    while True:
        mqtt()
        sleep(1)
    
    
main()
