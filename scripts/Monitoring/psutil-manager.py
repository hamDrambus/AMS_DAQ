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

import psutil
import time
import zmq

context = zmq.Context()

# bind the PIDs socket
socket = context.socket(zmq.PAIR)
socket.bind("tcp://*:6100")
name_pids = socket.recv_pyobj()

# connect the metrics publisher socket
socket = context.socket(zmq.PUB)
socket.connect("tcp://localhost:6007")

daqling_procs = {name: psutil.Process(pid) for name, pid in name_pids.items()}

while True:
  try:
    for name, p in daqling_procs.items():
      with p.oneshot():
        # print(p.cmdline(), flush=True)
        time_point = str(time.time()).replace('.','').ljust(19,'0')
        msg = name+'-'+'cpu_percent value='+str(p.cpu_percent())+' '+time_point+'\n'+name+'-'+'memory_percent value='+str(p.memory_percent())+' '+time_point+'\n'
        socket.send_string(msg)
    time.sleep(1)
  except Exception as e:
    print(e, "Removing", name, flush=True)
    daqling_procs.pop(name)
