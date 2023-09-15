FROM gitlab-registry.cern.ch/ci-tools/ci-worker:cc7
ENV PATH=$PATH:/opt/ohpc/pub/compiler/gcc/8.3.0/bin
RUN yum install -y http://build.openhpc.community/OpenHPC:/1.3/CentOS_7/x86_64/ohpc-release-1.3-1.el7.x86_64.rpm && \
yum install -y gnu8-compilers-ohpc cmake llvm-toolset-7-clang-tools-extra llvm-toolset-7-clang && \
git clone --recurse-submodules https://:@gitlab.cern.ch:8443/ep-dt-di/daq/daqling-spack-repo.git && \
/daqling-spack-repo/Install.sh && \
yum clean all && \
rm -rf /tmp/*

