@echo "start test clients ..."
asio_echo_client.exe mode=qps 192.168.2.191 8090 64
rem asio_echo_client.exe mode=latency 192.168.2.191 8090 64
pause
