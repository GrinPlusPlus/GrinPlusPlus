FROM dockcross/linux-arm64
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
COPY . /work/
WORKDIR /
RUN apt install -y curl unzip tar libpthread-stubs0-dev autopoint po4a
RUN git clone https://github.com/microsoft/vcpkg && \
    /vcpkg/bootstrap-vcpkg.sh -disableMetrics && \
    /vcpkg/vcpkg integrate install
WORKDIR /work
RUN /vcpkg/vcpkg install --overlay-ports=/work/vcpkg/custom_ports --triplet arm64-linux --debug libsodium && \
    /vcpkg/vcpkg install --overlay-ports=/work/vcpkg/custom_ports --triplet arm64-linux --debug rocksdb && \
    /vcpkg/vcpkg install --triplet arm64-linux zlib --debug && \
    /vcpkg/vcpkg install --overlay-ports=/work/vcpkg/custom_ports --triplet arm64-linux --debug civetweb && \
    /vcpkg/vcpkg install --triplet arm64-linux minizip --debug  && \
    /vcpkg/vcpkg install --overlay-ports=/work/vcpkg/custom_ports --triplet arm64-linux --debug roaring && \
    /vcpkg/vcpkg install --triplet arm64-linux --debug asio --debug && \
    /vcpkg/vcpkg install --triplet arm64-linux --debug fmt --debug  && \
    /vcpkg/vcpkg install --overlay-ports=/work/vcpkg/custom_ports --triplet arm64-linux --debug secp256k1-zkp && \
    /vcpkg/vcpkg install --triplet arm64-linux --debug mio --debug && \
<<<<<<< HEAD
    /vcpkg/vcpkg install --triplet arm64-linux --debug libuuid && \
    /vcpkg/vcpkg integrate install && \
=======
    /vcpkg/vcpkg install --overlay-ports=/work/vcpkg/custom_ports --triplet arm64-linux --debug libuuid && \
>>>>>>> 5b215e6e... adding tools
    rm -Rf /work/build && \
    mkdir /work/build && \
    cmake -S /work -B /work/build -G Ninja -D CMAKE_BUILD_TYPE=Release -D GRINPP_TESTS=OFF -D CMAKE_TOOLCHAIN_FILE=/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=arm64-linux && \
    cmake --build /work/build && \
<<<<<<< HEAD
    ls -lh /work/bin/Release/GrinNode
RUN mkdir /static-tor/ && \
=======
    ls -lh /work/bin/Release/GrinNode && \
    mkdir /static-tor/ && \
>>>>>>> 5b215e6e... adding tools
    cd /static-tor/ && \
    curl -fsSL https://zlib.net/zlib-1.2.11.tar.gz | tar zxf - &&  \
    cd /static-tor/zlib-1.2.11 &&  \
    ./configure --prefix=/static-tor/zlib-1.2.11/dist && \
    make && \
    make install && \
    cd /static-tor/ && \
    git clone https://git.tukaani.org/xz.git &&  \
    cd /static-tor/xz &&  \
    ./autogen.sh &&  \
    ./configure --prefix=/static-tor/xz/dist --disable-shared --enable-static --disable-doc --disable-scripts --disable-xz --disable-xzdec --disable-lzmadec --disable-lzmainfo --disable-lzma-links && \
    make && \
    make install && \
    cd /static-tor/ && \
    curl -fsSL https://github.com/openssl/openssl/archive/OpenSSL_1_0_2u.tar.gz | tar zxf - &&  \
    cd /static-tor/openssl-OpenSSL_1_0_2u &&  \
<<<<<<< HEAD
    CROSS_COMPILE=\"\" ./Configure linux-armv4 -march=armv8-a --prefix=/static-tor/openssl-OpenSSL_1_0_2u/dist no-asm no-shared no-dso no-zlib && \
=======
    CROSS_COMPILE=\"\" ./Configure linux-aarch64 --prefix=/static-tor/openssl-OpenSSL_1_0_2u/dist no-shared no-dso no-zlib && \
>>>>>>> 5b215e6e... adding tools
    make && \
    make install && \
    cd /static-tor/ && \
    curl -fsSL https://github.com/libevent/libevent/releases/download/release-2.1.12-stable/libevent-2.1.12-stable.tar.gz | tar zxf - &&  \
    cd /static-tor/libevent-2.1.12-stable &&  \
    PKG_CONFIG_PATH=/static-tor/openssl-OpenSSL_1_0_2u/ ./configure --prefix=/static-tor/libevent-2.1.12-stable/dist --disable-shared --enable-static --with-pic --disable-samples --disable-libevent-regress CPPFLAGS=-I/static-tor/openssl-OpenSSL_1_0_2u/dist/include LDFLAGS=-L/static-tor/openssl-OpenSSL_1_0_2u/dist/lib && \
    make && \
    make install && \
    cd /static-tor/ && \
    curl -fsSL https://github.com/torproject/tor/archive/tor-0.4.3.6.tar.gz | tar zxf - &&  \
    cd /static-tor/tor-tor-0.4.3.6 &&  \
    ./autogen.sh &&  \
<<<<<<< HEAD
    ./configure --prefix=/static-tor/tor-tor-0.4.3.6/bin --enable-static-tor --disable-gcc-hardening --disable-system-torrc --disable-asciidoc --disable-systemd --disable-lzma --disable-seccomp --disable-module-relay --disable-manpage --disable-asciidoc --disable-html-manual --disable-unittests \
=======
    ./configure --prefix=/static-tor/tor-tor-0.4.3.6/bin --enable-static-tor --disable-gcc-hardening --disable-system-torrc --disable-asciidoc --disable-systemd --disable-lzma --disable-seccomp \
>>>>>>> 5b215e6e... adding tools
                --enable-static-openssl --with-openssl-dir=/static-tor/openssl-OpenSSL_1_0_2u/dist \
                --enable-static-libevent --with-libevent-dir=/static-tor/libevent-2.1.12-stable/dist \
                --enable-static-zlib --with-zlib-dir=/static-tor/zlib-1.2.11/dist && \
    make && \
    make install && \
    ls -lh /static-tor/tor-tor-0.4.3.6/bin/bin/tor && \
    cp -a /static-tor/tor-tor-0.4.3.6/bin/bin /work/bin/Release && \
    mv /work/bin/Release/bin /work/bin/Release/tor 