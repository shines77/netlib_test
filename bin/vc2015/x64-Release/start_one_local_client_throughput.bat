@echo "start test clients ..."
rem asio_echo_client.exe mode=qps 127.0.0.1 8090 64
asio_echo_client.exe --host=127.0.0.1 --port=8090 --mode=echo --test=throughput --packet-size=64 --thread-num=1
pause
