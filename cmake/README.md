# Build system

The project's build system is based heavily on CMake. We ship a set of CMake modules inspired from other CERN projects. Some FindXYZ modules exist for some tools here as examples, and in case the build system needs extensions.

# Cross-build

One of the main principles of DAQling is to make it run in "lightweight" mode: sockets and queues on embedded systems. For this, we provide some setup scripts that can be easily modified to pick-up Linaro toolchains from x86 to ARM builds. We tested the following list with success:

    aarch64-linux-gnu
    arm-linux-gnueabihf
    armv8l-linux-gnueabihf

Provided you have your Linaro toolchains under /opt/toolchains, run one of the setup scripts:

    source setup-armv8l.sh

At the moment, we don't support HTTP and REST calls from the framework itself. Hence the reason, issue the following CMake command in your build area:

    cmake .. -DWITH_DOCS=OFF -DBUILD_WITH_CPR=OFF -DBUILD_MetricSimulator=OFF -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF

Ansible playbook for cross-build is coming.

