#!/bin/bash

test_method=$1
client_num=$2
packet_size=$3

test_method=${test_method:-pingpong}
client_num=${client_num:-10}
packet_size=${packet_size:-64}

# echo "test_method = $test_method"
# echo "client_num = $client_num"
# echo "packet_size = $packet_size"

for ((i=0; i<$client_num; i++));
do
    ./asio_echo_client --host=127.0.0.1 --port=8090 --mode=echo --test=$test_method --packet-size=$packet_size &
done
