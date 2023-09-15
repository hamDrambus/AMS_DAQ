"""
 Copyright (C) 2019-2021 CERN
 
 DAQling is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 DAQling is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public License
 along with DAQling. If not, see <http://www.gnu.org/licenses/>.
"""

import sys
import zmq

context = zmq.Context()
socket = context.socket(zmq.SUB)
# socket.setsockopt(zmq.SUBSCRIBE, (5561).to_bytes(4, byteorder='little'))
socket.setsockopt(zmq.SUBSCRIBE, b'')
socket.bind("tcp://*:6542")

while (1):
  msgs = socket.recv_multipart()
  print("["+str(msgs[0], 'utf-8')+"]", str(msgs[1], 'utf-8'))