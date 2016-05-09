#!/bin/bash

client_num=$1
client_num=${client_num:-10}

packet_size=$2
packet_size=${packet_size:-64}

./asio_echo_client mode=latency 192.168.2.191 8090 $packet_size
