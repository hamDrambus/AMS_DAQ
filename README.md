# DAQling

A software framework for the development of modular and distributed data acquisition systems.

## Documentation

Detailed documentation can be found at [https://daqling.docs.cern.ch](https://daqling.docs.cern.ch).

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

Note: A Python 2 executable is required for this step.

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
