#ifndef __LITTLEFS_RK_H
#define __LITTLEFS_RK_H

#include "SpiFlashRK.h"

class LittleFS {
public:
    LittleFS(SpiFlash *flash, size_t startBlock, size_t numBlocks, size_t blockSize = 4096);
    virtual ~LittleFS();

    int mount();

    int unmount(); 

    size_t getStartBlock() const { return startBlock; };

    size_t getNumBlocks() const { return numBlocks; };

    size_t getBlockSize() const { return blockSize; };


    static SpiFlash *getFlashInstance() { return _instance->flash; };

    static LittleFS *getInstance() { return _instance; };

protected:
    SpiFlash *flash;
    size_t startBlock = 0;
    size_t numBlocks = 0;
    size_t blockSize = 4096;
    static LittleFS *_instance;
};

#endif /* __LITTLEFS_RK_H */
