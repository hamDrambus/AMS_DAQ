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
        ui = uic.loadUi("scripts/CaenDummyEventViewerQt/EventViewer.ui", self)
        ui.plot1 = ui.graphicsView.addPlot(row=0, col=0)
        ui.plot2 = ui.graphicsView.addPlot(row=1, col=0)
        ui.plot1.setXLink(ui.plot2)
        ui.le_file_number.setReadOnly(True)
        ui.le_event_number.setReadOnly(True)
        ui.pb_next_event.clicked.connect(self.next_event)
        self.ui = ui
        self.showMaximized()

        self.data_folder = "/home/fvirt/Documents/AMS_DAQ/DAQling/daqling-develop/tmp/"
        # E.g. test-2023-09-25-13:06:29.0.0.0.txt or test-2023-09-25-13:06:54.0.0.6.txt
        self.file_pattern = "test-2023-10-10-[0-2][0-9]:[0-5][0-9]:[0-5][0-9].[0-9].[0-9].*.txt"
        
        try:
            self.current_filenum = int(self.ui.le_file_number.text())
        except ValueError:
            self.current_filenum = None
            self.ui.statusbar.showMessage("Warning: Invalid file number '" + self.ui.le_file_number.text() + "'!")
        self.current_file = self.find_file(self.current_filenum)
        #print("current_file = '" + self.current_file.resolve().as_posix() if self.current_file is not None else "None" + "'")
        self.current_file_content = None
        self.event_data = None
        self.next_event()

    def update_file_num(self):
        fn = self.current_filenum = self.get_file_number(self.current_file)
        if fn is None:
            self.ui.le_file_number.setText("None")
        else:
            self.ui.le_file_number.setText(str(fn))
        data = self.event_data
        if data is None or "event_number" not in data:
            self.ui.le_event_number.setText("None")
        else:
            self.ui.le_event_number.setText(str(data["event_number"]))


    def next_file(self, current_file, filelist, last_file):
        if filelist is None or len(filelist) == 0:
            self.ui.statusbar.showMessage("Warning: No data files were found!")
            return None
        try:
            index = filelist.index(current_file)
            index += 1
            if index == len(filelist):
                index = 0
            if last_file == filelist[index]:
                self.ui.statusbar.showMessage("Error: Could not open any of the files!")
                return None
            return filelist[index]            
        except ValueError:
            file = self.find_file(self.current_filenum+1)
            if file is None:
                return filelist[0]
            return file
        
    def next_event(self):
        current_file = self.current_file
        last_file = self.current_file # To avoid endless loop.
        file_list = self.get_file_list()

        if current_file is None or not self.current_file_content: 
            current_file = self.next_file(current_file, file_list, last_file)
            while current_file:
                with open(current_file, "r") as file:
                    self.current_file_content = file.readlines()
                    break
                self.ui.statusbar.showMessage("Error: Could not read file '" + current_file.resolve().as_posix() + "'! Going to the next one.")
                current_file = self.next_file(current_file, file_list, last_file)
        self.current_file = current_file

        if current_file is None:
            self.update_file_num()
            return

        self.event_data = self.read_event(self.current_file_content)
        self.plot_event(self.event_data)
        self.update_file_num()

    # Read event from file lines
    def read_event(self, file_content):
        event_data = {}
        try:
            line = file_content.pop(0)
            event_data["event_number"] = int(line.rstrip())
            line = file_content.pop(0)
            event_data["event_timestamp"] = int(line.rstrip())
            line = file_content.pop(0)
            event_data["event_device"] = line
            event_data["data"] = {}
            while True:
                line = file_content.pop(0)
                if not line.rstrip():
                    break
                channel = int(line.rstrip())
                line = file_content.pop(0)
                xs = np.fromstring(line.rstrip(), dtype=np.int_, sep=" ")
                line = file_content.pop(0)
                ys = np.fromstring(line.rstrip(), dtype=np.int_, sep=" ")
                event_data["data"][channel] = (xs, ys)
        except (ValueError, IndexError) as e:
            self.ui.statusbar.showMessage("Warning: invalid data in '" + self.current_file.resolve().as_posix() + "'!")
            event_data = None
        return event_data

    def plot_event(self, event_data):
        self.ui.plot1.clear()
        self.ui.plot2.clear()
        if not event_data:
            return
        try:
            if 0 in event_data["data"]:
                self.ui.plot1.plot(event_data["data"][0][0], event_data["data"][0][1])
            if 1 in event_data["data"]:
                self.ui.plot2.plot(event_data["data"][1][0], event_data["data"][1][1])
            self.ui.statusbar.showMessage("OK")
        except ValueError as e:
            self.ui.statusbar.showMessage("Warning: invalid data in '" + self.current_file.resolve().as_posix() + "'!")

    @staticmethod
    def get_file_number(file):
        res = file.name.split(".")
        if len(res) < 2:
            return None
        else:
            return int(res[-2])
        
    def find_file(self, filenum):
        filelist = self.get_file_list()
        if filenum is None:
            return filelist[0]
        for f in filelist:
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



