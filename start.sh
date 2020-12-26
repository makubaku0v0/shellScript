cmake CMakeLists.txt -DCMAKE_BUILD_TYPE=Debug
make clean && make
binaryScript="/root/detection/linux-terminal-detection"
if [ -f $binaryScript ]; then
  rm -f $binaryScript
fi
mv linux-terminal-detection $binaryScript
cd /root/detection && ./linux-terminal-detection 223.129.0.237 8086
