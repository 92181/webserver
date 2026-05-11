#gcc test_client.c -o test -O3 -lssl && gcc test_server.c -o server -O3 -lssl -lcrypto && ./server && rm test && rm server

#gcc test_client_new.c -o test -O3 -lssl && gcc test_server.c -o server -O3 -lssl -lcrypto && ./server && rm test && rm server
# add if linux use -libkqueue else do nothing

# work out dispatch example and archive mac.
gcc test_server.c -o server -O3 -lssl -lcrypto

if [ "$(uname -s)" = "Linux" ]; then
    modprobe tls && gcc test_client_new.c -o test -O3 -lssl -libkqueue
else
    #gcc test_client_new.c -o test -O3 -lssl
    gcc test_client.c -o test -O3 -lssl # for now use old to test server!
fi

./server && rm test && rm server