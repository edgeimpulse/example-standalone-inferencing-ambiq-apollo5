# Example standalone for Ambiq Apollo 5

Edge Impulse firmware for Ambiq Apollo 5 EVB (apollo510_evb)

## Prerequisites
Apollo 5 EVB
GCC v13.x.x
[Segger Jlink software](https://www.segger.com/downloads/jlink/)
Deploy the model you want to test as a C++ library, copy the folder **edge-impulse-sdk**, **model-paramters** and **tflite-model** into [src/edge-impulse](src/edge-impulse) and paste the raw feature in 
```
static const float features[] = {
    // copy raw features here
};
```

## Build

To build the application:
```
make -j
```

To clean the build:
```
make clean
```

## Flash
To flash the board:
```
make deploy
```

## Debug on VsCode
> [!IMPORTANT]
> You need to install [JLink software](https://www.segger.com/downloads/jlink/) and [Cortex-Debug](https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug) extension.
To start a debug session, make sure to connect the board to J6 (JLINK USB) and press F5.

## Connect consolle to the board
Connect USB-C cables to both APOLLO5 USB connector.
The board will show as a USB device.
