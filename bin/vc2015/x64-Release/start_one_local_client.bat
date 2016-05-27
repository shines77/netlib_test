@echo "start test clients ..."
asio_echo_client.exe mode=qps 127.0.0.1 8090 64
rem asio_echo_client.exe mode=latency 127.0.0.1 8090 64
pause
