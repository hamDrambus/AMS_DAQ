# AMS data acquisition and control system based on DAQling

DAQling is a software framework for the development of modular and distributed data acquisition systems.

## Documentation

Detailed documentation on DAQling can be found at [https://daqling.docs.cern.ch](https://daqling.docs.cern.ch).

Subscribe to the "daqling-users" CERN e-group for updates.

To contact the developers: daqling-developers@cern.ch (only for "daqling-users" members).

## Host configuration and framework build

### Build dependencies installation

In order to build the framework, dependencies must be installed.

#### Installing the packages.

##### Prerequisites

Clone the DAQling Spack repository, containing Spack and custom packages.

The project is found at https://gitlab.cern.ch/ep-dt-di/daq/daqling-spack-repo

Use `--recurse-submodules` to initialize and update the Spack sub-module.

    git clone --recurse-submodules https://:@gitlab.cern.ch:8443/ep-dt-di/daq/daqling-spack-repo.git

A GCC C++17 enabled compiler is required.

For CERN CentOS 7:

    yum install http://build.openhpc.community/OpenHPC:/1.3/CentOS_7/x86_64/ohpc-release-1.3-1.el7.x86_64.rpm
    yum install gnu8-compilers-ohpc cmake
    export PATH=$PATH:/opt/ohpc/pub/compiler/gcc/8.3.0/bin

For CentOS 8:

    yum install gcc-c++ libasan libubsan cmake

For Ubuntu (Server):

    sudo apt install build-essential cmake

#### Run the install script

The install script should take care of the rest:

    cd daqling-spack-repo/
    ./Install.sh

Dependencies installation does not require root access, unless you are installing on CentOS 7.

Now all the dependencies required by DAQling will be installed in a spack environment inside this repository.

### Configure the host for running the framework

To setup the host with the system libraries and tools, follow the steps below:

1. Install Ansible:
```
sudo yum install -y ansible
```
Note: to set up an Ubuntu server follow the same procedure, using `apt` instead of `yum`.

2. Run the `cmake/install.sh` script.

Run the install script in the cmake folder in daqling.

Note: A Python 3 executable is required for this step.

The script takes the following arguments:

    -d     Full path to daqling-spack-repo.
    -c     Full path to configs folder to be used by DAQling.
    -s     If set, runs ansible host setup (requires sudo privileges).
    -t     If set, installs control-deps.
    -w     If set, installs web-deps.

On the first invocation of the install script, supply both the daqling-spack-repo and configuration folder paths.

On subsequent invocations, it is possible to supply only one of these, to change its value.

The `-w` flag can be set if one wishes to install the dependencies required for running the daqling web services. If this flag is not set, only the dependencies for running the daqling control executables will be installed.

Below is an example of how the script can be run on first invocation, with host setup (requires sudo privileges):

    ./cmake/install.sh -d /path/to/daqling-spack-repo -c /path/to/configs-folder -s

Example installing control-deps (suggested in case the host has been already set up to run DAQling by an admin):

    ./cmake/install.sh /path/to/daqling-spack-repo -c /path/to/configs-folder -t

Example installing web-deps (after first invocation):

    ./cmake/install.sh -w


#### (Optional)

Local InfluxDB + Grafana stack:

    ansible-playbook install-influxdb-grafana.yml --ask-become

Redis

    ansible-playbook install-redis.yml --ask-become

### Build

To activate the spack environment, as well as the virtual python environment containing the daqling control executable dependencies, source the cmake/setup.sh script:

    source cmake/setup.sh

Then:

    mkdir build
    cd build
    cmake ../
    make

It is possible to build specified targets. `make help` will list the available ones.

#### Additional build options

The `ccmake` command:

    source cmake/setup.sh
    cd build
    ccmake ../

allows browsing available build options, such as selection of Modules to be built and Debug flags. E.g.:

    ENABLE_SANITIZE [ON, OFF]
    CMAKE_BUILD_TYPE [Debug, Release]

To generate *Doxygen* documentation for DAQling:

    source cmake/setup.sh
    cd build
    make doc

After generation it is possible to browse the pages by opening `doxygen_html/index.html` with a browser.

## Running the data acquisition system demo

`daqpy` is a command line tool that spawns and configures the components listed in the JSON configuration file passed as argument.
It showcases the use of the `daqcontrol` library, which can be used in any other Python control tool.

It then allows to control the components via standard commands such as `start` (with optional run number), `stop`, as well as custom commands.

    source cmake/setup.sh
    daqpy configs/demo/config.json
    start [run_num]
    stop
    down

`daqpy -h` shows the help menu.

### (NEW) DAQ control tree

`daqtree` is a command line tool that allows to control the data acquisition system as a "control tree", with advanced Finite State Machine (FSM) options.
It showcases the use of the `nodetree` and `daqcontrol` libraries, which can be used in any other Python control tool.

    source cmake/setup.sh
    daqtree configs/demo-dict.json

`daqtree -h` shows the help menu.

The `render` command allows to print the current status of the control tree. The render will print the tree structure with the name of the node, the status and its key flags:

- Included [`True`/`False`]. Nodes can be excluded or included from the controlled tree with `<node> exclude` and `<node> include`. Excluded nodes will not participate to the parent's status and will not receive commands from the parent node.
- Inconsistent [`True`/`False`]. If the children of a node are in different state, the Inconsistent flag is raised.

The control of the system is granular:

- commands can be issued to any node in the tree (for example to `readoutinterface01` only).
- the full FSM of the control system can be navigated (as opposed to `daqpy`).

                  ---add-->         ---boot-->           --configure-->         --start-->
      (not_added)           (added)             (booted)                (ready)            (running)
                  <-remove-         <-shutdown-          <-unconfigure-         <--stop--- 

More details on the JSON configuration required by `daqtree` can be found in the `configs/README.md`.

## Development

To develop a custom module, the existing modules in `src/Modules` can provide guidance.

It is necessary to copy and rename the template folder `src/Modules/Dummy` and its files to start developing the new module.

The custom module will be discovered and built by CMake as part of the project.

### Run custom Modules

To run a newly created Module (e.g. `MyDummyModule`), it is necessary to add a corresponding entry in `components:` to a JSON configuration file. Note that the name of the Module needs to be specified in the `type:` field. E.g.:

```json
{
  "name": "mydummymodule01",
  "host": "localhost",
  "port": 5555,
  "modules": [
   {
     "type": "MyDummyModule",
     "name":"mydummymodule",
     "connections": {
     }
   }
  ],
  "loglevel": {"core": "INFO", "module": "DEBUG","connection":"WARNING"},
  "settings": {
  }    
}
```

## Contributors

DAQling has been developed by the EP-DT-DI section at CERN and is maintained by:

- Enrico Gamberini, CERN, @engamber
- Roland Sipos, CERN, @rsipos
- Marco Boretto, CERN, @mboretto
  
The following authors, in alphabetical order, have contributed to DAQling:

- Wojciech Brylinski, Warsaw University of Technology, @wobrylin
- Zbynek Kral, Czech Technical University in Prague, @zkral
- Jens Noerby Kristensen, CERN, @jkristen
- Giovanna Lehmann Miotto, CERN, @glehmann
- Viktor Vilhelm Sonesten, CERN, @vsoneste
- Clement Claude Thorens, Université de Genève, @cthorens

## (NEW) DAQling design issues and fixes

These issues were identified by me when designing the Accelerator Mass Spectrometry (AMS) data acquisition and analysis as well as control and monitoring system.

#### Data flow between DAQling modules

Initially DAQling supported only sending simple (trivially copyable) data structs over module connections (zmq or others).
This meant that even sending std::vectors was not possible and one had to work with hard-coded arrays.
This is particularly limiting if one uses variable number of channels from ADC board.
It is unreasonable to send full data packet if only few of the channels are actually in use.

It was remedied by me by adding transfer of arbitrary serializable data (SerializableFormat.hpp and CaenOutputFormat.hpp).

#### Data flow from DAQling modules to GUI/Python

DAQling has metric mechanism which periodically sends some data from DAQling to the python control scripts (GUI).
The issue is that this data is very simple - only sending single atomic values (double, int) is supported.
So, for example, if one wants to monitor signals obtained by ADC board in GUI (like in oscilloscope), there is no built-in mechanism to do it.
There were several workarounds:

- Extend metric functionality to support std::atomic<std::vector<>>. Poor solution because:
1. Atomic of vector is a bad idea overall and 
2. Because it is still not possible to send complex data (e.g. structs).

- Send data to database (redis or others) and pull it by GUI on demand. Poor solution because: 
1. Writes to database should logically occur only at the end of event processing (FileWriterModule).
If not, then each monitored module will send data to additional temporary database.
This adds an additional entity (complexity).
2. Support of arrays by conventional databases is very limited.
Even if there is a workaround, e.g by storing arrays via json files, there are 64 kB limits on data size (in redis at least). There are other options: rasdaman, SciDB, PostgreSQL.

- Use already existing zmq connections in modules and connect to them from python.
It is chosen solution for its simplicity.
Its feasibility is shown in scripts/CaenDummyEventViewerQt/eventmonitor.py (proof of concept, very rough prototype).
There is concern that intercepting every event in python may be too slow (event if most of them are ignored for performance's sake).
The bonus is that GUI can be notified instantly by modules, as GUI clock is not used.

- Implement custom module command which returns binary data on demand from the GUI.
Initially discarded as it requires extensive rework of DAQling command interface.
There was also a question of data size limit, but it seems it is not an issue (requires further reading on XML-RPC).
Anyway, it seems that reworking commands is required anyway, see below.
Another point to note is that while using commands, it is not possible to notify GUI instantly if something important and time sensitive happens.

### Control of states (status) of DAQling modules

DAQling module loader (ModuleManager.hpp) has finite state machine (FSM) which keeps track of DAQling process' state and DAQProcess modules' states.
The built-in states are "booted", "ready" (configured), "running" and "inconsistent" (for whole manager only) plus their transitional states (e.g. "booting").
The changes between these states are driven by commands issued via XML-RPC server (supervisord).
When command is called from python, the returned value is either "Success" or "Failure: ..." with some message.
A special status command can be called from python to return current state of all modules as a list of strings.
Custom commands can be added along with new states.
There are several issues with current design:

1. Commands can not return any data (besides status command which also does not interact with DAQProcess class).
So it is not possible to request for signature of transferred data (for de-serialization) or some state info (error message) from DAQProcess itself.
Neither it is possible to request raw data from the module, for example if one wants to get some parameters for processed events to display in GUI while avoiding polling database (which can be not written to if a run has not started yet).

2. The states of DAQProcess are managed by the ModuleManager.
It does not make sense, as state is a property of each DAQProcess and should be kept there.
The only exception is when there are no modules at all, in which case the state of the manager is simply "booted".
The most glaring issue in the current approach is that if something happens in the module (e.g. some error which should change its status from "running" to "ready"), there is no way for the ModuleManager to be made aware of this.
The concrete example is the case when connection to ADC board is interrupted for some reason (pulled out wire) and DAQProcess detects it.
The correct way to handle it would be to change the module status from "running" to "disconnected"/"configured", not throwing terminating error in noexepts DAQProcess::runner.

3. Related to p.2, the ModuleManager and XML-RPC commands assume that all command calls guarantee state transition.
Which is incorrect assumption as command execution may throw error in which case the module state will be stuck in -ing state (not tested).
Another example is when incorrect configuration (specified by user in GUI) is passed to the module.
In this case the state should stay as booted and user (GUI/python) made aware of incorrect configuration.
Finally, it is possible that some commands may have different target states even on success, for example if some simple modules (e.g. event processors) are always running and transition directly from "booted" to "running".

4. Related to p.3 and p.2, there seem no way to easily modify built-in states and transitions.
Using the example of ADC board again, additional "connected" state should be between "ready" (should be "configured" rather) and "running".
Which states for built-in commands are valid should also be changed (e.g. start() can't now run from "ready").
There is an option to ignore build-in logic and use custom commands for everything, but it seems like a good way to become confused.

It is a work in progress on how to handle these issues elegantly (esp. p.4), without heavy feature creep and extensive modifications to DAQling.

Some thoughts:

- All commands should return .json files. Corresponding parser for python will have to be added.

- Move state handling to DAQProcess class.
std::string m_state, DAQProcess::getState() at the very least.

- Enums?

- DAQProcess will handle both commands and associated state transitions.

- std::string m\_error\_message.

- Whether command is accepted by module (DAQProcess) is determined by the module itself.
Issue: in this approach DAQProcess will have two distinct roles: state handling and runner (work) management.
Separate it into two classes then somehow?
But I do kind of want to make DAQProcess a complex class to handle complicated logic.

### Default databases can't store raw signals.

MongoDB or influxdb have poor or nonexistent support for storing raw arrays of Xs and Ys.
Workaround with storing json does not cut it for expected event sizes (> 64 kB).
There are database options (rasdaman, SciDB, PostgreSQL) but I decided that at the moment setting them up is not worth it.
So the raw data will be written into binary files.
Metadata (event parameters such as is\_selected\_event, energies, currents, etc. and slow parameters values) will be stored in influxdb.

### Supervisor is not aware about DAQling

As it is, daq.py script will only work when dealing with a single localhost machine (not actually tested but see below).
It is because daq.py gets DAQling variables from its own environment, and then uses these variables to run commands via supervisor server.
But these variables (DAQling path) are correct only for localhost machine. Other hosts may have different installation paths.
As such, supervisor will fail to launch DAQling on remote machines. Another consequence is that .py scripts (e.g. Monitoring/metric-manager.py) were not launching in clean DAQling installation as they were run in system environment, not DAQling one and as such the required packages were not installed. I fixed it by specifying usage of python3 binary in virtual environment, but it was not a very clean solution.

TODO, WIP: The solution is that supervisor on each machine must be made aware of DAQling environment variables during DAQling installation.
Then, when launching programs or scrips, daq.py on the main machine will make use of this knowledge.
Ideally only daq supervisor group \[group:daq\] should be executed from DAQling environment (host/.../daqling/cmake/setup.sh), but supervisor has no mechanism for that. As such I decided that whole supervisord service will be launched in the DAQling environment (after source cmake/setup.sh).
In case that other programs will also require supervisord, it is possible to launch several services from single binary but with different names and configurations, each in its own environment. For that to work, supervisorctl which controls supervisord will have to be aliased in the DAQling environment to use correct supervisord configuration file.

The main machine (the one which launches daq.py) can be made aware about environment of other hosts (DAQling location, log file location, etc.) by running printenv command on other hosts' supervisor. Speaking of logs, I also do not like that they were in some system folders by default. All logs beside that of supervisor itself should be in DAQling workspace, on each machine.

So the changes that were made:

- Installation of supervisor in ansible playbooks is now changed. In particular, its configuration is in /etc/supervisor/daqling/ folder instead of /etc/supervisor/.

- supervisord service was also renamed to daqling-supervisord (see ansible/roles/supervisor/files/daqling-supervisord.service) and is launched in DAQling environment, i.e. after executing cmake/setup.sh. The latter is done via creating service with cmake/run_supervisor.sh as an executable.

- supervisorctl is also aliased to work with daqling-supervisord when in DAQling environment (see cmake/setup.sh).

The result is that supervisor used by DAQling is in its environment on each machine and other programs can install supervisord without interfering with or being broken by DAQling. TODO: change daqling-supervisord port from default so that it really is fully decoupled other supervisords.

- WIP: Control .py scripts now call supervisor (remote processes) using supervisor's environment instead of environment of the scripts themselves. Affected scripts are ControlGUI/ControlGUIServer.py, Control/daq.py, Control/daqcontrol.py, Contol/daqtree.py, Control/supervisor_wrapper.py and possibly some others. For example, when '%(ENV\_DAQLING\_LOG\_DIR)s' is passed to daqling-supervisord server it is expanded to value of environment variable DAQLING\_LOG\_DIR on the server. TODO: some interface cleanup is required. Also did not check that anything besides Control/daq.py (e.g. Control/daqtree.py, ControlGUI/\*, Experimental/\*) works as intended as those are not feature which I intend to use or even understand.

- WIP: Control scripts can get environment of any remote daqling-supervisord server (host) using supervisor_wrapper.getEnvironment method which is implemented via calling 'bash -c "export -p"' on the server. This is required to, for example, get location of log files on the remote host which are tied to DAQling environment.

- All log files are now in DAQling\_folder/log/ directory, not in /var/log/. Only log file of the service itself (kept by systemd) is in the default system folder.
