FROM ubuntu:latest

RUN apt update
RUN apt install -y \
    git \
    cmake \
    vim \
    bzip2 \
    gcc-14 \
    g++-14 \
    libgcrypt-dev \
    nettle-dev

# Make a fake sudo
RUN echo '#!/bin/sh' > /usr/local/bin/sudo && \
    echo 'echo "[FAKE SUDO] $@"' >> /usr/local/bin/sudo && \
    echo 'exec "$@"' >> /usr/local/bin/sudo && \
    chmod +x /usr/local/bin/sudo

WORKDIR /root/

#install vim
RUN git clone https://github.com/Lurgypai/MyVimPlugins.git
WORKDIR /root/MyVimPlugins
RUN ./install_dependencies.sh
RUN ./install.sh --force-sudo

#install nettle
# RUN wget https://ftp.gnu.org/gnu/nettle/nettle-3.10.tar.gz


WORKDIR /workspace
