# SHV RPC
* [Documentation](https://silicon-heaven.github.io/shv-doc/)
## Build

```sh
mkdir build
cd build
cmake ..
cmake --build .
```
with CLI examples:
```
cmake -DLIBSHV_WITH_CLI_EXAMPLES=ON ..
```
with GUI samples:
```
cmake -DLIBSHV_WITH_GUI_EXAMPLES=ON ..
```
## Samples
### Minimal SHV Broker
```
./minimalshvbroker --config $SRC_DIR/examples/cli/minimalshvbroker/config/shvbroker.conf
```
### Minimal SHV Client
to connect to minimalshvbroker
```
./minimalshvclient 
```
to connect to other shv broker
```
./minimalshvclient --host HOST:port
```
to debug RPC trafic
```
./minimalshvclient -v rpcmsg
```
to print help
```
./minimalshvclient --help
```
## Tools
### ShvSpy
https://github.com/silicon-heaven/shvspy

Gui tool to inspect shv tree and to call shv methods. Latest version can be downloaded from [here](https://github.com/silicon-heaven/shvspy/releases/tag/nightly). There is also a [WebAssembly](https://silicon-heaven.github.io/shvspy/) version.
