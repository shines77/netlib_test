@echo "start test clients ..."
for /l %%i in (1,1,10) do start /min "asio_echo_client" asio_echo_client.exe 192.168.2.154 8090 64
