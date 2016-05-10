#!/bin/bash

packet_size=$1
thread_cnt=$2

packet_size=${packet_size:-64}
thread_cnt=${thread_cnt:-0}

./asio_echo_serv 192.168.2.154 8090 $packet_size $thread_cnt
