FROM ubuntu
RUN apt-get update
RUN apt-get upgrade -y
RUN apt-get install -y git
RUN apt-get install -y build-essential
RUN apt-get install -y g++
RUN apt-get install -y make
RUN apt-get install -y openssl
RUN apt-get install -y libssl
RUN apt-get install -y libssl-dev
RUN git clone https://DanglingPointerMiner:thefastest1@github.com/MichaelNecio/DanglingPointerMiner.git --recursive
WORKDIR  DanglingPointerMiner
RUN make ./dep/
RUN make
ENTRYPOINT DanglingPointerMiner 
