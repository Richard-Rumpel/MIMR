#!/usr/bin/python

import socket
import sys
from time import sleep
from serial import Serial
import struct

print "Start Bassfahrer"

'''init serial and network'''
# open serial port to Arduino
serial = Serial( "/dev/ttyACM0", 115200, bytesize=8, parity='N', timeout=0.01 )


# open UDP socket to listen raveloxmidi
udpIn = socket.socket( socket.AF_INET, socket.SOCK_DGRAM)
udpIn.bind( ('', 5010 ) )
udpIn.settimeout(0.01)

# decrease receive buffer size
udpIn.setsockopt( socket.SOL_SOCKET, socket.SO_RCVBUF, 128 )

# open UDP socket to send raveloxmidi
udpOut = socket.socket( socket.AF_INET, socket.SOCK_DGRAM )
udpOut.connect( ( "localhost", 5006 ) )


# initialize old value for change detection
oldvalue = 0

# midi values
midiMode = 0
midiHue = 0
midiSat = 0
midiVal = 0


# loop infinitely
while True:

	# incoming UDP packets in buffer?
	bufferClear = False

	# UDP data as string
	data = chr(0)

	# as long as packets in buffer...
	# (empty buffer, only forward latest packet)
	if not bufferClear:
		try:
			# read UDP packet from socket
			data, addr = udpIn.recvfrom(256)
		except Exception:
			# no more packets to read
			bufferClear = True
			pass

	# control command
	if ord(data[0]) == 176:
		#set mode
		if ord(data[1]) == 14:
			if ord(data[2]) <= 2:
				midiMode = ord(data[2])
			else:
				midiMode = 0
		#set hue
		elif ord(data[1]) == 15:
			midiHue = min(2*ord(data[2]),254)
		#set saturation
		elif ord(data[1]) == 16:
			midiSat = min(2*ord(data[2]),254)
		#set brightness
		elif ord(data[1]) == 17:
			midiVal = min(2*ord(data[2]),254)

		elements = [255,midiMode,midiHue,midiSat,midiVal]
		print elements

		for x in elements:
			serial.write(chr(x))

	safe = True

	while safe:

		try:
			# try to read line from Arduino
			line = serial.readline()

			# got complete line with expected start and end character?
			if line[:1] == ":" and line[-1:] == "\n":

				# get numeric value
				value = int( line[1:-1] )

				# read was successfull
				safe = True

			else:
				safe = False

		except Exception:
			safe = False

		# value available and different from old one?
		if safe and value != oldvalue:

			# scale value from 39..68
			value = (value-39) * 127 / (68-39)

			# limit to 0..127
			value = max( min( int(value), 127 ), 0 )

			# update cached value
			oldvalue = value

			# log current value
			print value

			# on MIDI channel 1, set controller #1 to value
			bytes = struct.pack( "BBBB", 0xaa, 0xB1, 0, value )
			udpOut.send( bytes )
