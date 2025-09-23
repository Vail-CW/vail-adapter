#! /bin/sh

src=$1
dst=$2

info=$dst/INFO_UF2.txt
echo -n "Waiting for $info to appear..."
while ! [ -f $info ]; do
    echo -n "."
    sleep 1
done
echo "👍"

cp $src $dst
echo "Installed!"
