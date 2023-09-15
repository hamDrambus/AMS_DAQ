################################################################################
# Help                                                                         #
################################################################################
Help()
{
   # Display Help
   echo "Installation script for the DAQling framework."
   echo
   echo "Syntax: install.sh [-h|-d|-c|-s|-t|-w]"
   echo "options:"
   echo "-d     Full path to daqling-spack-repos."
   echo "-c     Full path to configs folder to be used by DAQling."
   echo "-s     If set, runs ansible host setup (requires sudo privileges)."
   echo "-t     If set, installs control-deps."
   echo "-w     If set, installs web-deps."
   echo
}

################################################################################
################################################################################
# Main program                                                                 #
################################################################################
################################################################################

BASEDIR="$(dirname "$(realpath "$BASH_SOURCE")" )"
DAQLING_REPO_PATH="$(dirname "$(realpath "${BASEDIR}")" )"
CONFIG_FILE="${BASEDIR}/setup.cfg"
export DAQLING_LOCATION=${DAQLING_REPO_PATH}
HOST_SETUP=false
INSTALL_CONTROL=false
INSTALL_WEB=false
while getopts ":hd:c:wst" option; do
   case "$option" in
      h) # display Help
         Help
         exit;;
      d)
        echo "option d - ${OPTARG}"
        DAQLING_SPACK_REPO_PATH_VAR=${OPTARG};;
      c)
        echo "option c - ${OPTARG}"
        DAQ_CONFIG_PATH_VAR=${OPTARG};;
      w)
        echo "option w - Installing web-deps"
        INSTALL_WEB=true;;
      s)
        echo "option s - Setting up host"
        HOST_SETUP=true;;
      t)
        echo "option t - Installing control-deps"
        INSTALL_CONTROL=true;;
         
   esac
done

## Both configs and spack-repo paths must be supplied when creating config file first time:
if [ ! -s ${CONFIG_FILE} ] && ([ -z $DAQLING_SPACK_REPO_PATH_VAR ] || [ -z $DAQ_CONFIG_PATH_VAR ]); then
    echo "Error: Supply both configs and daqling-spack-repo paths to create configs file."
    exit
fi
touch ${CONFIG_FILE}

## Store values not to be overwritten:
. ${CONFIG_FILE}
if [ -z $DAQLING_SPACK_REPO_PATH_VAR ]; then
DAQLING_SPACK_REPO_PATH_VAR=${DAQLING_SPACK_REPO_PATH}
fi
if [ -z $DAQ_CONFIG_PATH_VAR ]; then
DAQ_CONFIG_PATH_VAR=${DAQ_CONFIG_PATH}
fi

## rewrite file
echo "DAQLING_SPACK_REPO_PATH=${DAQLING_SPACK_REPO_PATH_VAR}" > ${CONFIG_FILE}
echo "DAQ_CONFIG_PATH=${DAQ_CONFIG_PATH_VAR}" >> ${CONFIG_FILE}

if [ -d "/opt/ohpc/pub/compiler/gcc/8.3.0/" ]
then  
    echo "CUSTOM_GCC_PATH=/opt/ohpc/pub/compiler/gcc/8.3.0/" >> ${CONFIG_FILE}
else
    echo "CUSTOM_GCC_PATH=" >> ${CONFIG_FILE}
fi
if [ ! -z $CUSTOM_GCC_PATH ]
then 
  echo "Custom compiler, installed from OHPC."
  export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$CUSTOM_GCC_PATH/lib64/
  #export PATH=$PATH:$CUSTOM_GCC_PATH/bin

  #export CC=$CUSTOM_GCC_PATH/bin/gcc
  export CXX=$CUSTOM_GCC_PATH/bin/g++
fi

if [ "${HOST_SETUP}" = true ] ; then
  ansible-playbook ${DAQLING_REPO_PATH}/ansible/set-up-host.yml --ask-become
fi
if [ "${INSTALL_WEB}" = true ] ; then
  ansible-playbook ${DAQLING_REPO_PATH}/ansible/install-webdeps.yml
elif [ "${INSTALL_CONTROL}" = true ] ; then
  ansible-playbook ${DAQLING_REPO_PATH}/ansible/install-controldeps.yml
fi
