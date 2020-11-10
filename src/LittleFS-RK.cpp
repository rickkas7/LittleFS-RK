#include "LittleFS-RK.h"

#include "filesystem.h"

LittleFS *LittleFS::_instance = 0;


LittleFS::LittleFS(SpiFlash *flash, size_t startBlock, size_t numBlocks, size_t blockSize) : 
    flash(flash), startBlock(startBlock), numBlocks(numBlocks), blockSize(blockSize) {
    _instance = this;
}

LittleFS::~LittleFS() {

}

int LittleFS::mount() {
    const auto fs = filesystem_get_instance(nullptr);

    int res = filesystem_mount(fs);

    return res;
}

int LittleFS::unmount() {
    const auto fs = filesystem_get_instance(nullptr);

    int res = filesystem_unmount(fs);

    return res;
}
