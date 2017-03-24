FROM ubuntu
RUN apt-get update
RUN apt-get install -y python3 
RUN apt-get install -y python3-pip
RUN apt-get install -y git
RUN apt-get install -y build-essential
RUN apt-get install -y g++
RUN apt-get install -y make
RUN apt-get install -y openssl
RUN apt-get install -y libssl-dev
RUN git clone https://DanglingPointerMiner:thefastest1@github.com/MichaelNecio/DanglingPointerMiner.git 
WORKDIR  DanglingPointerMiner
RUN make ./dep/
RUN make
ENTRYPOINT DanglingPointerMiner 
