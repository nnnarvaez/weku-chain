# Introducing Weku (beta)

Weku is a Delegated Proof of Stake platform based on the STEEM blockchain, that uses a "Proof of Brain" social consensus algorithm for token allocation.

  - Currency symbol WEKU.
  - 10% APR inflation narrowing to 1% APR over 20 years.
  - 75% of inflation to "Proof of Brain" social consensus algorithm.
  - 15% of inflation to stake holders.
  - 10% of inflation to block producers.

# Public Announcement & Discussion

tbd

# Last Candidate release HF22

![](https://images2.weku.io/DQmUTkiVbJSAsLMFd8YZYa3jfw39nZnxeuZqXMQRFZGu4TZ/image.png)



# No Support & No Warranty

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

# Whitepaper

You can read the Weku Whitepaper at [weku.io/WekuWhitePaper.pdf](https://weku.io/WekuWhitePaper.pdf).
It is also reccomended to read the original [2017 / 2018 Steemit whitepaper](https://steem.com/steem-whitepaper.pdf) where many technical details are addressed


# How to build WEKUD from source HF21 release:

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

You might want to add the websocket blocking even if it is inefficient, follow [these instructions](https://github.com/nnnarvaez/weku-chain/blob/master/original_wss_block.md)



Example: (non rpc node)
```
git clone https://github.com/wekuio/weku-chain
cd weku-chain
git submodule update --init --recursive
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DLOW_MEMORY_NODE=ON -DSKIP_BY_TX_ID=ON -DCLEAR_VOTES=ON ..
make -j$(sysctl -n hw.logicalcpu)
```

Example: (full rpc node needs 128 GB server)
```
git clone https://github.com/wekuio/weku-chain
git submodule update --init --recursive
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DLOW_MEMORY_NODE=ON ..
make -j$(sysctl -n hw.logicalcpu)
```

Wait for 100% completion, watch for errors (there will be some warnings that can be ignored)
If all went well, the `wekud` binary (executable) can be found inside folder: `weku-chain/build/programs/wekud/`

# Running after building:

You can run it from the folder it was created or copy the file to your usual folder, that is up to you and your linux OS skills.
the important thing is to have the `config.ini` file with your witness settings in the proper folder,  once that is done, `cd` to the folder and just run:

```
# ./wekud
```


# Advanced build options
(For those who know what they are doing)

### LOW_MEMORY_NODE=[OFF/ON]

Builds wekud to be a consensus-only low memory node. Data and fields not
needed for consensus are not stored in the object database. This option is
recommended for witnesses and seed-nodes.

### CLEAR_VOTES=[ON/OFF]

Clears old votes from memory that are no longer required for consensus.

### BUILD_STEEM_TESTNET=[OFF/ON]

Builds weku for use in a private testnet. Also required for building unit tests.

### SKIP_BY_TX_ID=[OFF/ON]

By default this is off. Enabling will prevent the account history plugin querying transactions
by id, but saving around 65% of CPU time when reindexing. Enabling this option is a
huge gain if you do not need this


# Speeding up replay
(For linux gurus)

On Linux use the following Virtual Memory configuration for the initial sync and subsequent replays. It is not needed for normal operation.
```
echo    75 | sudo tee /proc/sys/vm/dirty_background_ratio
echo  1000 | sudo tee /proc/sys/vm/dirty_expire_centisec
echo    80 | sudo tee /proc/sys/vm/dirty_ratio
echo 30000 | sudo tee /proc/sys/vm/dirty_writeback_centisec
``` 

Revert to default values once your replay is done. 
default
```
echo    10 | sudo tee /proc/sys/vm/dirty_background_ratio
sudo rm -fr sudo tee /proc/sys/vm/dirty_expire_centisec
echo    20 | sudo tee /proc/sys/vm/dirty_ratio
sudo rm -fr /proc/sys/vm/dirty_writeback_centisec
```

# After building

You have the option to declare your witness intention to the network, unless you just want to run a read-only node for rpc or seed purposes.

[Declaring your witness intention](https://github.com/nnnarvaez/weku-chain/blob/master/registeringwitnesses.md)

# Seed Nodes

tbd


# Testing

See [doc/testing.md](doc/testing.md) for test build targets and info
on how to use lcov to check code test coverage.


# Sample witness config

**Basic**

```
seed-node = dev.weku.io:4444
rpc-endpoint = 127.0.0.1:8190
p2p-endpoint = 0.0.0.0:4444
shared-file-size = 50G
enable-plugin = witness
enable-stale-production = true
required-participation = false
witness = "<your-account>"
private-key = <your-active-private-key>
```

**Full config file with all options**
[example config.ini](https://github.com/nnnarvaez/weku-chain/blob/master/doc/example_config.ini)


# More documentation

[This folder contains varied information](https://github.com/nnnarvaez/weku-chain/tree/master/doc)
Beware some might not apply to current version
