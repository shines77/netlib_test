@echo "start test clients ..."
asio_echo_client.exe --host=192.168.2.231 --port=8090 --mode=echo --test=throughput --packet-size=64 --thread-num=1
pause
