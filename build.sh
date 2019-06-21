
#!/bin/sh

if [ -d "./out" ];then
    echo "clean old result: ./out"
    rm -r ./out
fi

echo "begin compile"

mkdir ./out
cd ./out

cmake ..

make

cd ../

echo "compile finish"
    