#!/bin/bash

packet_size=$1
thread_cnt=$2

packet_size=${packet_size:-64}
thread_cnt=${thread_cnt:-0}

./asio_echo_serv --host=127.0.0.1 --port=8090 --mode=echo --test=qps --packet-size=$packet_size --thread-num=$thread_cnt
