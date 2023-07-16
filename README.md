Exercise for learning MQTT setup:
Raspberry Pi accesspoint and MQTT broker, 
Raspberry Pico_W read sensor values and send to MQTT broker.
Third party script to read the sensor data.

---

##Set up an Access Point (AP) on the Raspberry Pi (RPI):

First use raspi-config to set wifi locale settings.

Update you rystem:
```
sudo apt-get update
sudo apt-get upgrade
```

Install necessary packages:
```
sudo apt-get install dnsmasq hostapd
```

Add to /etc/dhcpcd.conf
```
sudo nano /etc/dhcpcd.conf

interface wlan0
    static ip_address=192.168.4.1/24
    nohook wpa_supplicant
```

Restart the "dhcpd" deamon, to start the new "wlan0" config
```
sudo service dhcpcd restart
```

Backup "dnsmasq" config, and make a new.
```
sudo mv /etc/dnsmasq.conf /etc/dnsmasq.conf.orig  
sudo nano /etc/dnsmasq.conf
```

Add following lines
```
interface=wlan0
dhcp-range=192.168.4.2,192.168.4.20,255.255.255.0,24h
```

Add lines to hostapd file, setup your SSID, and Password!!
```
sudo nano /etc/hostapd/hostapd.conf

interface=wlan0
driver=nl80211
ssid=YourNetworkSSID
hw_mode=g
channel=7
wmm_enabled=0
macaddr_acl=0
auth_algs=1
ignore_broadcast_ssid=0
wpa=2
wpa_passphrase=YourPassword
wpa_key_mgmt=WPA-PSK
wpa_pairwise=TKIP
rsn_pairwise=CCMP
```

Add config path to deamon
```
sudo nano /etc/default/hostapd

DAEMON_CONF="/etc/hostapd/hostapd.conf"
```

Unmask and start hostapd
```
sudo systemctl unmask hostapd
sudo systemctl enable hostapd
```

Edit sysctl, uncomment "#net.ipv4.ip_forward=1"
```
sudo nano /etc/sysctl.conf

net.ipv4.ip_forward=1
```

Add masquerade for outbound traffic on eth0
```
sudo iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE

sudo sh -c "iptables-save > /etc/iptables.ipv4.nat"
```

Load rule on boot, add line before "exit 0"
```
sudo nano /etc/rc.local

iptables-restore < /etc/iptables.ipv4.nat
```

Reboot system
```
sudo reboot
```

The AP should now be running on the Raspberry Pi, sharing internet from ethernet to connected devices.

---

##Install an MQTT Broker on the RPI:
The MQTT broker is the "middleman" that routes messages between MQTT clients. 
Lets use Mosquitto, a popular open-source MQTT broker.

```
sudo apt update
sudo apt install -y mosquitto mosquitto-clients
```

Start mosquitto at boot
```
sudo systemctl enable mosquitto.service
```

Test mosquitto
```
mosquitto_sub -h localhost -t "test"
```

With a new shell run
```
mosquitto_pub -h localhost -t "test" -m "Hello, MQTT"
```
"Hello, MQTT" should appear in your first shell
For listening on network with ipv4 you may need to add lines to "/etc/mosquitto/mosquitto.conf" - "listener 1883" and "allow_anonymous true"


```
sudo netstat -tuln

```
Should return a line with: "0.0.0.0:1883"



---


##Connect the Raspberry Pi Pico_W to the RPI's WiFi:
To do this, you'll need to configure the Raspberry Pi Pico to connect to the WiFi network provided by the RPI's access point.

Micropython script for Pico_W
```
import network
import socket
from time import sleep
from picozero import pico_temp_sensor, pico_led
import machine

ssid = "Your_SSID"
password = "Your_Password"

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

try:
    ip = connect()
except KeyboardInterrupt:
    machine.reset()
```

This should be able to connect to your AP
```
Output:
Waiting for connection...
Waiting for connection...
Connected on 192.168.4.4
```

---

##Install an MQTT client on the Raspberry Pi Pico:
The Raspberry Pi Pico will need an MQTT client to send and receive MQTT messages.

Lets use umqtt.simple library, and test the connection
```
import network
import socket
from time import sleep
from picozero import pico_temp_sensor, pico_led
import machine
from umqtt.simple import MQTTClient

ssid = "Your_SSID"
password = "Your_Password"

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
    client = MQTTClient("pico_w", "ip_addr")
    client.connect()
    client.publish("test", "Hello, MQTT")  # "test" is the topic and "Hello, MQTT" is the message.
    client.disconnect()

def main():
    connect()
    mqtt()
    
main()
```

Message should appear in the shell subbed to "test" topic


Send ADC values to the MQTT broker
```
import network
import socket
from time import sleep
from picozero import pico_temp_sensor, pico_led
import machine
from umqtt.simple import MQTTClient

ssid = "Your_SSID"
password = "Your_Password"

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
    client = MQTTClient("pico_w", "broker_host_adress")
    client.connect()
    
    # client.publish("test", "Hello, world!")  # "Hello" is the topic and "world" is the message.
    
    adc1_value = adc1.read_u16()
    adc2_value = adc2.read_u16()
    
    client.publish("Analog1", str(adc1_value))
    client.publish("Analog2", str(adc2_value))
    
    
    client.disconnect()

def main():
    connect()
    
    while True:
        mqtt()
        sleep(1)
    
    
main()

```

And make a script to recieve the values from the broker
```
pip install paho-mqtt
```
```
import paho.mqtt.client as mqtt

def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))
    client.subscribe("Analog1")
    client.subscribe("Analog2")

def on_message(client, userdata, msg):
    print(msg.topic + " " + str(msg.payload))

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect("mqtt_broker_host", 1883, 60)

client.loop_forever()
```

This should now print the ADC data from the Pico_W.

