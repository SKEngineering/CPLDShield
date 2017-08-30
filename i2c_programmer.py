import serial
import sys
import argparse
import crc16

ser = 0
operations = b'2'
erase = 1
program = 2
verify = 3


def err(msg):
    print "err: %s" % msg
    sys.exit(1)


def read_jedec(jedec):
    """
    Read and parse jedec.
    Note that feature row and bits are reversed compared to config data.
    """
    UFM = []
    noUFM = []

    programUFM = False

    f = open(args.jedec, 'r').read()
    f = f.split("\r\n")

    for idx, line in enumerate(f):
        if "NOTE TAG DATA*" in line:
            programUFM = True

        elif line.startswith("E"):
            row = line[:0:-1]
            bits = f[idx + 1][-2::-1]

        elif line.startswith("UH"):
            user = line[2:-1]

        elif programUFM and len(line) == 128:
            UFM.append(line)

        elif not programUFM and len(line) == 128:
            noUFM.append(line)

    return UFM, noUFM, user, row, bits


def bitstring_to_bytes(s, bytes):
    """
    Convert ascii binary string to byte array.
    """
    v = int(s, 2)
    b = bytearray()
    for i in range(bytes):
        b.append(v & 0xff)
        v >>= 8
    return b[::-1]

def add_crc16(bytelist):
    strlist = str(bytelist)
    crc = crc16.crc16xmodem(strlist)
    crcHex = hex(crc)[2:].replace("L","")
    paddedHex = crcHex.rjust(4,'0')
    bytelistcrc = paddedHex.decode('hex')
    return bytearray(bytelistcrc)[::-1] + bytelist

def add_header(bytelist):
    """
    Add header for array of data
    """
    tmp = bytelist[::-1]
    tmp.append(len(bytelist))
    return tmp[::-1]


def write_flash_pages(page):
    """
    Write 129 bits of data.
    First byte is header that specifies byte length[4:0] and if programming should stop[7]
    """
    if(page):
        ser.write("\x01")
        for idx, msg in enumerate(page):
            msgBytes = bitstring_to_bytes(msg, 16)
            msgBytes = add_crc16(msgBytes)
            msgBytes = add_header(msgBytes)

            if idx == (len(page) - 1):
                msgBytes[0] |= 0x80

            response = ser.readline()

            while(response == "REQUEST\r\n"):
                print list(msgBytes)
                ser.write(msgBytes)
                response = ser.readline()

            if response == "FAILED\r\n":
                err("Programming failed")
                

    else:
        ser.write("\x00")


def write_single_page(page, errMsg):
    """
    Write single page info to the device.
    Send bit indicating if it will be programmed.
    Send page twice, first for programming and then for confirmation.
    """
    if(page):
        ser.write('\x01')
        print(list(page))
        msg = add_crc16(page)
        response = ser.readline()

        while(response == "REQUEST\r\n"):
            print list(msg)
            ser.write(msg)
            response = ser.readline()

        if response != "ACK\r\n":
            err(errMsg)

        if int(operations) == verify:
            ser.write(msg)
            print ser.readline()

    else:
        ser.write('\x00')


def main(args):

    if int(operations) < 0 or int(operations) > 2:
        return

    UFM, noUFM, user, row, bits = read_jedec(args.jedec)

    print("waiting for arduino connection")


    ser.readline()
    print("got device connection")
    
    ser.write(operations)

    print ser.readline()  #receive device id

    if int(operations) == erase:
        return

    write_flash_pages(noUFM)

    write_flash_pages(UFM)

    write_single_page(user.decode('hex'), "Usercode programming failed")

    write_single_page(bitstring_to_bytes(row, 8), "feature row programming failed")

    write_single_page(bitstring_to_bytes(bits, 2), "feature bits programming failed")

    print ser.readline()

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Python programming interface for Lattice devices via Arduino', )

    parser.add_argument('port', help='Port used to communicate with Arduino', nargs = '?')
    parser.add_argument('jedec', help='Jedec what will be send to the device', nargs = '?')
    parser.add_argument('function', help='Possible options: [Erase, Program]', nargs = '?')

    parser.add_argument('--port', help='Port used to communicate with Arduino')
    parser.add_argument('--jedec', help='Jedec what will be send to the device')
    parser.add_argument('--function', help='Possible options: [Erase, Program]')
    args = parser.parse_args()

    if "erase" in args.function:
        operations = '1'
    elif "program" in args.function:
        operations = '2'

    ser = serial.Serial(args.port, 115200)

    
    main(args)
