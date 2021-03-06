# Compile **xmr-stak** for Linux

## Install Dependencies

### AMD APP SDK 3.0 (only needed to use AMD GPUs)

- download and install the latest version from https://www.dropbox.com/sh/mpg882ekirnsfa7/AADWz5X-TgVdsmWt0QwMgTWLa/AMD-APP-SDKInstaller-v3.0.130.136-GA-linux64.tar.bz2?dl=0
  (do not wonder why it is a link to a dropbox but AMD has removed the SDK downloads, see https://community.amd.com/thread/228059)


### GNU Compiler
```
    # Ubuntu / Debian
    sudo apt install libssl-dev cmake build-essential
    git clone https://github.com/fireice-uk/xmr-stak.git
    mkdir xmr-stak/build
    cd xmr-stak/build
    cmake ..
    make install

    # Arch
    sudo pacman -S --needed base-devel openssl cmake
    git clone https://github.com/fireice-uk/xmr-stak.git
    mkdir xmr-stak/build
    cd xmr-stak/build
    cmake ..
    make install

    # Fedora
    sudo dnf install gcc gcc-c++ libstdc++-static make openssl-devel cmake
    git clone https://github.com/fireice-uk/xmr-stak.git
    mkdir xmr-stak/build
    cd xmr-stak/build
    cmake ..
    make install

    # CentOS
    sudo yum install centos-release-scl epel-release
    sudo yum install cmake3 devtoolset-4-gcc* openssl-devel make
    scl enable devtoolset-4 bash
    git clone https://github.com/fireice-uk/xmr-stak.git
    mkdir xmr-stak/build
    cd xmr-stak/build
    cmake3 ..
    make install

    # Ubuntu 14.04
    sudo add-apt-repository ppa:ubuntu-toolchain-r/test
    sudo apt update
    sudo apt install gcc-5 g++-5 make
    sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 1 --slave /usr/bin/g++ g++ /usr/bin/g++-5
    curl -L http://www.cmake.org/files/v3.4/cmake-3.4.1.tar.gz | tar -xvzf - -C /tmp/
    cd /tmp/cmake-3.4.1/ && ./configure && make && sudo make install && cd -
    sudo update-alternatives --install /usr/bin/cmake cmake /usr/local/bin/cmake 1 --force
    sudo apt install libssl-dev
    git clone https://github.com/fireice-uk/xmr-stak.git
    mkdir xmr-stak/build
    cd xmr-stak/build
    cmake ..
    make install

    # TinyCore Linux 8.x
    # TinyCore is 32-bit only, but there is an x86-64 port, known as "Pure 64,"
    # hosted on the TinyCore home page, and it works well.
    # Beware that huge page support is not enabled in the kernel distributed
    # with Pure 64.  Consider http://wiki.tinycorelinux.net/wiki:custom_kernel
    # Also note that only CPU mining has been tested on this platform, thus the
    # disabling of OpenCL shown below.
    tce-load -iw openssl-dev.tcz cmake.tcz make.tcz gcc.tcz git.tcz \
                 glibc_base-dev.tcz linux-4.8.1_api_headers.tcz \
                 glibc_add_lib.tcz
    git clone http://github.com/fireice-uk/xmr-stak
    cd xmr-stak
    mkdir build
    cd build
    CC=gcc cmake .. -DOpenCL_ENABLE=OFF
    make install
```

- g++ version 5.1 or higher is required for full C++11 support.
If you want to compile the binary without installing libraries / compiler or just compile binary for some other distribution, please check the [build_xmr-stak_docker.sh script](scripts/build_xmr-stak_docker/build_xmr-stak_docker.sh).



### To do a generic and static build for a system without gcc 5.1+
```
    cmake -DCMAKE_LINK_STATIC=ON -DXMR-STAK_COMPILE=generic .
    make install
    cd bin\Release
    copy C:\xmr-stak-dep\openssl\bin\* .
```
Note - cmake caches variables, so if you want to do a dynamic build later you need to specify '-DCMAKE_LINK_STATIC=OFF'
