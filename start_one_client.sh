#!/bin/bash

test_method=$1
packet_size=$2

test_method=${test_method:-pingpong}
packet_size=${packet_size:-64}

# echo "test_method = $test_method"
# echo "packet_size = $packet_size"

./asio_echo_client --host=192.168.3.225 --port=8090 --mode=echo --test=$test_method --packet-size=$packet_size &
