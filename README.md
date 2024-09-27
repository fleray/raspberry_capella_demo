
# raspberry_capella_demo
Mobile demo using Raspberry PI with Couchbase Lite C SDK code and Capella

UPDATE 09/27/2024 : add support for customer scope and collections (measures.temperatures and measures.pressures).

# Pre requisite
## Install CB lite dependencies and CB Lite itself on Raspberry PI

```
curl -O https://ftp.lip6.fr/pub/linux/distributions/debian/pool/main/i/icu/icu-devtools_67.1-7_arm64.deb
curl -O https://ftp.lip6.fr/pub/linux/distributions/debian/pool/main/i/icu/libicu67_67.1-7_arm64.deb
curl -O http://ftp.de.debian.org/debian/pool/main/i/icu/libicu-dev_67.1-7_arm64.deb

sudo apt-get install ./libicu67_67.1-7_arm64.deb
sudo apt-get install ./icu-devtools_67.1-7_arm64.deb
sudo apt-get install ./libicu-dev_67.1-7_arm64.deb

curl -O https://packages.couchbase.com/releases/couchbase-lite-c/3.1.6/libcblite-enterprise_3.1.6-debian11_arm64.deb
curl -O https://packages.couchbase.com/releases/couchbase-lite-c/3.1.6/libcblite-dev-enterprise_3.1.6-debian11_arm64.deb

sudo apt install ./libcblite-enterprise_3.1.6-debian11_arm64.deb
sudo apt install ./libcblite-dev-enterprise_3.1.6-debian11_arm64.deb

sudo apt-get install uuid-dev
```


## Compile program

```
gcc -Wall main.c sensor_simulator.c -lcblite -luuid -o main
```


## Execute local program on Raspberry
The "main" program takes 1 argument : the AppServices endpoint.

Example:
```
./main wss://w1nvfkserkyzadhx.apps.cloud.couchbase.com:4984/probes
```

Replace the websocket URL by your Capella App Services endpoint URL.

# Thanks
A huge thank you to my colleagues Marco (@marcobevilacqua94) for his IoT Simulator logic I re-used to simulate probes values and @Pasin for his help on C code. 

