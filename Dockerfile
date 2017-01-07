FROM jbradach/toolchain_latest
MAINTAINER Jack Bradach <jack@bradach.net>

RUN DEBIAN_FRONTEND=noninteractive apt-get update \
    && apt-get install -y \
        freeglut3-dev \
        libglew-dev \
        libsdl2-dev \
        libsdl2-mixer-dev \
        libsdl2-ttf-dev \
        zlib1g-dev \
        cmake \
        git
