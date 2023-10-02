"""
 This is simple PyQt application which displays text data
 obtained with CaenDummyModule DAQling module and written
 by CaenFileWriterModule. These are for demonstration and
 testing of transfer of arbitrary serializable classes over
 the DAQling instead of very simple hard-coded data.

 Launch from DAQling environment (source cmake/setup.sh) and
 from daqling root directory
"""

from pathlib import Path as path
from os import environ as env
from PyQt5 import QtWidgets, QtCore, uic, QtGui
from PyQt5.QtWidgets import *
import pyqtgraph as pg
import numpy as np

## Switch to using white background and black foreground
#pg.setConfigOption('background', 'w')
#pg.setConfigOption('foreground', 'k')

class MainWindow(QMainWindow):
    def __init__(self):
        super(MainWindow, self).__init__()
        uic.loadUi("scripts/CaenDummyEventViewerQt/EventViewer.ui", self)
        self.plot1 = self.graphicsView.addPlot(row=0, col=0)
        self.plot2 = self.graphicsView.addPlot(row=1, col=0)
        self.plot1.setXLink(self.plot2)
        self.lFileNumber.setReadOnly(True)
        self.lEventNumber.setReadOnly(True)
        self.bNextEvent.clicked.connect(self.next_event)
        self.showMaximized()

        self.data_folder = "/home/fvirt/Documents/AMS_DAQ/DAQling/daqling-develop/tmp/"
        # E.g. test-2023-09-25-13:06:29.0.0.0.txt or test-2023-09-25-13:06:54.0.0.6.txt
        self.file_pattern = "test-2023-10-02-[0-2][0-9]:[0-5][0-9]:[0-5][0-9].[0-9].[0-9].*.txt"
        
        try:
            self.current_filenum = int(self.lFileNumber.text())
        except ValueError:
            self.current_filenum = None
            self.statusbar.showMessage("Warning: Invalid file number '" + self.lFileNumber.text() + "'!")
        self.current_file = self.find_file(self.current_filenum)
        #print("current_file = '" + self.current_file.resolve().as_posix() if self.current_file is not None else "None" + "'")
        self.opened_file = None
        self.event_data = None
        self.next_event()

    def update_file_num(self):
        self.current_filenum = self.get_file_number(self.current_file)
        if self.current_filenum is None:
            self.lFileNumber.setText("None")
        else:
            self.lFileNumber.setText(str(self.current_filenum))
        if self.event_data is None or "event_number" not in self.event_data:
            self.lEventNumber.setText("None")
        else:
            self.lEventNumber.setText(str(self.event_data["event_number"]))


    def next_file(self, filelist, last_file):
        if self.opened_file is not None:
            self.opened_file.close()
            self.opened_file = None
        if filelist is None or len(filelist) == 0:
            self.statusbar.showMessage("Warning: No data files were found!")
            return None
        try:
            index = filelist.index(self.current_file)
            index += 1
            if index == len(filelist):
                index = 0
            if last_file == filelist[index]:
                self.statusbar.showMessage("Error: Could not open any of the files!")
                return None
            return filelist[index]            
        except ValueError:
            file = self.find_file(self.current_filenum+1)
            if file is None:
                return filelist[0]
            
    def open_current_file(self):
        if self.opened_file is None:
            try:
                self.opened_file = open(self.current_file, "r")
                return True
            except OSError:
                self.opened_file = None
                self.statusbar.showMessage("Error: Could not read file '" + self.current_file.resolve().as_posix() + "'! Going to the next one.")
                return False
        return True
        
    def next_event(self):
        last_file = self.current_file # To avoid endless loop.
        file_list = self.get_file_list()

        if self.current_file is None:
            self.current_file = self.next_file(file_list, last_file)
        
        while self.opened_file is None and self.current_file is not None:
            if not self.open_current_file():
               self.current_file = self.next_file(file_list, last_file)

        if self.current_file is None:
            self.update_file_num()
            return

        #print("Opened file '" + self.current_file.resolve().as_posix() + "'")
        line = self.opened_file.readline()
        if not line and last_file != self.current_file:
            self.statusbar.showMessage("Warning: Empty file '" + self.current_file.resolve().as_posix()+ "'! No new event.")
            self.update_file_num()
            self.current_file = self.next_file(file_list, last_file)
            return
        
        if not line: # All events in file have ended
            self.current_file = self.next_file(file_list, last_file)
            # self.next_event() # If file is empty it will skip it instead of printing a warning
            while self.opened_file is None and self.current_file is not None:
                if not self.open_current_file():
                    self.current_file = self.next_file(file_list, last_file)
            if self.current_file is None:
                self.update_file_num()
                return
            #print("Opened file '" + self.current_file.resolve().as_posix() + "'")
            line = self.opened_file.readline()

        # Read event from file and plot
        # TODO: to separate function
        self.plot1.clear()
        self.plot2.clear()
        try:
            self.event_data = {}
            self.event_data["event_number"] = int(line.rstrip())
            line = self.opened_file.readline()
            self.event_data["event_timestamp"] = int(line.rstrip())
            line = self.opened_file.readline()
            self.event_data["event_device"] = line
            self.event_data["data"] = {}
            while True:
                line = self.opened_file.readline()
                if not line.rstrip():
                    break
                channel = int(line.rstrip())
                line = self.opened_file.readline()
                xs = np.fromstring(line.rstrip(), dtype=np.int_, sep=" ")
                line = self.opened_file.readline()
                ys = np.fromstring(line.rstrip(), dtype=np.int_, sep=" ")
                self.event_data["data"][channel] = (xs, ys)            
            if 0 in self.event_data["data"]:
                self.plot1.plot(self.event_data["data"][0][0], self.event_data["data"][0][1])
            if 1 in self.event_data["data"]:
                self.plot2.plot(self.event_data["data"][1][0], self.event_data["data"][1][1])
            self.statusbar.showMessage("OK")
        except ValueError:
            self.statusbar.showMessage("Warning: invalid data in '" + self.current_file.resolve().as_posix() + "'!")
            self.event_data = None
        self.update_file_num()


    @staticmethod
    def get_file_number(file):
        res = file.name.split(".")
        if len(res) < 2:
            return None
        else:
            return int(res[-2])
        
    def find_file(self, filenum):
        if filenum is None:
            return self.get_file_list()[0]
        for f in self.get_file_list():
            if self.get_file_number(f) == filenum:
                return f
        return None

    # TODO: add file locking checks
    def get_file_list(self):
        return sorted(path(self.data_folder).glob(self.file_pattern), key=MainWindow.get_file_number)
    

if __name__ == '__main__':
    from sys import argv, exit

    app = QApplication(argv)
    win = MainWindow()
    win.show()
    exit(app.exec_())



