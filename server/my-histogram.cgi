#!/usr/bin/python3

import os
import sys
import stat
import math
import matplotlib.pyplot as plt
#TOdo: a check to see if 
amtreg = amtdir = amtsym = amtblk = amtsock = amtfifo = amtchar = 0
startingDir = sys.argv[1]
def traverse(directory):
    global amtreg, amtdir, amtsym, amtblk, amtsock, amtfifo, amtchar
    try:
        for filename in os.listdir(directory):
            if filename == "." or filename == "..":
                continue
            #open the file
            f = os.path.join(directory, filename)
            file_stat = os.lstat(f)
            # Checking file type using file mode
            if stat.S_ISREG(file_stat.st_mode):
                amtreg += 1
                #maybe this should be changed to contains /cgiOutput.html cuz it's not guaranteed
                if(filename == "cgiOutput.html"):
                    amtreg -= 1
                    continue
            elif stat.S_ISDIR(file_stat.st_mode):
                amtdir += 1
                if os.access(f, os.X_OK):
                    traverse(f)
            elif stat.S_ISLNK(file_stat.st_mode):
                amtsym += 1
            elif stat.S_ISFIFO(file_stat.st_mode):
                amtfifo += 1
            elif stat.S_ISSOCK(file_stat.st_mode):
                amtsock += 1
            elif stat.S_ISBLK(file_stat.st_mode):
                amtblk += 1
            elif stat.S_ISCHR(file_stat.st_mode):
                amtchar += 1
    except PermissionError:
        print(f"Permission denied: {directory}")
        return
# Create histogram
try:
    traverse(startingDir)
except PermissionError:
    print(f"Permission denied: {startingDir}")

data = [amtreg, amtdir, amtsym, amtblk, amtsock, amtfifo, amtchar]
categories = ["regular", "directory", "symlink", "block", "socket", "fifo", "character"]
colors = ['red', 'blue', 'green', 'orange', 'grey', 'black', 'purple']
fig, freqbar = plt.subplots()
freqbar.bar(categories, data, color=colors) 
freqbar.set_xlabel('FileTypes')
freqbar.set_ylabel('Frequency')
freqbar.set_title('Histogram')
yint = range(math.floor(min(data)), math.ceil(max(data)) + 1)
plt.yticks(yint)
'''
print("Regular files:", amtreg)
print("Symbolic links:", amtsym)
print("Directories:", amtdir)
print("FIFOs:", amtfifo)
print("Sockets:", amtsock)
print("Block special files:", amtblk)
print("Character special files:", amtchar)'''
fig.savefig('histogram.jpeg')
print('<body style="text-align: center; color: red; background-color: white;">' + 
      '<p style="background-color: white; font-size: 16pt;">CS410 Webserver</p><br>' + 
      '<img src="histogram.jpeg" style="max-width: 600px"></body>')
#output the image