#!/bin/bash

for file in ./*
do
    if test -d $file then
        if [ ! -d  $file"/obj/" ];then
            mkdir $file"/obj"
        fi
    fi
done
