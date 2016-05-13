@echo "start test clients ..."
rem asio_echo_client.exe mode=qps 192.168.2.191 8090 64
asio_echo_client.exe mode=throughput 192.168.2.191 8090 64
pause
