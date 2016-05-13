@echo "start test clients ..."
rem asio_echo_client.exe mode=qps 127.0.0.1 8090 64
asio_echo_client.exe mode=throughput 127.0.0.1 8090 64
pause
