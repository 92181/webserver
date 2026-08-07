if [ "$(uname -s)" = "Linux" ]; then
    gcc test_server.c -o server -O3 -flto -lssl -lcrypto -luring
    gcc test_client.c -o test -O3 -flto -lssl -lcrypto -lkqueue

    sudo modprobe tls
else
    gcc test_server.c -o server -O3 -lssl -lcrypto
    gcc test_client.c -o test -O3 -lssl -lcrypto
fi

sudo ./server

rm -f test server