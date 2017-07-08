from __future__ import print_function
import serial
import io
from PIL import Image
from matplotlib import pyplot as plt
import numpy as np

s = serial.Serial("/dev/ttyACM0", 115200)

n = 0
with open("serialcam.dat", "wb") as fh:
    j = i = None
    inside = False
    f = io.BytesIO()
    while True:
        i = j
        j = s.read(1)
        fh.write(j)
        if not inside and i == '\xFF' and j == '\xD8':
            inside = True
            n += 1
            print(n, "Inside")
            f.write(i)
        if inside and i == '\xFF' and j == '\xD9':
            print(n, "Done")
            f.write(j)
            f.seek(0)
            try:
                img = Image.open(f)
                plt.imshow(np.asarray(img))
                plt.show()
            except IOError:
                pass
            inside = False
            fname = "img/{}.jpg".format(n)
            with open(fname, "wb") as ofh:
                ofh.write(f.getvalue())
            f.close()
            f = io.BytesIO()
        if inside:
            f.write(j)
