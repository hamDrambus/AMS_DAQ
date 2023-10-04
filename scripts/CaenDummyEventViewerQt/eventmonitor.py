"""
 This is simple PyQt application which reciceves and plots
 binary data generated with CaenDummyModule DAQling module
 in real time.
 
"""

from pathlib import Path as path
from os import environ as env
from PyQt5 import QtWidgets, QtCore, uic, QtGui
from PyQt5.QtWidgets import *
import pyqtgraph as pg
import numpy as np
import zmq
import json

## Switch to using white background and black foreground
#pg.setConfigOption('background', 'w')
#pg.setConfigOption('foreground', 'k')

if __name__ == '__main__':
    from sys import argv, exit
    if len(argv) <= 1:
        fname = path("scripts/CaenDummyEventViewerQt/monitor-config.json")
    else:
        fname = path(argv[1])

    with open(fname) as f:
        config = json.load(f)

    try:
        refresh_rate = config["refresh_rate_ms"]
        configs_zmq = []
        # TODO: separate function
        for (i, r) in enumerate(config["connections"]["receivers"]):
            try:
                # TODO: factory
                if r["type"] == "ZMQPubSub":
                    chid = int(r["chid"])
                    for (j, c) in enumerate(r["connections"]):
                        if c["transport"] == "icp":
                            connStr = "ipc://" + c["path"]
                        elif c["transport"] == "tcp":
                            connStr = "tcp://" + str(c["host"]) + ":" + str(c["port"])
                        elif c["transport"] == "inproc":
                            connStr = "inproc://" + c["endpoint"]
                        else:
                            raise ValueError("Invalid trasnport in ZMQPubSub connection #" + j + " for chid "+ str(chid))
                        # No filter, so socket option (sockopt) zmq.SUBSCRIBE has "" value
                        configs_zmq.append({
                             "chid": chid,
                             "connStr": connStr,
                             "socktype": zmq.SUB,
                             "sockopt": [(zmq.SUBSCRIBE, "")]
                             })
                if r["type"] == "ZMQPair":
                    chid = int(r["chid"])
                    if r["transport"] == "icp":
                        connStr = "ipc://" + r["path"]
                    elif r["transport"] == "tcp":
                        connStr = "tcp://" + str(r["host"]) + ":" + str(r["port"])
                    elif r["transport"] == "inproc":
                        connStr = "inproc://" + r["endpoint"]
                    else:
                        raise ValueError("Invalid trasnport in ZMQPair connection for chid "+ str(chid))
                    # No filter, so socket option (sockopt) zmq.SUBSCRIBE has "" value
                    configs_zmq.append({
                             "chid": chid,
                             "connStr": connStr,
                             "socktype": zmq.PAIR,
                             "sockopt": [(zmq.SUBSCRIBE, "")]
                             })
            except Exception as e:
                print(repr(e), flush=True)
                print("Warning: Reciever #" + i + " has invalid configureation", flush=True)
                continue

    except Exception as e: 
        print(repr(e), flush=True)
        print("Connection (ZMQ) settings are invalid!", flush=True)
        quit()

    sockets = []
    context = zmq.Context()
    for zmq_c in configs_zmq:
        socket = context.socket(zmq_c["socktype"])
        print("Connecting socket to " + zmq_c["connStr"])
        socket.connect(zmq_c["connStr"])
        for (opt, val) in zmq_c["sockopt"]:
            socket.setsockopt_string(opt, val)
        sockets.append((zmq_c["chid"], socket))

    if len(sockets) == 0:
        print("No sockets were connected, terminating program")
        quit()
    
    try:
        while True:
            # For testing, only single socket (thread) is used
            data = sockets[0][1].recv()
            print("Recieved message with size " + str(len(data)) + " bytes")

    except KeyboardInterrupt: # e.g. Ctrl+C
        for (chid, socket) in sockets:
            socket.close()
    except Exception as error:
        print("ERROR: {}".format(error))
        for (chid, socket) in sockets:
            socket.close()
        

#    app = QApplication(argv)
#    win = MainWindow()
#    win.show()
#    exit(app.exec_())



