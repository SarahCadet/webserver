#!/usr/bin/python3

import socket

# Create a Unix domain socket
sock = socket.socket(socket.AF_UNIX)
sock.bind('./testDir/my_socket3.sock')  # Specify the desired socket file name

# Now you have a socket file named 'my_socket.sock' in the current directory
while(True):
    print("waiting")
sock.shutdown(socket.SHUT_RDWR)
sock.close()