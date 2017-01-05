FROM debian:latest
MAINTAINER Jack Bradach <jack@bradach.net>


RUN echo "deb http://pkg.mxe.cc/repos/apt/debian wheezy main" > \
        /etc/apt/sources.list.d/mxeapt.list
RUN apt-key adv --keyserver keyserver.ubuntu.com \
        --recv-keys D43A795B73B16ABE9643FE1AFD8FFF16DB45C6AB
RUN DEBIAN_FRONTEND=noninteractive apt-get update \
    && apt-get install -y \
        bsdmainutils \
        git

RUN DEBIAN_FRONTEND=noninteractive apt-get update \
    && apt-get install -y \
        mxe-x86-64-w64-mingw32.static-zlib \
        mxe-x86-64-w64-mingw32.static-sdl2 \
        mxe-x86-64-w64-mingw32.static-sdl2-mixer \
        mxe-x86-64-w64-mingw32.static-sdl2-ttf \
        mxe-x86-64-w64-mingw32.static-freeglut \
        mxe-x86-64-w64-mingw32.static-glew \
