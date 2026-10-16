# ============================================================================
#  Reproducible build environment for RAW UI.
#
#  Ubuntu 24.04 + LLVM 18 (the toolchain release is built against LLVM 18) plus
#  the OpenOrbis PS4 toolchain itself, so that a plain `make` inside the
#  container produces the same .pkg that CI produces.
#
#  Build:  docker build -t rawui-build tools/
#  Use:    docker run --rm -v "$PWD:/workspace" -u "$(id -u):$(id -g)" \
#                     -e HOME=/tmp rawui-build
# ============================================================================
FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive
ARG OO_VERSION=v0.5.4
ARG OO_ASSET=toolchain-llvm-18.tar.gz

RUN apt-get update && apt-get install -y --no-install-recommends \
        clang-18 lld-18 llvm-18 \
        make cmake ninja-build \
        curl ca-certificates xz-utils file \
        python3 git \
    && rm -rf /var/lib/apt/lists/* \
    && ln -sf /usr/bin/clang-18    /usr/local/bin/clang \
    && ln -sf /usr/bin/clang++-18  /usr/local/bin/clang++ \
    && ln -sf /usr/bin/ld.lld-18   /usr/local/bin/ld.lld \
    && ln -sf /usr/bin/llvm-ar-18  /usr/local/bin/llvm-ar

# ---------------------------------------------------------------------------
#  OpenOrbis PS4 Toolchain (headers, stub libs, create-fself / PkgTool.Core).
#  The release tarball unpacks to OpenOrbis/PS4Toolchain/, we flatten it.
# ---------------------------------------------------------------------------
RUN mkdir -p /opt/toolchain-dl /opt/OpenOrbis \
    && curl -fsSL -o /opt/toolchain-dl/${OO_ASSET} \
       https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/releases/download/${OO_VERSION}/${OO_ASSET} \
    && tar -xzf /opt/toolchain-dl/${OO_ASSET} -C /opt/toolchain-dl \
    && mv /opt/toolchain-dl/OpenOrbis/PS4Toolchain /opt/OpenOrbis \
    && rm -rf /opt/toolchain-dl

ENV OO_PS4_TOOLCHAIN=/opt/OpenOrbis
ENV PATH=/opt/OpenOrbis/bin/linux:$PATH
# ---------------------------------------------------------------------------
#  Build OpenSSL 1.1.1w for PkgTool.Core (.NET Core 3.1) compatibility
#  Ubuntu 24.04 ships OpenSSL 3.0; PkgTool.Core needs libssl1.1.
# ---------------------------------------------------------------------------
RUN mkdir -p /tmp/ossl && cd /tmp/ossl && \
    curl -fsSL -o openssl.tar.gz https://github.com/openssl/openssl/releases/download/OpenSSL_1_1_1w/openssl-1.1.1w.tar.gz && \
    tar -xzf openssl.tar.gz && \
    cd openssl-1.1.1w && \
    ./config --prefix=/usr/local/ssl1.1 --openssldir=/usr/local/ssl1.1 shared -Wl,-rpath,/usr/local/ssl1.1/lib no-docs no-tests && \
    make -j$(nproc) && \
    make install_sw && \
    cp /usr/local/ssl1.1/lib/libssl.so.1.1 /usr/lib/x86_64-linux-gnu/ 2>/dev/null || true && \
    cp /usr/local/ssl1.1/lib/libcrypto.so.1.1 /usr/lib/x86_64-linux-gnu/ 2>/dev/null || true && \
    cp /usr/local/ssl1.1/lib/libssl.so.1.1 /var/tmp/.net/PkgTool.Core/*/ 2>/dev/null || true && \
    cp /usr/local/ssl1.1/lib/libcrypto.so.1.1 /var/tmp/.net/PkgTool.Core/*/ 2>/dev/null || true && \
    ldconfig 2>/dev/null || true && \
    rm -rf /tmp/ossl

ENV LD_LIBRARY_PATH=/var/tmp/.net/PkgTool.Core/*:/usr/local/ssl1.1/lib:/usr/lib/x86_64-linux-gnu

