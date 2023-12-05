# Install the daqling framework
### Install ansible on CentOS7

    sudo yum install -y ansible

### Run cmake/setup.sh to configure your host
One command to set up your host and install supervisord

    cmake/setup.sh -s -t
    
It is possible to run ansible directly:
    cd ansible
    ansible-playbook set-up-host.yml --ask-become
    
but do take note that playbooks have dependency on some DAQling environment variables:
DAQLING\_LOCATION
DAQLING\_SUPERVISOR\_SH
DAQLING\_LOG\_DIR
    
