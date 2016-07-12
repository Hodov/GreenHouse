import serial
import json
import requests
import socket
import time

#ser = serial.Serial('/dev/cu.usbmodem621',9600)
ser = serial.Serial('/dev/cu.usbmodem411',9600)
url = 'https://hooks.slack.com/services/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'

graphiteURL="95.213.252.56"
graphitePort=2003

while 1:
	t = ser.readline()
	print(t)
	payload={'text':t}
	try: 
		r = requests.post(url,data=json.dumps(payload))
	except requests.exceptions.ConnectionError:
		print("Connection error")
	s = ""
	i = 0
	while t[i]!=';' and len(t) >= i:
		s = s + t[i]
		i = i + 1
	sensor = s

	s = ""
	i = i + 1	
	while t[i]!=';' and len(t) >= i:
		s = s + t[i]
		i = i + 1
	key = s
	
	s = ""
	i = i + 1
	while t[i]!=';' and len(t) >= i:
		s = s + t[i]
		i = i + 1
	value = s
	
	try:
		conn = socket.create_connection((graphiteURL, graphitePort))
		timeString = str(int(time.time()))
		sendString = "ABROffice." + sensor + "." + key + " " + value + " " + timeString + "\n"
		conn.send(sendString)
		conn.close()
	except socket.error:
		print("Socket error")


