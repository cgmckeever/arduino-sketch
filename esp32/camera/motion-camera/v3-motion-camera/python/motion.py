import json
from time import time, sleep

import numpy as np
from PIL import Image
from serial import Serial, SerialException

def serial_to_file(port=None, baudrate=115200):
    """Read serial and generate files required for the visualization frontend"""
    SHAPE = (48, 64) #(24, 32)
    with Serial(port, baudrate, timeout=3) as ser:
        changed = []
        while True:
            try:
                line = ser.readline().decode('utf-8').strip()
                # raw image
                if line == 'Current frame:':
                    pixels = []
                    # read each line of pixels
                    for i in range(SHAPE[0]):
                        l = ser.readline().decode('utf-8').strip()
                        l = [int(x) for x in l.split('\t')]
                        pixels.append(l)
                    mat = np.array(pixels).astype(np.uint8).reshape(SHAPE)
                    image = Image.fromarray(mat)
                    # save to disk to update frontend
                    image.save('capture.jpg')
                # different pixels
                if line.startswith('diff'):
                    # each line has a pair (x, y) for changed tile
                    y, x = line.split('\t')[1:3]
                    changed.append((int(y), int(x)))
                if line.startswith('======'):
                    # end of different pixels
                    with open('diff.json', 'w') as out:
                        json.dump(changed, out)
                    changed = []
            except (SerialException, KeyboardInterrupt):
                break


if __name__ == '__main__':
    serial_to_file(port='/dev/tty.usbserial-AD0K15H4')