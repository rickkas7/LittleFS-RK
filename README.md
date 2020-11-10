# LittleFS-RK 

Port of LittleFS for Particle Gen 2 devices.

**Warning:** This only works on Gen 2 devices (Photon, P1, Electron, E Series)! The issue is that it uses the same C POSIX file API as currently used on the Gen 3 devices, so the linker will complain that it's implemented in two places, as it is. 

**Warning:** As this point in time, it's just a proof-of-concept for testing. There are almost certainly still bugs that haven't been found yet as it has not been extensively tested yet!

- This is based on the Particle LittleFS implementation in Device OS: [https://github.com/particle-iot/device-os/tree/develop/third_party/littlefs](https://github.com/particle-iot/device-os/tree/develop/third_party/littlefs).

- It contains the POSIX wrappers from Device OS: [https://github.com/particle-iot/device-os/tree/develop/hal/src/nRF52840/littlefs](https://github.com/particle-iot/device-os/tree/develop/hal/src/nRF52840/littlefs).

- It contains other hacked bits of Device OS needed to make it compile and link from user firmware.

- The POSIX file system API calls are the same as [are documented for the Boron](https://docs.particle.io/reference/device-os/firmware/boron/#file-system).

- Tested with Winbond, ISSI, and Macronix SPI NOR flash chips.  

- It even works with the MX25L25645G 256 Mbit (32 Mbyte) flash chip, which I could not get to work reliably with SPIFFS. See note in LargeFileTest.cpp; you must enable 32-bit addressing in SpiFlashRK using `spiFlash.set4ByteAddressing(true);` for this to work properly.

## Usage

You'll probably need some includes:

```cpp
#include "LittleFS-RK.h"

#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
```

This code uses the [SpiFlashRK library](https://github.com/rickkas7/SpiFlashRK) to interface to the flash chip. You typically use one of these lines depending on the brand, SPI port, and CS line:

```cpp
// Pick a chip, port, and CS line
// SpiFlashISSI spiFlash(SPI, A2);
// SpiFlashWinbond spiFlash(SPI, A2);
// SpiFlashMacronix spiFlash(SPI, A2);
// SpiFlashWinbond spiFlash(SPI1, D5);
// SpiFlashMacronix spiFlash(SPI1, D5);
```

You then allocate a `LittleFS` object as a global:

```
LittleFS fs(&spiFlash, 0, 256);
```

The parameters are:

- `&spiFlash` the object for your flash chip
- `0` the start sector for the file system (0 = beginning of chip)
- `256` replace with the number of 4096-byte sectors to use for the file system. 256 * 4096 = 1,048,576 bytes = 1 Mbyte, the size of an 8 Mbit SPI flash chip. 

Note: You must only ever allocate one LittleFS object. Bad things will happen if you create more than one. You can allocate it with `new` but don't allocate it on the stack.

Finally, in `setup()`, initialize the SPI flash and mount the file system. This will format it if it has not been formatted.

```cpp
spiFlash.begin();
int res = fs.mount();
Log.info("fs.mount() = %d", res);
```

## Testing

There are two test programs:

- FileSystemTest: A simple app
- LargeFileTest: A test that writes larger files to test performance


## Version History

### 0.0.1 (2020-11-10)

- Initial testing version. It probably still has bugs!

