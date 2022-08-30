FROM ubuntu:focal

ENV DEBIAN_FRONTEND=noninteractive
RUN apt update
RUN apt -q -y upgrade
RUN apt -q -y install ninja-build
RUN apt -q -y install libpng-dev libjpeg-dev zlib1g-dev libtinyxml2-dev libturbojpeg0-dev liblz4-dev libzstd-dev libzip-dev libarchive-dev libbotan-2-dev libtbb-dev
RUN apt -q -y install libboost-atomic-dev libboost-date-time-dev libboost-exception-dev libboost-filesystem-dev libboost-iostreams-dev libboost-locale-dev libboost-log-dev libboost-math-dev libboost-program-options-dev libboost-random-dev libboost-serialization-dev libboost-system-dev libboost-thread-dev libboost-timer-dev libboost-tools-dev libboost-dev
RUN apt -q -y install qttools5-dev qtscript5-dev libqt5scripttools5 libqt5svg5-dev libqt5opengl5-dev libqt5webkit5-dev
RUN apt -q -y install libglew-dev libopenscenegraph-dev libassimp-dev
RUN apt -q -y install libvtk7-dev
RUN apt -q -y install zsh vim
