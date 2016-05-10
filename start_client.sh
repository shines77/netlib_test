#!/bin/bash

client_num=$1
client_num=${client_num:-10}

packet_size=$2
packet_size=${packet_size:-64}

# echo "client_num = $client_num"
# echo "packet_size = $packet_size"

for ((i=0; i<$client_num; i++));
do
    ./asio_echo_client 192.168.2.154 8090 $packet_size &
done
