FROM dockcross/linux-arm64
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
COPY . /work/
WORKDIR /
RUN apt update && apt upgrade -y && apt install -y git curl unzip tar libpthread-stubs0-dev autopoint po4a && apt autoremove
RUN git clone https://github.com/microsoft/vcpkg && \
    cd /vcpkg/ && git checkout 322efbb61760887eab5007ff14b00fc0d2aac4a5 && cd / && \
    /vcpkg/bootstrap-vcpkg.sh -disableMetrics && \
    /vcpkg/vcpkg integrate install
RUN /vcpkg/vcpkg install --debug --overlay-triplets=/work/vcpkg/custom_triplets --overlay-ports=/work/vcpkg/custom_ports libsodium:arm64-unknown-linux-static && \
    /vcpkg/vcpkg install --debug --overlay-triplets=/work/vcpkg/custom_triplets --overlay-ports=/work/vcpkg/custom_ports rocksdb:arm64-unknown-linux-static && \
    /vcpkg/vcpkg install --debug --overlay-triplets=/work/vcpkg/custom_triplets --overlay-ports=/work/vcpkg/custom_ports zlib:arm64-unknown-linux-static && \
    /vcpkg/vcpkg install --debug --overlay-triplets=/work/vcpkg/custom_triplets --overlay-ports=/work/vcpkg/custom_ports civetweb:arm64-unknown-linux-static && \
    /vcpkg/vcpkg install --debug --overlay-triplets=/work/vcpkg/custom_triplets --overlay-ports=/work/vcpkg/custom_ports minizip:arm64-unknown-linux-static  && \
    /vcpkg/vcpkg install --debug --overlay-triplets=/work/vcpkg/custom_triplets --overlay-ports=/work/vcpkg/custom_ports roaring:arm64-unknown-linux-static && \
    /vcpkg/vcpkg install --debug --overlay-triplets=/work/vcpkg/custom_triplets --overlay-ports=/work/vcpkg/custom_ports asio:arm64-unknown-linux-static && \
    /vcpkg/vcpkg install --debug --overlay-triplets=/work/vcpkg/custom_triplets --overlay-ports=/work/vcpkg/custom_ports fmt:arm64-unknown-linux-static  && \
    /vcpkg/vcpkg install --debug --overlay-triplets=/work/vcpkg/custom_triplets --overlay-ports=/work/vcpkg/custom_ports secp256k1-zkp:arm64-unknown-linux-static && \
    /vcpkg/vcpkg install --debug --overlay-triplets=/work/vcpkg/custom_triplets --overlay-ports=/work/vcpkg/custom_ports mio:arm64-unknown-linux-static && \
    /vcpkg/vcpkg install --debug --overlay-triplets=/work/vcpkg/custom_triplets --overlay-ports=/work/vcpkg/custom_ports libuuid:arm64-unknown-linux-static && \
    rm -Rf /work/build && \
    mkdir /work/build && \
    cmake -S /work -B /work/build -G Ninja -D CMAKE_BUILD_TYPE=Release -D GRINPP_TESTS=OFF -D GRINPP_TOOLS=OFF -D CMAKE_TOOLCHAIN_FILE=/vcpkg/scripts/buildsystems/vcpkg.cmake -D VCPKG_TARGET_TRIPLET=arm64-unknown-linux-static -D VCPKG_TARGET_ARCHITECTURE=arm64 -D VCPKG_LIBRARY_LINKAGE=static -D VCPKG_CMAKE_SYSTEM_NAME=Linux && \
    cmake --build /work/build && \
    ls -lh /work/bin/Release/GrinNode && \
    file /work/bin/Release/GrinNode
RUN mkdir /static-tor/ && \
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
    CROSS_COMPILE=\"\" ./Configure linux-armv4 -march=armv8-a --prefix=/static-tor/openssl-OpenSSL_1_0_2u/dist no-asm no-shared no-dso no-zlib && \
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
    ./configure --prefix=/static-tor/tor-tor-0.4.3.6/bin --enable-static-tor --disable-gcc-hardening --disable-system-torrc --disable-asciidoc --disable-systemd --disable-lzma --disable-seccomp --disable-module-relay --disable-manpage --disable-asciidoc --disable-html-manual --disable-unittests \
                --enable-static-openssl --with-openssl-dir=/static-tor/openssl-OpenSSL_1_0_2u/dist \
                --enable-static-libevent --with-libevent-dir=/static-tor/libevent-2.1.12-stable/dist \
                --enable-static-zlib --with-zlib-dir=/static-tor/zlib-1.2.11/dist && \
    make && \
    make install && \
    ls -lh /static-tor/tor-tor-0.4.3.6/bin/bin/tor && \
    cp -a /static-tor/tor-tor-0.4.3.6/bin/bin /work/bin/Release && \
    mv /work/bin/Release/bin /work/bin/Release/tor 