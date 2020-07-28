"""
27 JUL 2020
cli.py
Nathan Zimmerberg (nhz2@cornell.edu)
Command line interface for reading the BNK chip
"""
import time
import serial
from serial.tools import list_ports
import numpy


def findteensy():
    """returns the port name to connect to a teensy over usb, returns None if no teensy is found."""
    port_list=serial.tools.list_ports.comports()
    for port in port_list:
        if port.manufacturer=='Teensyduino':
            return port.device

def nop(ser):
    ser.reset_input_buffer()
    ser.write(b'a\n')
    #time.sleep(0.1)# this prevents python from busy waiting.
    ser.readline()

def dac_set_voltage(ser,voltage):
    ser.reset_input_buffer()
    ser.write(b'd%f\n'%(voltage))
    #time.sleep(0.1)# this prevents python from busy waiting.
    ser.readline()

def recording_start(ser,framerate,framechunks,auxchannel,range,userdata0,userdata1):
    ser.reset_input_buffer()
    print(b'r%d,%d,%d,%d,%d,%d\n'%(framerate,framechunks,auxchannel,range,userdata0,userdata1))
    ser.write(b'r%d,%d,%d,%d,%d,%d\n'%(framerate,framechunks,auxchannel,range,userdata0,userdata1))
    line = ser.readline().strip()   # read a '\n' terminated line
    ser.readline()
    return float(line)

def status(ser):
    ser.reset_input_buffer()
    ser.write(b's\n')
    recording = int(ser.read_until(b',').strip(b','))
    framechunks = int(ser.read_until(b',').strip(b','))
    frameskipped = int(ser.read_until(b',').strip(b','))
    framedata= ser.read(256)
    ser.readline()
    ser.readline()
    return (recording,framechunks,frameskipped,framedata)

def eject_card(ser):
    ser.reset_input_buffer()
    ser.write(b'e\n')
    ser.readline()

def frame_chunk_readout(ser,chunkid):
    ser.write(b'f%d\n'%(chunkid))
    chunkdata= ser.read(256*32)
    er.readline()
    ser.readline()
    return chunkdata

def testskippedframes():
    portname=findteensy()
    if (not portname):
        print('no teensy found')
        return
    with serial.Serial(portname, 4000000, timeout=10) as ser:
        framerates=[40000,20000,10000,8000,4000,2000,1000]
        frameskips=[]
        for framerate in framerates:
            #100s recording
            chunklen= int(framerate*100/32)
            frameskip=0
            for i in range(36):
                recording_start(ser,framerate,chunklen,2,1,0,0)
                time.sleep(10)
                statusmessage= status(ser)
                while(statusmessage[0]==1):
                    #print("checking status again")
                    #print(statusmessage)
                    time.sleep(10)
                    statusmessage= status(ser)
                frameskip+= status(ser)[2]
                print(frameskip)
            frameskips.append(frameskip)
    print(frameskips)
    print(framerates)
    return frameskips

def decodeframe(framedatas):
    """ Returns the decoded frame info.
    Specifically a tuple of doubles
        a numpy array of frame numbers, 
        a numpy array of the voltages,
        a numpy array of the aux channel,
    framedatas is of type bytes of size a multiple of 256
    """
    




if __name__ == '__main__':
    testskippedframes()