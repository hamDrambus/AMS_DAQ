#!/bin/bash
# Run supervisor in DAQling environment
BASEDIR="$(dirname "$(realpath "$BASH_SOURCE")" )"
SETUP_FILE="${BASEDIR}/setup.sh"
source ${SETUP_FILE}
/usr/local/bin/supervisord -n -c /etc/supervisor/daqling/supervisord.conf
