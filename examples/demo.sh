printf 'Select example: \n1. Simple Router\n2. Static File Server\n3. WebSockets\n4. File I/O\n'

stty -echo
read -n 1 name
stty echo

case $name in
    1) rg="router"  ;;
    2) rg="server"  ;;
    3) rg="ws"  ;;
    4) rg="io"  ;;
    *) exit 0 ;;
esac

cd $rg

if [ "$(uname -s)" = "Linux" ]; then
    eval "gcc $rg.c -o main -O3 -flto -lssl -lcrypto -luring"
    sudo modprobe tls
else
    exit 1
fi

sudo ./main

rm -f main