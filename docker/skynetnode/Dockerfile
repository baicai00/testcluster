FROM ubuntu:22.04

WORKDIR /work
COPY ./boost_1_55_0.tar.gz /work/
COPY ./glog.tar.gz /work/
COPY ./protobuf34.tar.gz /work/
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y && \
    apt-get upgrade -y && \
    apt-get install -y build-essential vim autoconf automake libtool git wget && \
    # install skynet
    # echo "start install skynet" && \
    # git clone -b v1.6.0 --depth=1 https://github.com/cloudwu/skynet.git && cd skynet && make linux && \
    # mkdir -p /usr/local/skynet && cd /usr/local/skynet/ && \
    # cp -Rf /work/skynet/lualib lualib && \
    # cp -Rf /work/skynet/luaclib luaclib && \
    # cp -Rf /work/skynet/cservice cservice && \
    # cp -f /work/skynet/skynet skynet &&\
    # install boost
    cd /work && \
    tar -zxf boost_1_55_0.tar.gz && cd boost_1_55_0 && \
    sh bootstrap.sh --with-libraries=thread --prefix=/usr/ && ./b2 install && \
    # install glog
    cd /work && tar -zxf glog.tar.gz -C /usr/local && \
    # install protobuf
    cd /work && tar -zxf protobuf34.tar.gz -C /usr/local && \
    # 清理
    rm -rf /work/* && \
    apt-get clean && apt-get autoclean && apt-get autoremove && \
    echo 'Install Finish'