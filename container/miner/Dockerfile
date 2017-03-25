FROM fedora
RUN dnf update -y
RUN dnf install -y git
RUN dnf install -y kernel-devel
RUN dnf install -y gcc
RUN dnf install -y gcc-c++
RUN dnf install -y make
RUN dnf install -y openssl
RUN dnf install -y openssl-libs
RUN dnf install -y openssl-devel
RUN dnf install -y make
RUN git clone https://DanglingPointerMiner:thefastest1@github.com/MichaelNecio/DanglingPointerMiner.git --recursive
WORKDIR  DanglingPointerMiner
RUN make
ENTRYPOINT DanglingPointerMiner 
