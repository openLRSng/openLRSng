#!/bin/bash

makeone() {
    make clean
    if [ "$1" = "TX" ] ; then 
	make COMPILE_TX=1 BOARD_TYPE=$2 all
    else
	make COMPILE_TX=0 BOARD_TYPE=$2 all
    fi
    test -e openLRSng.hex && cp -v openLRSng.hex out/$1-$2.hex
}

rm -rf out
mkdir out
makeone TX 2
makeone TX 3
makeone TX 4
makeone TX 5
makeone TX 6
makeone RX 3
makeone RX 5
