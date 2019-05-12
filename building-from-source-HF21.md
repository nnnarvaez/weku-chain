# How to build from source HF21 release:

It is strongly reccomended you use UBUNTU 16.04, this instructions are for this OS and altought it might run in other versions the instructions might vary and it is your responsability to figure them out. (it will be nice if a detailed how-to is provided after you succesfully manage to deploy it in a newer version)

1 -. Install pre-requirements
2 -. clone repository
3 -. compile the source

# Pre-requierements


#### Required packages
```
sudo apt-get install -y 
autoconf 
automake 
cmake 
g++ 
git 
libssl-dev 
libtool 
make 
pkg-config 
python3 
python3-jinja2
```

#### Boost packages (also required)
```
sudo apt-get install -y \
    libboost-chrono-dev \
    libboost-context-dev \
    libboost-coroutine-dev \
    libboost-date-time-dev \
    libboost-filesystem-dev \
    libboost-iostreams-dev \
    libboost-locale-dev \
    libboost-program-options-dev \
    libboost-serialization-dev \
    libboost-signals-dev \
    libboost-system-dev \
    libboost-test-dev \
    libboost-thread-dev
```

#### Optional packages (not required, but will make a nicer experience)
```
sudo apt-get install -y \
    doxygen \
    libncurses5-dev \
    libreadline-dev \
    perl

```



# Clone and Compile

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
