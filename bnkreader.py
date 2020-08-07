"""
27 JUL 2020
bnkreader.py
Nathan Zimmerberg (nhz2@cornell.edu)
Helper functions for reading the BNK chip
"""
import time
import serial
from serial.tools import list_ports
import numpy as np
import binascii


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
    if(ser.readline().strip()!=b'a'):
        raise IOError("ack failed")


def dac_set_voltage(ser,voltage):
    ser.reset_input_buffer()
    ser.write(b'd%f\n'%(voltage))
    #time.sleep(0.1)# this prevents python from busy waiting.
    if(ser.readline().strip()!=b'a'):
        raise IOError("ack failed")


def recording_start(ser,framerate,framechunks,auxchannel,adcrange,userdata0,userdata1):
    ser.reset_input_buffer()
    print(b'r%d,%d,%d,%d,%d,%d\n'%(framerate,framechunks,auxchannel,adcrange,userdata0,userdata1))
    ser.write(b'r%d,%d,%d,%d,%d,%d\n'%(framerate,framechunks,auxchannel,adcrange,userdata0,userdata1))
    line = ser.readline().strip()   # read a '\n' terminated line
    if(ser.readline().strip()!=b'a'):
        raise IOError("ack failed")
    return float(line)

def status(ser):
    ser.reset_input_buffer()
    ser.write(b's\n')
    recording = int(ser.read_until(b',').strip(b','))
    framechunks = int(ser.read_until(b',').strip(b','))
    frameskipped = int(ser.read_until(b',').strip(b','))
    framedata= ser.read(256)
    ser.readline()
    if(ser.readline().strip()!=b'a'):
        raise IOError("ack failed")
    return (recording,framechunks,frameskipped,framedata)

def eject_card(ser):
    ser.reset_input_buffer()
    ser.write(b'e\n')
    if(ser.readline().strip()!=b'a'):
        raise IOError("ack failed")

def frame_chunk_readout(ser,chunkid):
    ser.write(b'f%d\n'%(chunkid))
    chunkdata= ser.read(256*32)
    ser.readline()
    if(ser.readline().strip()!=b'a'):
        raise IOError("ack failed")
    return chunkdata

def testskippedframes():
    portname=findteensy()
    if (not portname):
        print('no teensy found')
        return
    with serial.Serial(portname, 4000000, timeout=10) as ser:
        framerates=[40000,20000,10000]
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
    Specifically a tuple of
        a numpy array of frame numbers, 
            this will show if any frame was skipped because the sd card was too slow
        a numpy array of the voltages in volts, size (rows,adcs*2,numframes)
        a numpy array of the aux channel, size (rows,numframes)
        a float of the max range in volts,
        a numpy array of userdata0,
        a numpy array of userdata1,
    framedatas is of type bytes of size a multiple of 256
    """
    framesize=256;
    rows= 10
    adcs= 6
    numframes= len(framedatas)//framesize
    #put the data into a numpy array
    data= np.frombuffer(framedatas,dtype='<u4')
    framenums= np.copy(data[0::64])
    userdata0s = np.copy(data[61::64])
    userdata1s = np.copy(data[62::64])
    crcs= np.copy(data[63::64])
    #delete extra data
    adcdata= np.delete(data,np.s_[63::64])
    adcdata= np.delete(adcdata,np.s_[62::63])
    adcdata= np.delete(adcdata,np.s_[61::62])
    adcdata= np.delete(adcdata,np.s_[0::61])
    for i in range(numframes):
        framedata= framedatas[i*framesize:(i+1)*framesize]
        #checksum
        if(binascii.crc32(framedata[:-4]) != crcs[i]):
            raise ValueError("crc mismatch")
    leadingzeroH= (adcdata>>31)&1
    leadingzeroL= (adcdata>>15)&1
    rangeH= (adcdata>>30)&1
    rangeL= (adcdata>>14)&1
    channelH = (adcdata>>29)&1
    channelL = (adcdata>>13)&1
    BboolH = (adcdata>>28)&1
    BboolL = (adcdata>>12)&1
    dataH = (adcdata>>16)&0xFFF
    dataL = (adcdata)&0xFFF
    #validity checks
    if (not all(leadingzeroH==0)):
        raise ValueError("leading zero high error")
    if (not all(leadingzeroL==0)):
        raise ValueError("leading zero low error")
    if (not all(rangeH==rangeL)):
        raise ValueError("range high and low unequal")
    if (not all(rangeH==rangeH[0])):
        raise ValueError("range changes during recording")
    if (not all(channelH==channelL)):
        raise ValueError("channel high and low not equal")
    if (not all(BboolH==BboolH[0])):
        raise ValueError("BboolH changes")
    if (not all(BboolL==BboolL[0])):
        raise ValueError("BboolL changes")
    if (BboolL[0]==BboolH[0]):
        raise ValueError("Bbool high and low equal")
    #convert to float
    if(rangeH[0]==0):
        #0v to Vref, straight binary
        vH= dataH*(2.5/4096.0)
        vL= dataL*(2.5/4096.0)
        maxvoltage= 0xFFF*(2.5/4096.0)
    else:
        #2s complement Vref+-Vref
        vH = (dataH ^ (1<<11))*(5.0/4096.0)
        vL = (dataL ^ (1<<11))*(5.0/4096.0)
        maxvoltage= 0xFFF*(5.0/4096.0)
    if(BboolH[0]==1):
        voltageB= vH
        voltageA= vL
    else:
        voltageA= vH
        voltageB= vL
    channels= channelH[adcs-1::adcs]
    channels= np.reshape(channels,(numframes,rows))
    channels= np.transpose(channels)
    #reshape voltages to build a 3d array
    volts= np.zeros((rows,adcs*2,numframes))
    voltageB= np.reshape(voltageB,(numframes,rows,adcs))
    voltageA= np.reshape(voltageA,(numframes,rows,adcs))
    for row in range(rows):
        for adc in range(adcs):
            volts[row,adc*2]= voltageB[:,row,adc]
            volts[row,adc*2+1]= voltageA[:,row,adc]
    return (framenums,volts,channels+1,maxvoltage,userdata0s,userdata1s)

if __name__ == '__main__':
    testskippedframes()
