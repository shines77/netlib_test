#!/bin/bash

client_num=$1
client_num=${client_num:-10}

echo "client_num = $client_num"

for ((i=0; i<$client_num; i++));
do
    ./asio_echo_client 127.0.0.1 8090 64 &
done

