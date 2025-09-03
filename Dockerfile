FROM ubuntu:latest

RUN apt update
RUN apt install -y \
    git \
    cmake \
    bzip2 \
    gcc-14 \
    g++-14 \
    libgcrypt-dev \
    nettle-dev \
    python3

RUN mkdir /encryption-benchmark
WORKDIR /encryption-benchmark

COPY docker_contents /encryption-benchmark

# compile
RUN ./generate_and_compile.sh
# generate benchmark configs
RUN python3 generate-benchmarks.py
# run all benchmarks
ENTRYPOINT /encrytpion-benchmark/run_all.sh
