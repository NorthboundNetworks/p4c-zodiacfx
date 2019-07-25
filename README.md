# p4c-zodiacfx
Zodiac FX extension for the P4 Compiler

## Installation

### Ubuntu dependencies

Most dependencies can be installed using `apt-get install`:

`sudo apt-get install cmake g++ git automake libtool libgc-dev bison flex
libfl-dev libgmp-dev libboost-dev libboost-iostreams-dev
libboost-graph-dev llvm pkg-config python python-scapy python-ipaddr python-ply
tcpdump`

For documentation building:
`sudo apt-get install -y doxygen graphviz texlive-full`

### Install protobuf 3.2.0
```bash
git clone https://github.com/google/protobuf.git`
git checkout v3.2.0`
./autogen.sh`
./configure`
make`
make check`
sudo make install`
sudo ldconfig # refresh shared library cache.`
```

### P4-16 Compiler
First you need to follow the installation guide of [P4-16](https://github.com/p4lang/p4c/)
When you have P4-16 compiler, then add this project as an extension.
Assuming you have P4-16 at your dir  ~/p4c/, to setup p4c-zodiacfx:
```bash
cd ~/p4c/
mkdir extensions
cd extensions
git clone https://github.com/pzanna/p4c-zodiacfx.git
ln -s ~/p4c p4c-zodiacfx/p4c
```
Now that you have cloned p4c-zodiacfx at ~/p4c/extensions/p4c-zodiacfx, the next step is to
recompile p4c:
```bash
cd ~/p4c/
mkdir -p build
cd build/
cmake ..
make
```
This generates a p4c-zodiacfx binary in ~/p4c/build.
Next create a soft link to the binary:
```bash
cd ~/p4c/extensions/p4c-zodiacfx
ln -s ~/p4c/build/p4c-zodiacfx p4c-zodiacfx
```

### Current Status
This is the first version of this extension and very much a work in progress to don't expect too much at the start with, more functionality will be added over time. On that note, any assistance would be extremely welcome. 