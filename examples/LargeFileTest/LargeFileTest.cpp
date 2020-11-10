
#include "Particle.h"

#include "LittleFS-RK.h"

#include <fcntl.h>

// Test program that creates a number of large files for testing. 
// Used to exercise the whole flash over time.

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(MANUAL);

SerialLogHandler logHandler(LOG_LEVEL_WARN, { // Logging level for non-application messages
    { "app", LOG_LEVEL_INFO } // Default logging level for all application messages
});

// Chose a flash configuration:
// SpiFlashISSI spiFlash(SPI, A2); 		// ISSI flash on SPI (A pins)
// SpiFlashISSI spiFlash(SPI1, D5);		// ISSI flash on SPI1 (D pins)
// SpiFlashWinbond spiFlash(SPI, A2);	// Winbond flash on SPI (A pins)
// SpiFlashWinbond spiFlash(SPI1, D5);	// Winbond flash on SPI (D pins)
// SpiFlashMacronix spiFlash(SPI, A2);	// Macronix flash on SPI (A pins)
SpiFlashMacronix spiFlash(SPI1, D5);	// Macronix flash on SPI1 (D pins), typical config for E series
// SpiFlashP1 spiFlash;					// P1 external flash inside the P1 module


const unsigned long TEST_PERIOD_MS = 120000;
unsigned long lastTestRun = 10000 - TEST_PERIOD_MS;

typedef void (*StateHandler)();
StateHandler stateHandler = 0;

// This is the size of writes, not the sector size!
const size_t BLOCK_SIZE = 512; // This affects RAM usage.
uint8_t testBuf1[BLOCK_SIZE], testBuf2[BLOCK_SIZE];


// Number of BLOCK_SIZE bytes per file
// For BLOCK_SIZE = 512, NUM_BLOCKS = 512: files are 256 Kbytes each
const size_t NUM_BLOCKS = 512;

size_t blockNum = 0;


// Each file is 256 Kbytes = 1/4 Mbytes
// The maximum number of files was determined by seeing when writes fail because the
// volume was full. There is more overhead than I would have thought.

// W25Q32JV = 32 Mbit = 4 Mbyte: NUM_FILES = 12, PHYS_SIZE = 4 * 1024 * 1024
// 64 Mbit = 8 Mbyte: NUM_FILES = 24 PHYS_SIZE = 8 * 1024 * 1024
// 128 Mbit = 16 Mbyte: NUM_FILES = 39, PHYS_SIZE = 16 * 1024 * 1024 
// MX25L25645G = 256 Mbit = 32 Mbyte: NUM_FILES = 96, PHYS_SIZE = 32 * 1024 * 1024
const size_t NUM_FILES = 96;
const size_t PHYS_SIZE = 32 * 1024 * 1024;

LittleFS fs(&spiFlash, 0, PHYS_SIZE / 4096); 


size_t fileNum = 0;
char fileName[12];
int fd;
size_t curOffset = 0;
unsigned long fileStartTime;

void fillBuf(uint8_t *buf, size_t size);

void stateWaitNextRun();
void stateChipEraseAndMount();
void stateStartNextFile();
void stateWriteAndTest();
void stateVerifyNextFile();
void stateVerifyBlocks();
void stateDeleteFiles();
void stateFailure();


class LogTime {
public:
	inline LogTime(const char *desc) : desc(desc), start(millis()) {
		Log.info("starting %s", desc);
	}
	inline ~LogTime() {
		Log.info("finished %s: %lu ms", desc, millis() - start);
	}

	const char *desc;
	unsigned long start;
};


void setup() {
	Serial.begin();

	spiFlash.begin();

    stateHandler = stateWaitNextRun;
}

void loop() {
    if (stateHandler) {
        (*stateHandler)();
    }
}


void stateWaitNextRun() {
	if (millis() - lastTestRun < TEST_PERIOD_MS) {
        return;
    }
	
    lastTestRun = millis();
    stateHandler = stateChipEraseAndMount;
}

void stateChipEraseAndMount() {
	Log.info("sending reset to flash chip");
	spiFlash.resetDevice();

	if (!spiFlash.isValid()) {
		Log.info("failed to detect flash chip");
		stateHandler = stateFailure;
		return;
	}

	// If using a 256 Mbit SPI flash chip, enable 32-bit
	// addressing or things will not work properly
	if (PHYS_SIZE > (16 * 1024 * 1024)) {
		// Larger than 16 Mbyte requires 32-bit addressing mode
		spiFlash.set4ByteAddressing(true);
	}

    // Unmount if mounted
    LittleFS::getInstance()->unmount();

    int res;

    {
        LogTime time("chipErase");
        spiFlash.chipErase();
    }

    res = LittleFS::getInstance()->mount();

    Log.info("mount res=%d", res);
    if (res != 0) {
        stateHandler = stateFailure;
        return;
    }

	srand(0);
	fileNum = 0;

    stateHandler = stateStartNextFile;
}

void stateStartNextFile() {
    // Create a new numbered file
	if (++fileNum > NUM_FILES) {
		Log.info("writes completed, verifying files now");
		srand(0);
		fileNum = 0;
		lastTestRun = millis();
		stateHandler = stateVerifyNextFile;
		return;
	}

	snprintf(fileName, sizeof(fileName), "t%d", fileNum);

	fd = open(fileName, O_RDWR|O_CREAT);
	if (fd == -1) {
        Log.info("open failed %d", __LINE__);
		stateHandler = stateFailure;
		return;
	}

    curOffset = 0;
	blockNum = 0;
	stateHandler = stateWriteAndTest;
	fileStartTime = millis();

	Log.info("writing file %u", fileNum);
}

void stateWriteAndTest() {

	if (++blockNum > NUM_BLOCKS) {
		// Done with this file
		Log.info("file %u completed in %lu ms", fileNum, millis() - fileStartTime);
		close(fd);
		stateHandler = stateStartNextFile;
		return;
	}

	fillBuf(testBuf1, sizeof(testBuf1));

	size_t offset = curOffset;

	int writeResult = write(fd, testBuf1, sizeof(testBuf1));
	if (writeResult != sizeof(testBuf1)) {
		Log.info("write failure %d != %u at offset %u", writeResult, sizeof(testBuf1), offset);
		close(fd);
		stateHandler = stateFailure;
		return;
	}
    curOffset += sizeof(testBuf1);

	lseek(fd, offset, SEEK_SET);

	read(fd, (char *)testBuf2,  sizeof(testBuf2));

	for(size_t ii = 0; ii < sizeof(testBuf1); ii++) {
		if (testBuf1[ii] != testBuf2[ii]) {
			Log.info("mismatched data blockNum=%u ii=%u expected=%02x got=%02x", 
				blockNum, ii, testBuf1[ii], testBuf2[ii]);
		}
	}

}

void stateVerifyNextFile() {
	if (++fileNum > NUM_FILES) {
		Log.info("tests completed!");
		stateHandler = stateDeleteFiles;
		return;
	}

	snprintf(fileName, sizeof(fileName), "t%d", fileNum);

	fd = open(fileName, O_RDWR|O_CREAT);
	if (fd == -1) {
        Log.info("open failed %d", __LINE__);
		stateHandler = stateFailure;
		return;
	}

	blockNum = 0;
	stateHandler = stateVerifyBlocks;
	fileStartTime = millis();

	Log.info("verifying file %u", fileNum);
}

void stateVerifyBlocks() {
	if (++blockNum > NUM_BLOCKS) {
		// Done with this file
		Log.info("file %u verified in %lu ms", fileNum, millis() - fileStartTime);
		close(fd);
		stateHandler = stateVerifyNextFile;
		return;
	}

	fillBuf(testBuf1, sizeof(testBuf1));

	read(fd, (char *)testBuf2,  sizeof(testBuf2));

	for(size_t ii = 0; ii < sizeof(testBuf1); ii++) {
		if (testBuf1[ii] != testBuf2[ii]) {
			Log.info("mismatched data blockNum=%u ii=%u expected=%02x got=%02x", 
				blockNum, ii, testBuf1[ii], testBuf2[ii]);
		}
	}
}

void stateDeleteFiles() {
	Log.info("deleting all files");

	for(size_t ii = 0; ii < NUM_FILES; ii++) {
		snprintf(fileName, sizeof(fileName), "t%d", ii);
		unlink(fileName);
	}

	Log.info("running tests again...");
	srand(0);
	fileNum = 0;
    stateHandler = stateStartNextFile;

}

void stateFailure() {
	static bool reported = false;
	if (!reported) {
		reported = true;
		Log.info("entered failure state, tests stopped");
	}
}


void fillBuf(uint8_t *buf, size_t size) {
	for(size_t ii = 0; ii < size; ii += 4) {
		int val = rand();
		buf[ii] = (uint8_t) (val >> 24);
		buf[ii + 1] = (uint8_t) (val >> 16);
		buf[ii + 2] = (uint8_t) (val >> 8);
		buf[ii + 3] = (uint8_t) val;
	}
}

