trap 'rm main' INT

printf 'Select Example: \n1. Simple Routing\n2. File Server\n3. WebSocket\n4. File I/O\n'

stty -echo
read -n 1 name
stty echo

case $name in
    49) rg="router/io.c"  ;; # works?
    50) rg="static/serve.c" ;;
    51) rg="ws/ws.c" ;;
    *) exit 0 ;;
esac

echo $rg

if [ "$(uname -s)" = "Linux" ]; then
    modprobe tls && #gcc main.c -o main -O3 -lssl -lcrypto -luring
    eval "gcc $rg -o main -O3 -lssl -lcrypto -luring"
else
    #gcc main.c -o main -O3 -lssl -lcrypto
    eval "gcc $rg -o main -O3 -lssl -lcrypto"
fi

./main && rm main