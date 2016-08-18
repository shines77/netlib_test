@echo "start test clients ..."
# asio_echo_client.exe mode=latency 192.168.2.154 8090 64
asio_echo_client.exe mode=pingpong 127.0.0.1 8090 64
pause
