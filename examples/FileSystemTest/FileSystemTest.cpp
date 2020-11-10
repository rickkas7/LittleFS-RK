#include "Particle.h"

#include "LittleFS-RK.h"

#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler;

void test1();

// Pick a chip, port, and CS line
// SpiFlashISSI spiFlash(SPI, A2);
SpiFlashWinbond spiFlash(SPI, A2);
// SpiFlashMacronix spiFlash(SPI, A2);
// SpiFlashWinbond spiFlash(SPI1, D5);
//SpiFlashMacronix spiFlash(SPI1, D5);

LittleFS fs(&spiFlash, 0, 256);

void setup() {
	// Wait for a USB serial connection for up to 10 seconds
  	waitFor(Serial.isConnected, 10000);
    delay(1000);

    spiFlash.begin();
    int res = fs.mount();
    Log.info("fs.mount() = %d", res);
}

void loop() {
    test1();
    
    delay(10000);
}

void printDir(const char *path, int indent) {
    String indentStr = String("                                     ").substring(0, 2 * indent);

    DIR *dir = opendir(path);
    if (dir) {
        while(true) {
            struct dirent* ent = readdir(dir); 
            if (!ent) {
                break;
            }
            
            if (ent->d_type == DT_REG) {
                Log.info("%s File %s", indentStr.c_str(), ent->d_name);
            }
            else
            if (ent->d_type == DT_DIR) {
                Log.info("%s Dir %s", indentStr.c_str(), ent->d_name);   
                if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..")) {
                    // Don't recursively handle . (current dir) or .. (parent dir)
                    printDir(String::format("%s/%s", path, ent->d_name), indent + 1);
                }             
            }
            
        }
        closedir(dir);
    }
    else {
        Log.error("failed to open directory %s errno=%d", path, errno);
    }
}

bool createDirIfNecessary(const char *path) {
    struct stat statbuf;

    int result = stat(path, &statbuf);
    if (result == 0) {
        if ((statbuf.st_mode & S_IFDIR) != 0) {
            Log.info("%s exists and is a directory", path);
            return true;
        }

        Log.error("file in the way, deleting %s", path);
        unlink(path);
    }
    else {
        if (errno != ENOENT) {
            // Error other than file does not exist
            Log.error("stat filed errno=%d", errno);
            return false;
        }
    }
    
    // File does not exist (errno == 2)
    result = mkdir(path, 0777);
    if (result == 0) {
        Log.info("created dir %s", path);
        return true;
    }
    else {
        Log.error("mkdir failed errno=%d", errno);
        return false;
    }
}

class SequentialFile {
public:
    SequentialFile(const char *dirPath);
    virtual ~SequentialFile();


    bool scan();
    int openNext();


    static bool createDirIfNecessary(const char *path);

protected:
    String      dirPath;
    int         lastFile;
};

SequentialFile::SequentialFile(const char *dirPath) : dirPath(dirPath) {

}

SequentialFile::~SequentialFile() {

}

bool SequentialFile::scan() {
    bool bResult = createDirIfNecessary(dirPath);
    if (!bResult) {
        return false;
    }

    DIR *dir = opendir(dirPath);
    if (dir) {
        while(true) {
            struct dirent* ent = readdir(dir); 
            if (!ent) {
                break;
            }
            
            if (ent->d_type == DT_REG) {
                Log.info("File %s", ent->d_name);

                
            }    
        }
        closedir(dir);
    }
}

int SequentialFile::openNext() {
    return -1;
}


bool SequentialFile::createDirIfNecessary(const char *path) {
    struct stat statbuf;

    int result = stat(path, &statbuf);
    if (result == 0) {
        if ((statbuf.st_mode & S_IFDIR) != 0) {
            Log.info("%s exists and is a directory", path);
            return true;
        }

        Log.error("file in the way, deleting %s", path);
        unlink(path);
    }
    else {
        if (errno != ENOENT) {
            // Error other than file does not exist
            Log.error("stat filed errno=%d", errno);
            return false;
        }
    }
    
    // File does not exist (errno == 2)
    result = mkdir(path, 0777);
    if (result == 0) {
        Log.info("created dir %s", path);
        return true;
    }
    else {
        Log.error("mkdir failed errno=%d", errno);
        return false;
    }
}


void test1() {
    Log.info("Starting test1");

    printDir("/", 0);
#if 1    
    bool bResult = createDirIfNecessary("/FileSystemTest");
    Log.info("createDirIfNecessary=%d", bResult);

    {
        int fd = open("/FileSystemTest/test1.txt", O_RDWR | O_CREAT | O_TRUNC);
        if (fd != -1) {
            for(int ii = 0; ii < 100; ii++) {
                String msg = String::format("testing %d\n", ii);

                write(fd, msg.c_str(), msg.length());
            }
            close(fd);
        }
    }
#endif
    Log.info("Completed test1");
}

/*
0000030315 [app] INFO: Starting test1
0000030316 [app] INFO:  Dir .
0000030317 [app] INFO:  Dir ..
0000030318 [app] INFO:  Dir sys
0000030319 [app] INFO:    Dir .
0000030320 [app] INFO:    Dir ..
0000030320 [app] INFO:    File dct.bin
0000030321 [app] INFO:    File cellular_config.bin
0000030322 [app] INFO:    File openthread.dat
0000030323 [app] INFO:  Dir usr
0000030324 [app] INFO:    Dir .
0000030325 [app] INFO:    Dir ..
0000030326 [app] INFO: Completed test1
*/
