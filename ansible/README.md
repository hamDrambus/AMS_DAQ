# Install the daqling framework
### Install ansible on CentOS7

    sudo yum install -y ansible

### Run ansible-playbook to configure your host
One command to set up your host and install supervisord

    cd ansible/
    ansible-playbook set-up-host.yml --ask-become
