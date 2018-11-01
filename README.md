# ec700_project

## Getting Started ##

1. Download [Intel PIN 3.7](https://software.intel.com/en-us/articles/pin-a-binary-instrumentation-tool-downloads)
2. Clone me into `<pin directory>/source/tools`
3. Execute: `make obj-intel64/lbr_simulator.so` to build
4. To test run: `../../../pin -t obj-intel64/lbr_simulator.so -- <program path>`
