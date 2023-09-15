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

import json

with open("config.stream-level.json") as f:
  data = json.load(f)
f.close()

# sorts the processes based on their stream_level
data.sort(key = lambda x: x["stream_level"])

data_port = 8001
connections = [{"sources": {}, "destinations": {}} for x in range(len(data))]
ch_count = [1 for x in range(len(data))]

for idx, p in enumerate(data):
  print(p)
  lvl = p['stream_level']
  for idx1, p1 in enumerate(data):
    if lvl == p1['stream_level'] - 1:
      dsts = len(connections[idx]["destinations"])
      print("dsts",dsts)
      connections[idx]["destinations"].update({str(dsts+1): {"chid": ch_count[idx], "host": p1['host'], "port": data_port}})
      ch_count[idx] = ch_count[idx] + 1

      srcs = len(connections[idx1]["sources"])
      print("srcs",srcs)
      connections[idx1]["sources"].update({str(srcs+1): {"chid": ch_count[idx1], "host": "*", "port": data_port}})
      ch_count[idx1] = ch_count[idx1] + 1

      data_port = data_port + 1
      
print("===================")
print(connections)


