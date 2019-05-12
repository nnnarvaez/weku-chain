# How to build from source HF21 release:

```
# git clone https://github.com/wekuio/weku-chain
# git submodule update --init --recursive
# mkdir build
# cd build
# cmake -DCMAKE_BUILD_TYPE=Release -DLOW_MEMORY_NODE=ON ..
# make -j$(sysctl -n hw.logicalcpu)
```
Wait for 100% completion, watch for errors (there will be some warnings that can be ignored)
If all went well, the `wekud` binary (executable) can be found inside folder: `weku-chain/build/programs/steemd/`

# Running after building:

You can run it from the folder it was created or copy the file to your usual folder, that is up to you and your linux OS skills.
the important thing is to have the `config.ini` file with your witness settings in the proper folder,  once that is done, `cd` to the folder and just run:

```
# ./wekud
```
