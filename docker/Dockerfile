ARG PYTHON_VERSION=3.13
ARG EM_VERSION=4.0.1
ARG EM_VERSION_COMMIT=9dbdc4b3437750b85d16931c7c801bb71a782122
ARG EM_NODE_VERSION=20.18.0_64bit
ARG OPENOCD_COMMIT=0ff57c6f8f7440cda52e6a1847b0b28c91e5b558
ARG ARM_GNU_TOOLCHAIN_VERSION=14.2.rel1
ARG ARM_GNU_TOOLCHAIN_PLATFORM=x86_64-arm-none-eabi
ARG PEBBLE_BOARD_LIST="snowy_bb2 spalding_bb2 silk_bb2 asterix_evt1"

FROM python:${PYTHON_VERSION} AS base
RUN apt update && apt install -y --no-install-suggests --no-install-recommends gcc-multilib gettext clang libusb-1.0-0-dev

FROM base as git2cl
WORKDIR /tmp
RUN wget https://repo.or.cz/git2cl.git/snapshot/78455e00db59155469212fecae1362be00eb8754.zip && \
    unzip 78455e00db59155469212fecae1362be00eb8754.zip && \
    mv git2cl-78455e0/ git2cl/

FROM base as jimtcl
WORKDIR /tmp
RUN wget https://github.com/msteveb/jimtcl/archive/51f65c6d38fbf86e1f0b036ad336761fd2ab7fa0.zip && \
    unzip 51f65c6d38fbf86e1f0b036ad336761fd2ab7fa0.zip && \
    mv jimtcl-51f65c6d38fbf86e1f0b036ad336761fd2ab7fa0 jimtcl/

FROM base AS arm-toolchain
ARG ARM_GNU_TOOLCHAIN_VERSION
ARG ARM_GNU_TOOLCHAIN_PLATFORM
RUN curl -L "https://developer.arm.com/-/media/Files/downloads/gnu/${ARM_GNU_TOOLCHAIN_VERSION}/binrel/arm-gnu-toolchain-${ARM_GNU_TOOLCHAIN_VERSION}-${ARM_GNU_TOOLCHAIN_PLATFORM}.tar.xz" -o arm-gnu-toolchain.tar.xz && \
    tar xf arm-gnu-toolchain.tar.xz -C /opt && \
    mv /opt/arm-gnu-toolchain-${ARM_GNU_TOOLCHAIN_VERSION}-${ARM_GNU_TOOLCHAIN_PLATFORM} /opt/arm-gnu-toolchain

FROM base AS emsdk
ARG EM_VERSION_COMMIT
ARG EM_VERSION
RUN wget https://github.com/emscripten-core/emsdk/archive/${EM_VERSION_COMMIT}.zip && \
    unzip ${EM_VERSION_COMMIT}.zip && \
    mv emsdk-${EM_VERSION_COMMIT}/ emsdk/ && \
    cd emsdk/ && \
    ./emsdk install ${EM_VERSION} && \
    ./emsdk activate ${EM_VERSION}

FROM base AS openocd
ARG OPENOCD_COMMIT
RUN wget https://github.com/pebble/openocd/archive/${OPENOCD_COMMIT}.zip && \
    unzip ${OPENOCD_COMMIT}.zip && \
    mv openocd-${OPENOCD_COMMIT}/ openocd/
WORKDIR /openocd
COPY --from=jimtcl /tmp/jimtcl/ jimtcl/
COPY --from=git2cl /tmp/git2cl/ tools/git2cl/
RUN ./bootstrap nosubmodule && \
    ./configure --disable-werror --disable-ewarning && \
    make && \
    make install

FROM base AS runtime
COPY --from=arm-toolchain /opt/arm-gnu-toolchain /opt/arm-gnu-toolchain
COPY --from=openocd /usr/local /usr/local
COPY --from=emsdk /emsdk /emsdk
ARG EM_NODE_VERSION
ENV PATH="/emsdk:/emsdk/upstream/emscripten:/emsdk/node/${EM_NODE_VERSION}/bin:/opt/arm-gnu-toolchain/bin:${PATH}"

WORKDIR /app
COPY requirements.txt .
COPY python_libs/ python_libs/
RUN pip install -r requirements.txt

COPY . .
CMD ["/bin/bash", "-c"]