gcc test_server.c -o server -O3 -lssl -lcrypto

if [ "$(uname -s)" = "Linux" ]; then
    modprobe tls && gcc test_client_new.c -o test -O3 -lssl -lcrypto -libkqueue
else
    gcc test_client.c -o test -O3 -lssl -lcrypto
fi

./server && rm test && rm server