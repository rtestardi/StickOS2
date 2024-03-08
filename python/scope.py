import sys
import serial

# COM17 <qus>

ser = None
line = None
cal_zero_x1 = 0
cal_3v3_x1 = 0
cal_zero_x10 = 0
cal_3v3_x10 = 0

def eat():
	global ser, lastline
	eaten = ''
	while True:
		bytes = ser.readline()
		if bytes == b'':
			break
		line = bytes.decode('utf-8').strip()
		print(line)
		if (line != 'done'):
			lastline = line

def connect():
	global ser, lastline
	import serial
	ser = serial.Serial(sys.argv[1], timeout=.1)
	ser.write(b'\x03'); eat()
	ser.write(b'echo off\r'); eat()
	ser.write(b'prompt off\r'); eat()
	ser.write(b'dim unused[8] as flash\r'); eat()
	ser.write(b'dim cal_zero_x1 as flash, cal_3v3_x1 as flash\r'); eat()
	ser.write(b'dim cal_zero_x10 as flash, cal_3v3_x10 as flash\r'); eat()
	ser.write(b'print cal_zero_x1, cal_3v3_x1, cal_zero_x10, cal_3v3_x10\r'); eat()
	list = lastline.split(' ')
	print(list)
	cal_zero_x1 = int(list[0])
	cal_3v3_x1 = int(list[1])
	cal_zero_x10 = int(list[2])
	cal_3v3_x10 = int(list[3])
	print('cal_zero_x1 =', cal_zero_x1, 'cal_3v3_x1 =', cal_3v3_x1, 'cal_zero_x10 =', cal_zero_x10, 'cal_3v3_x10 =', cal_3v3_x10)

connect()

# XXX -- use calibration values for both trigger level and results!

line = 'scope ' + sys.argv[2] + ' ~512\r'
print(line)
bytes = line.encode('utf-8')
ser.write(bytes)
ser.timeout = None

while True:
	bytes = ser.readline()
	line = bytes.decode('utf-8').strip()
	if line == 'done':
		break
	print(line)
