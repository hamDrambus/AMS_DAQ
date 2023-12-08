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

from supervisor_wrapper import supervisor_wrapper
import zmq
import json
# bson from pymogo is used, not from bson package!
import bson
import jsonref
from time import sleep
from xmlrpc.client import ServerProxy

## DAQ control class
#  Handles communication with Supervisor and the Command XML-RPC server of DAQling processes.
class daqcontrol:
  ## Constructor
  #  @param group The Supervisor registered group to which to add processes
  #  @use_supervisor If False doesn't rely on Supervisor to spawn and check the process status
  def __init__(self, group, use_supervisor=True):
    self.group = group
    self.use_supervisor = use_supervisor

  ## Get all names and process IDs
  #  Returns a map of process names and correspondent process IDs.
  def getAllNameProcessID(self, host):
    sw = supervisor_wrapper(host, self.group)
    return {info['name']: info['pid'] for info in sw.getAllProcessInfo()}

  ## Remove a single process
  def removeProcess(self, host, name):
    sw = supervisor_wrapper(host, self.group)
    sw.removeProcess(name)

  ## Removes multiple processes
  #  @param components The JSON array of components to remove
  #  @param extra_processes The additional names of processes to remove for each host
  def removeProcesses(self, components, extra_processes=[]):
    for p in components:
      self.removeProcess(p['host'], p['name'])
      for name in extra_processes:
        self.removeProcess(p['host'], name)

  ## Add a single process
  #  @param exe The executable to spawn, with relative path from dir
  #  @param dir The absolute path of the directory from which exe is evaluated
  #  @param lib_path The LD_LIBRARY_PATH to pass to the executable
  #  @param command The command to use to run a script (example: python3)
  def addProcess(self, host, name, exe, dir, lib_path="", command=""):
    sw = supervisor_wrapper(host, self.group)
    try:
      host_, log_file, settings = sw.addProgram(name, exe, dir, lib_path, command)
      # TODO: add logging to file for these scripts themselves
      print(f"Added program '{self.group}:{name}' to {host}:")
      print(f"  command: '{settings['command']}'")
      print(f"  directory: '{settings['directory']}'")
      # print(f"  environment: '{settings['environment']}'")
      print(f"  logfile: '{settings['stdout_logfile']}'")
      return (host_, log_file)
    except Exception as e:
      raise Exception(e, ": cannot add program", name)

  ## Boot a single process
  #  Executes a process if it has been already added.
  def bootProcess(self, host, name):
    sw = supervisor_wrapper(host, self.group)
    try:
      sw.startProcess(name)
    except Exception as e:
      raise Exception(e, ": cannot start program", name)

  ## Adds multiple daqling components (processes)
  #  @param components The JSON array of components to add
  def addComponents(self, components):
    log_files = []
    for p in components:
      name = p['name']
      port = p['port']
      loglvl_core = p['loglevel']['core']
      loglvl_module = p['loglevel']['module']
      loglvl_connection = p['loglevel']['connection']
      dir = '%(ENV_DAQ_BUILD_DIR)s'
      # The LD_LIBRARY_PATH to pass to the executable
      lib_path = 'LD_LIBRARY_PATH='+'%(ENV_LD_LIBRARY_PATH)s'+':'+dir+'/lib/,TDAQ_ERS_STREAM_LIBS=DaqlingStreams'
      full_exe = "bin/daqling --name "+name+" --port "+str(port)+" --core_lvl "+loglvl_core+" --module_lvl "+loglvl_module+" --connection_lvl "+loglvl_connection
      log_files.append(self.addProcess(p['host'], name, full_exe, dir, lib_path=lib_path))
    return log_files

  ## Adds multiple scripts (processes)
  #  @param scripts The JSON array of scripts to add
  def addScripts(self, scripts):
    log_files = []
    for s in scripts:
      name = s['name']
      command = s['command']
      exe = s['executable']
      # DAQ_SCRIPT_DIR environment variable is resolved on host machine, not here
      dir = '%(ENV_DAQ_SCRIPT_DIR)s'+s['directory']
      log_files.append(self.addProcess(s['host'], name, exe, dir, command=command))
    return log_files

  ## Handles XML-RPC requests
  def handleRequest(self, host, port, request, *args):
    proxy = ServerProxy("http://"+host+":"+str(port)+"/RPC2")
    return getattr(proxy, request)(*args)

  def configureProcess(self, p):
    req = 'configure'
    config = json.dumps(p)
    return bson.BSON(self.handleRequest(p['host'], p['port'], req, config).data).decode()

  def unconfigureProcess(self, p):
    req = 'unconfigure'
    return bson.BSON(self.handleRequest(p['host'], p['port'], req).data).decode()

  def startProcess(self, p, run_num=0):
    req = 'start'
    return bson.BSON(self.handleRequest(p['host'], p['port'], req, int(run_num)).data).decode()

  def stopProcess(self, p):
    req = 'stop'
    return bson.BSON(self.handleRequest(p['host'], p['port'], req).data).decode()

  def shutdownProcess(self, p):
    req = 'down'
    return bson.BSON(self.handleRequest(p['host'], p['port'], req).data).decode()

  def customCommandProcess(self, p, command, arg=None):
    req = 'custom'
    return bson.BSON(self.handleRequest(p['host'], p['port'], req, command, arg).data).decode()

  def getStatus(self, p):
      sw = supervisor_wrapper(p['host'], self.group)
      req = 'status'
      status = []
      module_names = []
      try:
        json_result = bson.BSON(self.handleRequest(p['host'], p['port'], req).data).decode()
        # 'status' key means here whether command was executed successfully.
        # The actual status is in 'response' to the command.
        if json_result['status'] != 'Success': # Command failed
          for mod in p['modules']:
            status.append('unknown')
            module_names.append(mod['name'])
        else:
          for key, val in json_result['modules'].items():
            module_names.append(key)
            if val['status'] != 'Success': # status command failed for this module
              status.append('unknown')  
            else:
              status.append(val['response'])
      except Exception as e:
        print("Exception during executing status command:")
        print(str(e))
        if self.use_supervisor:
          for mod in p['modules']:
            status.append('added')
            module_names.append(mod['name'])
          try:
            sw.getProcessState(p['name'])['statename']
          except:
            status=[]
            module_names = []
            for mod in p['modules']:
              status.append('not_added')
              module_names.append(mod['name'])
      return status, module_names


