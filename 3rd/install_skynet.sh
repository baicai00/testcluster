#!/bin/bash

SKYNET_PATH=./skynet
INSTALL_PATH=../publish/skynet

rm -rf ${INSTALL_PATH}/*
mkdir -p ${INSTALL_PATH}
cp -Rf ${SKYNET_PATH}/lualib ${INSTALL_PATH}/lualib
cp -Rf ${SKYNET_PATH}/luaclib ${INSTALL_PATH}/luaclib
cp -Rf ${SKYNET_PATH}/cservice ${INSTALL_PATH}/cservice
cp -Rf ${SKYNET_PATH}/service ${INSTALL_PATH}/service
cp -f ${SKYNET_PATH}/skynet ${INSTALL_PATH}/skynet