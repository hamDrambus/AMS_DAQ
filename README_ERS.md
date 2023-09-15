### Run demo (optional)

    daqpy configs/ers-test.json

or run one of the other demos.
Check log file afterwards to see log entries.

### Compile and runtime ERS flags

When building DAQling in "Release" mode, the `ERS_NO_DEBUG` compile definition is added, compiling every DEBUG call out of the final executable and libraries.

Debug macro can be disabled at run-time by defining the `TDAQ_ERS_DEBUG_LEVEL` environment variable to the highest possible debug level (default is 0, range 0-3).
`ERS_DEBUG(0, ...)` should be used for debug messages that appear only few times.
`ERS_DEBUG(3, ...)` should be used for debug messages that appera at high rate.