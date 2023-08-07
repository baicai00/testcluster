FROM ubuntu:22.04

WORKDIR /work
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y && \
    apt-get upgrade -y && \
    apt-get install -y build-essential vim autoconf git net-tools && \
    # install skynet
    echo "start install skynet" && \
    git clone -b v1.6.0 --depth=1 https://github.com/cloudwu/skynet.git && cd skynet && make linux && \
    mkdir -p /usr/local/skynet && cd /usr/local/skynet/ && \
    cp -Rf /work/skynet/lualib lualib && \
    cp -Rf /work/skynet/luaclib luaclib && \
    cp -Rf /work/skynet/cservice cservice && \
    cp -f /work/skynet/skynet skynet &&\
    # 清理
    rm -rf /work/skynet && \
    apt-get clean && apt-get autoclean && apt-get autoremove && \
    echo 'Install Finish'