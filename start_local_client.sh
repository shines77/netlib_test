#!/bin/bash

client_num=$1
packet_size=$2

client_num=${client_num:-10}
packet_size=${packet_size:-64}

# echo "client_num = $client_num"
# echo "packet_size = $packet_size"

for ((i=0; i<$client_num; i++));
do
    ./asio_echo_client --host=127.0.0.1 --port=8090 --mode=echo --test=qps --packet-size=$packet_size &
done
