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

from xmlrpc.client import ServerProxy
from supervisor.states import STOPPED_STATES
from supervisor.xmlrpc import RPCError, Faults
import datetime
import getpass
import threading
from time import sleep

supervisor_wrapper_lock = threading.Lock()

## Supervisor wrapper class
#  Wraps the XML-RPC client calls in order to control Twiddler and Supervisor.
class supervisor_wrapper:
  ## Constructor
  #  @param host The targeted host where the supervisor daemon is running
  #  @param group The registered group to which to add processes
  def __init__(self, host, group):
    self.group = group
    self.host = host
    self.server = ServerProxy('http://'+host+':9001/RPC2')

  def getAllProcessInfo(self):
    return [info for info in self.server.supervisor.getAllProcessInfo() if info['group'] == self.group]

  def startProcess(self, name):
    return self.server.supervisor.startProcess(self.group+":"+name)

  def stopProcess(self, name):
    return self.server.supervisor.stopProcess(self.group+":"+name)

  def getProcessState(self, name):
    return self.server.supervisor.getProcessInfo(self.group+":"+name)

  def hasProcess(self, name):
    states = self.getAllProcessInfo()
    return any(st['name'] == name for st in states)
  
  def isProcessStopped(self, name):
    status = self.server.supervisor.getProcessInfo(self.group+":"+name)
    # See supervisor documentation on state values
    return status['state'] in STOPPED_STATES
  
  def removeProcess(self, name):
    try:
      if self.hasProcess(name):
        if not self.isProcessStopped(name):
          try:
            self.stopProcess(name)
          except Exception as e:
            raise Exception(e, ": cannot stop process", name)
        self.server.twiddler.removeProcessFromGroup(self.group, name)
    except Exception as e:
      raise Exception(e, ": Couldn't get process state")
    return True
  
  ## Add program
  #  Adds a program, sets the log name with date and time.
  #  Execution of the added program is automatic.
  #  @param exe The executable to spawn, with relative path from dir
  #  @param dir The absolute path of the directory from which exe is evaluated
  #  @param env The environment variables to pass to the executable
  #  @param command The command to use to run a script (example: python3)
  def addProgram(self, name, exe, dir, env, command=""):
    now = datetime.datetime.now().strftime("%Y%m%d%H%M%S")
    user = getpass.getuser()
    log_file = '%(ENV_DAQLING_LOG_DIR)s'+name+'-'+user+'-'+now+'.log'
    settings = {
        'command': command+' '+dir+exe,
        'directory': dir,
        'autorestart': 'false',
        'startretries': '0',
        'environment': env,
        'user': user,
        'stdout_logfile': log_file,
        'stdout_logfile_maxbytes': '0',
        'stdout_logfile_backups': '0',
        'stderr_logfile': log_file
    }
    self.server.twiddler.addProgramToGroup(self.group, name, settings)
    # Resolve environment variables in settings
    environment = {'ENV_' + key: val[1:-1] if val.endswith('"') and val.startswith('"') else val \
                         for key, val in self.getEnvironment().items()}
    settings = {key: val % environment for key, val in settings.items()}
    return (self.host, log_file % environment, settings)
  
  def getEnvironment(self):
    """
    Returns all environment variables of supervisor.
    Necessary to determine DAQling location on the remote machine.
    """
    ret = {}
    # printenv with --null argument is necessary to handle multi-line environment variables,
    # for exaplme in case BASH_FUNC__spack_shell_wrapper%% function, which is present
    # in DAQling environment. However, printing 0x00 character breaks the log file and
    # as such it is impossible to get printenv --null results here.
    # Ultimately, export -p command works better.
    settings = {
        'command': 'bash -c "export -p"',
        'autostart':'false',
        'autorestart': 'false',
        'startsecs': '0',
    }
    name = "printenv"
    cleanup_state = 0
    output = ""
    # Require locking since this can be potentially called from status checker in the future.
    with supervisor_wrapper_lock:
      try:
        if not self.hasProcess(name):
          self.server.twiddler.addProgramToGroup(self.group, name, settings)
        elif not self.isProcessStopped(name):
          cleanup_state = 1
          self.server.supervisor.stopProcess(self.group+":"+name, True)
        cleanup_state = 1
        self.server.supervisor.startProcess(self.group+":"+name)
        cleanup_state = 2
        counter = 0
        while not self.isProcessStopped(name):
          sleep(0.05)
          counter +=1
          if (counter > 20):
            self.server.supervisor.stopProcess(self.group+":"+name, True)
            raise RPCError(Faults.STILL_RUNNING, self.group+":"+name)
        cleanup_state = 3
        output = '\n'+self.server.supervisor.readProcessStdoutLog(self.group+":"+name, 0, 0)

      except RPCError as e:
        print("Error while determinig environment on host "+self.host+":")
        if 0 == cleanup_state:
          print("Could not add '" + name + "' programm")
        if 1 == cleanup_state:
          error = self.server.supervisor.readProcessStderrLog(self.group+":"+name, 0, 0)
          print(error)
          self.server.supervisor.clearProcessLogs(self.group+":"+name)
        if 2 == cleanup_state:
          print("Time out for running '" + name + "' programm")
        if 3 == cleanup_state:
          print("Could not read logs for '" + name + "' programm")
        return ret
      
      finally:
        if 3 <= cleanup_state:
          pass
        if 2 <= cleanup_state:
          self.server.supervisor.clearProcessLogs(self.group+":"+name)
        if 1 <= cleanup_state:
          self.server.twiddler.removeProcessFromGroup(self.group, name)
        if 0 <= cleanup_state:
          pass

    for line in output.split('\ndeclare -x '):
      string = str(line)
      if not string:
        continue
      try:
        val = string.split('=', 1)
        ret[val[0]] = val[1]
      except Exception as e:
        continue
    return ret

