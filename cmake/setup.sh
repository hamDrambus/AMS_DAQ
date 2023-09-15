BASEDIR="$(dirname "$(realpath "$BASH_SOURCE")" )"
CONFIG_FILE="${BASEDIR}/setup.cfg"
if [ -s ${CONFIG_FILE} ]
then
  . ${CONFIG_FILE}
else
  echo "No config file - Exiting."
  return 1
fi

DAQLING_REPO_PATH="$(dirname "$(realpath "${BASEDIR}")" )"
export DAQ_CONFIG_DIR=${DAQ_CONFIG_PATH}
export DAQ_BUILD_DIR=$PWD/build/
export DAQ_SCRIPT_DIR=${DAQLING_REPO_PATH}/scripts/
export DAQLING_LOCATION=${DAQLING_REPO_PATH}
daqpy_path=$(find -name daq.py | cut -c3-)
alias daqpy='python3 $PWD/$daqpy_path'
daqtree_path=$(find -name daqtree.py | cut -c3-)
alias daqtree='python3 $PWD/$daqtree_path'

# runtime requirements
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$DAQ_BUILD_DIR/lib
export TDAQ_ERS_STREAM_LIBS=DaqlingStreams

echo "Activating Spack DAQling environment"
source ${DAQLING_SPACK_REPO_PATH}/spack/share/spack/setup-env.sh
spack env activate daqling
spack find
spack load cmake
export CPLUS_INCLUDE_PATH=$CMAKE_PREFIX_PATH/include
export C_INCLUDE_PATH=$CMAKE_PREFIX_PATH/include
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$CMAKE_PREFIX_PATH/lib:$CMAKE_PREFIX_PATH/lib64
export LIBRARY_PATH=$LD_LIBRARY_PATH

#check if virtualenv exists
if [ -f ${DAQLING_REPO_PATH}/etc/daqling_venv/bin/activate ]; then
  echo "Activating Python DAQling virtual environment"
  source ${DAQLING_REPO_PATH}/etc/daqling_venv/bin/activate
fi
if [ ! -z $CUSTOM_GCC_PATH ]
then 
  echo "Custom compiler, installed from OHPC."
  export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$CUSTOM_GCC_PATH/lib64/
  #export PATH=$PATH:$CUSTOM_GCC_PATH/bin

  #export CC=$CUSTOM_GCC_PATH/bin/gcc
  export CXX=$CUSTOM_GCC_PATH/bin/g++
fi

#export TBB_VERSION=2019.0
#export TBB_ROOT_DIR=/opt/tbb-2019_U5

export BOOST_VERSION=1.70

if [ -f "/opt/rh/llvm-toolset-7/enable" ]
then
  source /opt/rh/llvm-toolset-7/enable
fi
