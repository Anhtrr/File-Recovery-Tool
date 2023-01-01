#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>
#include <math.h>

// 
#define TRUE 1
#define FALSE 0

//
#pragma pack(push,1)
typedef struct BootEntry {
  unsigned char  BS_jmpBoot[3];     // Assembly instruction to jump to boot code
  unsigned char  BS_OEMName[8];     // OEM Name in ASCII
  unsigned short BPB_BytsPerSec;    // Bytes per sector. Allowed values include 512, 1024, 2048, and 4096
  unsigned char  BPB_SecPerClus;    // Sectors per cluster (data unit). Allowed values are powers of 2, but the cluster size must be 32KB or smaller
  unsigned short BPB_RsvdSecCnt;    // Size in sectors of the reserved area
  unsigned char  BPB_NumFATs;       // Number of FATs
  unsigned short BPB_RootEntCnt;    // Maximum number of files in the root directory for FAT12 and FAT16. This is 0 for FAT32
  unsigned short BPB_TotSec16;      // 16-bit value of number of sectors in file system
  unsigned char  BPB_Media;         // Media type
  unsigned short BPB_FATSz16;       // 16-bit size in sectors of each FAT for FAT12 and FAT16. For FAT32, this field is 0
  unsigned short BPB_SecPerTrk;     // Sectors per track of storage device
  unsigned short BPB_NumHeads;      // Number of heads in storage device
  unsigned int   BPB_HiddSec;       // Number of sectors before the start of partition
  unsigned int   BPB_TotSec32;      // 32-bit value of number of sectors in file system. Either this value or the 16-bit value above must be 0
  unsigned int   BPB_FATSz32;       // 32-bit size in sectors of one FAT
  unsigned short BPB_ExtFlags;      // A flag for FAT
  unsigned short BPB_FSVer;         // The major and minor version number
  unsigned int   BPB_RootClus;      // Cluster where the root directory can be found
  unsigned short BPB_FSInfo;        // Sector where FSINFO structure can be found
  unsigned short BPB_BkBootSec;     // Sector where backup copy of boot sector is located
  unsigned char  BPB_Reserved[12];  // Reserved
  unsigned char  BS_DrvNum;         // BIOS INT13h drive number
  unsigned char  BS_Reserved1;      // Not used
  unsigned char  BS_BootSig;        // Extended boot signature to identify if the next three values are valid
  unsigned int   BS_VolID;          // Volume serial number
  unsigned char  BS_VolLab[11];     // Volume label in ASCII. User defines when creating the file system
  unsigned char  BS_FilSysType[8];  // File system type label in ASCII
} BootEntry;
#pragma pack(pop)

//
#pragma pack(push,1)
typedef struct DirEntry {
  unsigned char  DIR_Name[11];      // File name
  unsigned char  DIR_Attr;          // File attributes
  unsigned char  DIR_NTRes;         // Reserved
  unsigned char  DIR_CrtTimeTenth;  // Created time (tenths of second)
  unsigned short DIR_CrtTime;       // Created time (hours, minutes, seconds)
  unsigned short DIR_CrtDate;       // Created day
  unsigned short DIR_LstAccDate;    // Accessed day
  unsigned short DIR_FstClusHI;     // High 2 bytes of the first cluster address
  unsigned short DIR_WrtTime;       // Written time (hours, minutes, seconds
  unsigned short DIR_WrtDate;       // Written day
  unsigned short DIR_FstClusLO;     // Low 2 bytes of the first cluster address
  unsigned int   DIR_FileSize;      // File size in bytes. (0 for directories)
} DirEntry;
#pragma pack(pop)

//
#pragma pack(push,1)
typedef struct FatEntry{
    unsigned int clusterIndex;
} FatEntry;
#pragma pack(pop)


// MAIN
int main(int argc, char*argv[]){
    void validateUsage(int argc, char*argv[]);
    validateUsage(argc, argv);
    return 0;
}

// MILESTONE 1 - validate command line options
void validateUsage(int argc, char*argv[]){
    // declare the function to print usage information and exit prog
    void printUsageInfo();
    // ERROR 1 - prog invoked with no arguments
    if(argc == 1){
        printUsageInfo();
    }
    // store option
    int opt;
    // for collecting user command
    char command = '\0';
    char* commandArg = NULL;
    char* sArg = NULL;
    // get option
    while ((opt = getopt(argc, argv, "r:R:s:il")) != -1){
        switch (opt){
            // option -i
            case 'i': 
                // set command as option -i
                command = 'i';
                break;
            // option -l
            case 'l':
                // set command as option -l
                command = 'l';
                break;
            // option -r
            case 'r':
                // set command as option -r
                command = 'r';
                commandArg = optarg;                
                // store opt -s (if it exists)
                int sOpt;
                // get opt -s
                while ((sOpt = getopt(argc, argv, "s:")) != -1){
                    switch (sOpt){
                        // option -s
                        case 's':
                            // set sArg as argument for -s option
                            sArg = optarg;
                            break;
                        // ERROR 2 - if any other options called with -r
                        default:
                            printUsageInfo();
                    }
                }
                break;
            // option -R
            case 'R':
                // set command as option -R
                command = 'R';
                commandArg = optarg;
                // store opt -s (if it exists)
                int sOptR;
                // get opt -s
                while ((sOptR = getopt(argc, argv, "s:")) != -1){
                    switch (sOptR){
                        // option -s
                        case 's':
                            // set sArgR as argument for -s option
                            sArg = optarg;
                            break;
                        // ERROR 2 - if any other options called with -R
                        default:
                            printUsageInfo();
                    }
                }
                break;
            // option -s
            case 's':
                sArg = optarg;
                // set command as option -s
                int sOptFirst;
                while((sOptFirst = getopt(argc, argv, "R:r:")) != -1){
                    switch(sOptFirst){
                        case 'r':
                            command = 'r';
                            commandArg = optarg;
                            break;
                        case 'R':
                            command = 'R';
                            commandArg = optarg;
                            break;
                        default:
                            printUsageInfo();
                    }
                }
                break;
            // ERROR 3 - Option not listed above called
            default: 
                printUsageInfo();
        }
    }
    // store states of disk
    struct stat diskStat;
    // ERROR 4 - if more than one unrecognized argument (should be only disk file name)
    if (optind != argc-1){     
        printUsageInfo();
    }
    // ERROR 5 - if disk declared does not exist
    else if (stat(argv[optind], &diskStat) == -1){
        printUsageInfo();
    }
    else{
        // assign and call function of command declared in user option
        void assignCommand(unsigned char command, unsigned char* commandArg, unsigned char* sArg, int sValid, unsigned char* diskImage);
        int sValid = FALSE;
        if(sArg){
            sValid = TRUE;
        }
        assignCommand((unsigned char) command, (unsigned char*) commandArg, (unsigned char*) sArg, sValid, (unsigned char*) argv[optind]);
    }
    return;
} 
void printUsageInfo(){
    fprintf(stderr, "Usage: ./nyufile disk <options>\n  -i                     Print the file system information.\n  -l                     List the root directory.\n  -r filename [-s sha1]  Recover a contiguous file.\n  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
    exit(1);
}
void assignCommand(unsigned char command, unsigned char* commandArg, unsigned char* sArg, int sValid, unsigned char* diskImage){
    // Print the file system information.
    if(command == 'i'){
        void option_i(char* diskImage);
        option_i((char*) diskImage);
        return;
    }
    // List the root directory.
    else if(command == 'l'){
        void option_l(char* diskImage);
        option_l((char*) diskImage);
        return;
    }
    // Recover a contiguous file.
    else if(command == 'r'){
        // ERROR 6 - if option -r is called with no argument
        if(!commandArg){
            printUsageInfo();
        }
        void option_rR(unsigned char command, unsigned char* diskImage, unsigned char* fileName, unsigned char* shaHash, int sValid);
        option_rR(command, diskImage, commandArg, sArg, sValid);
    }


    // Recover a possibly non-contiguous file.
    else if(command == 'R'){
        // ERROR 8 - if option -R is called with no argument
        if(!commandArg){
            printUsageInfo();
        }
        else if (!sArg){
            printUsageInfo();
        }
        //void option_rR(unsigned char command, unsigned char* diskImage, unsigned char* fileName, unsigned char* shaHash);
        //option_rR(command, diskImage, commandArg, sArg);
    }
    // ERROR 10 - if none of the above conditions are met
    else{
        printUsageInfo();
    }
    return;
}

// MILESTONE 2 - option -i
void option_i(char* diskImage){
    void printUsageInfo();
    BootEntry* diskBootSector;
    int fd;
    fd = open(diskImage, O_RDONLY, S_IRUSR | S_IWUSR);
    // if file aint open-able
    if (fd == -1){
        printUsageInfo();
    }
    // mmap the disk boot sector
    diskBootSector = (BootEntry*) mmap(NULL, 512, PROT_READ, MAP_PRIVATE, fd, 0);
    
    unsigned char numOfFats = diskBootSector->BPB_NumFATs;
    unsigned short numOfBPS = diskBootSector->BPB_BytsPerSec;
    unsigned char numOfSPC = diskBootSector->BPB_SecPerClus;
    unsigned short numOfRS = diskBootSector->BPB_RsvdSecCnt;

    printf("Number of FATs = %i\nNumber of bytes per sector = %hu\nNumber of sectors per cluster = %i\nNumber of reserved sectors = %hu\n", numOfFats, numOfBPS, numOfSPC, numOfRS);
    return;
}

// MILESTONE 3 - option -l
void option_l(char* diskImage){
    // declare functions
    void printUsageInfo();
    void processRootEntry(unsigned char* disk, unsigned int rootDirStartIndex, unsigned int bytesPerClus, 
    unsigned int fatAreaStartIndex, unsigned int fatAreaSize, unsigned int rootClusIndex, int entryCount);
    // variables
    unsigned char* disk;
    int fd;
    struct stat diskStat;
    fd = open(diskImage, O_RDONLY, S_IRUSR | S_IWUSR);
    // if file aint open-able
    if (fd == -1){
        printUsageInfo();
    }
    // if sb aint fstat-able
    if (fstat(fd, &diskStat) == -1){
        printUsageInfo();
    }
    // mmap the disk boot sector
    disk = mmap(NULL, diskStat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    BootEntry* diskBootSector = (BootEntry*) disk;
    // bytes per SECTOR
    unsigned short numOfBPS = diskBootSector->BPB_BytsPerSec;
    // num of reserved SECTORS
    unsigned short numOfRS = diskBootSector->BPB_RsvdSecCnt;
    // size of each FAT in SECTORS
    unsigned int sizeOfEachFAT = diskBootSector->BPB_FATSz32;
    // number of FATs 
    unsigned char numOfFATS = diskBootSector->BPB_NumFATs;
    // sectors per CLUSTER 
    unsigned char numOfSPC = diskBootSector->BPB_SecPerClus;
    // bytes per CLUSTER
    unsigned int bytesPerCluster = numOfBPS*numOfSPC;
    // cluster start 
    unsigned int rootClusterIndex = diskBootSector->BPB_RootClus;
    // root area and fat area in bytes
    unsigned int reservedArea = numOfRS*numOfBPS;
    unsigned int fatArea = numOfFATS*sizeOfEachFAT*numOfBPS;
    // root cluster offset
    unsigned int rootClusterOffset = (rootClusterIndex-2) * bytesPerCluster;
    // root directory start index
    unsigned int rootDirStartIndex = reservedArea+fatArea+rootClusterOffset;
    // process the rootentry
    processRootEntry(disk, rootDirStartIndex, bytesPerCluster, reservedArea, fatArea, rootClusterIndex, 0);
    return;
}
void processRootEntry(unsigned char* disk, unsigned int rootDirStartIndex, unsigned int bytesPerClus
, unsigned int fatAreaStartIndex, unsigned int fatAreaSize, unsigned int rootClusIndex, int entryC){
    int entryCount = entryC;
    for (unsigned int i = 0; i < bytesPerClus/32 ; i++){
        DirEntry* rootEntry = (DirEntry*) &disk[rootDirStartIndex+(i*32)];
        unsigned char* dirName = rootEntry->DIR_Name;
        // BASE CASE 1 - if no more files in directory
        if(!dirName[0]){
            break;
        }
        // BASE CASE 2 - check if file is deleted
        else if(dirName[0] == 0xE5){
            continue;
        }
        // BASE CASE 3 - if long file name
        if (rootEntry->DIR_Attr == 0x0f){
            continue;
        }
        // ELSE - if file/dir exists
        unsigned char* fileName = (unsigned char*) malloc (sizeof(char)*13);
        int isDir = FALSE;
        if (rootEntry->DIR_Attr == 0x10){
            isDir = TRUE;
        }
        // process the name of file/directory
        for (int j = 0; j < 9; j++){
            if(dirName[j] == ' ' || j == 8){
                if (isDir == FALSE){   
                    if(dirName[8] != ' '){
                        fileName[j] = '.';
                        fileName[j+1] = dirName[8];
                    }
                    if(dirName[9] != ' '){
                        fileName[j+2] = dirName[9];
                    }
                    if(dirName[10] != ' '){
                        fileName[j+3] = dirName[10];
                    }
                }
                else{
                    fileName[j] = '/';
                }
                break;
            }
            else{
                fileName[j] = dirName[j];
            }
        }
        unsigned int fileSize = rootEntry->DIR_FileSize;
        unsigned short highClus = rootEntry->DIR_FstClusHI;
        unsigned short lowClus = rootEntry->DIR_FstClusLO;
        unsigned int clus = highClus*(pow(2,16))+lowClus;
        printf("%s (size = %u, starting cluster = %u)\n", fileName, fileSize, clus);
        entryCount++;
        free(fileName);
    }
    // check FAT if root directory continues
    unsigned int rootDirOffset = fatAreaStartIndex+(4*rootClusIndex);
    // if new root directory location exceeds the FAT area
    if(rootDirOffset >= fatAreaStartIndex+fatAreaSize){
        return;
    }
    FatEntry* fatEntryy = (FatEntry*) &disk[rootDirOffset];
    unsigned int rootDirFatLoc = fatEntryy->clusterIndex;
    // RECURSIVE - if not end of root directory and not a bad cluster
    if((rootDirFatLoc < 0x0ffffff8)&&(rootDirFatLoc != 0x0ffffff7)&&(rootDirFatLoc != 0x00000000)){
        unsigned int newOffset = (rootDirFatLoc-2) * bytesPerClus;
        unsigned int newRootStartIndex = fatAreaStartIndex+fatAreaSize+newOffset;
        processRootEntry(disk, newRootStartIndex, bytesPerClus, fatAreaStartIndex, fatAreaSize, rootDirFatLoc, entryCount);
        return;
    }
    printf("Total number of entries = %i\n", entryCount);
    return;
}

// MILESTONE 4, 5, 6, 7 - option -r, -s
void option_rR(unsigned char command, unsigned char* diskImage, unsigned char* fileName, unsigned char* shaHash, int sValid){
    // declare functions
    void printUsageInfo();
    //
    void searchDeletedFiles(unsigned char* disk, unsigned char* fileName, unsigned int rootDirStartIndex, 
    unsigned int bytesPerClus, unsigned int fatAreaStartIndex, unsigned int fatAreaSize, unsigned int rootClusIndex
    , unsigned int bytesPerFat, unsigned int numOfFats, unsigned int foundcount, unsigned char* shaHash,
    DirEntry* preserve, int found, unsigned int preserveStartDataSect, int sValid);
    //
    //void findDeletedFiles(unsigned char* disk, unsigned char* fileName, unsigned int rootDirStartIndex, 
    //unsigned int bytesPerClus, unsigned int fatAreaStartIndex, unsigned int fatAreaSize, unsigned int rootClusIndex
    //, unsigned int bytesPerFat, unsigned int numOfFats, unsigned int foundcount, unsigned char* shaHash, DirEntry** entries
    //, unsigned int preserveStartDataSect);

    // variables
    unsigned char* disk;
    int fd;
    struct stat diskStat;
    // had to search online to figure out how to write with mmap
    fd = open( (char*) diskImage, O_RDWR | O_CREAT, (mode_t)0600);
    // if file aint open-able
    if (fd == -1){
        printUsageInfo();
    }
    // if sb aint fstat-able
    if (fstat(fd, &diskStat) == -1){
        printUsageInfo();
    }
    // mmap the disk boot sector
    disk = mmap(NULL, diskStat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    BootEntry* diskBootSector = (BootEntry*) disk;
    // bytes per SECTOR
    unsigned short numOfBPS = diskBootSector->BPB_BytsPerSec;
    // num of reserved SECTORS
    unsigned short numOfRS = diskBootSector->BPB_RsvdSecCnt;
    // size of each FAT in SECTORS
    unsigned int sizeOfEachFAT = diskBootSector->BPB_FATSz32;
    // number of FATs 
    unsigned char numOfFATS = diskBootSector->BPB_NumFATs;
    unsigned int bytesPerFAT = sizeOfEachFAT*numOfBPS;
    // sectors per CLUSTER 
    unsigned char numOfSPC = diskBootSector->BPB_SecPerClus;
    // bytes per CLUSTER
    unsigned int bytesPerCluster = numOfBPS*numOfSPC;
    // cluster start 
    unsigned int rootClusterIndex = diskBootSector->BPB_RootClus;
    // root area and fat area in bytes
    unsigned int reservedArea = numOfRS*numOfBPS;
    unsigned int fatArea = numOfFATS*sizeOfEachFAT*numOfBPS;
    // root cluster offset
    unsigned int rootClusterOffset = (rootClusterIndex-2) * bytesPerCluster;
    // root directory start index
    unsigned int rootDirStartIndex = reservedArea+fatArea+rootClusterOffset;
    unsigned char* filee = (unsigned char*) fileName;
    
    // contiguous 
    if(command == 'r'){
        // call the actual recovery method
        searchDeletedFiles(disk, filee, rootDirStartIndex, bytesPerCluster, reservedArea, 
        fatArea, rootClusterIndex, bytesPerFAT, (unsigned int) numOfFATS, 0, shaHash, 
        NULL, FALSE, rootDirStartIndex, sValid);
    }
    return;
}
void searchDeletedFiles(unsigned char* disk, unsigned char* fileName, unsigned int rootDirStartIndex, 
unsigned int bytesPerClus, unsigned int fatAreaStartIndex, unsigned int fatAreaSize, unsigned int rootClusIndex
, unsigned int bytesPerFat, unsigned int numOfFats, unsigned int foundcount, unsigned char* shaHash, DirEntry* preserve
, int found, unsigned int preserveStartDataSect, int sValid){
    void recoverContFile(unsigned char* disk, unsigned char* fileName, DirEntry* rootEntry, 
    unsigned int fatAreaStartIndex, unsigned int bytesPerClus, unsigned int numOfFats, unsigned int bytesPerFat);
    unsigned char *SHA1(const unsigned char *d, size_t n, unsigned char *md); 
    DirEntry* preservedEntry = preserve;
    unsigned int matchCount = foundcount;
    unsigned int startDataSect = preserveStartDataSect;
    int shaFound = found;
    // if user provided -s option
    if(sValid == TRUE){
        for (unsigned int i = 0; i < bytesPerClus/32; i++){
            DirEntry* rootEntry = (DirEntry*) &disk[rootDirStartIndex+(i*32)];
            unsigned char* dirName = rootEntry->DIR_Name;
            // BASE CASE 1 - if no more files in directory
            if(!dirName[0]){
                break;
            }
            // DELETED FILES
            else if(dirName[0] == (unsigned char) 0xe5){
                // CHECK FOR NAME MATCH (excluding first character)
                unsigned char compareFileName[11] = "           ";
                compareFileName[0] = 0xe5;
                // convert fileName to dirName format
                for(unsigned int j = 1; j < 9; j++){
                    if(fileName[j]){
                        if(fileName[j] == '/'){
                            break;
                        }
                        else if(fileName[j] == '.'){
                            if(fileName[j+1]){
                                compareFileName[8] = (unsigned char) toupper(fileName[j+1]);
                                if(fileName[j+2]){
                                    compareFileName[9] = (unsigned char) toupper(fileName[j+2]);
                                    if(fileName[j+3]){
                                        compareFileName[10] = (unsigned char) toupper(fileName[j+3]);
                                    }
                                }
                            }
                            break;
                        }
                        else{
                            compareFileName[j] = (unsigned char) toupper(fileName[j]);
                            
                        }
                    }
                    else{
                        break;
                    }
                }
                // check if name of file matches user specified
                int matches1 = TRUE;
                for(unsigned int j = 1; j < 11; j++){
                    if (compareFileName[j] == dirName[j]){
                        continue;
                    }
                    else{
                        matches1 = FALSE;
                        break;
                    }
                } 
                if(matches1 == FALSE){
                    continue;
                }
                // NAME MATCHES USER-SPECIFIED NAME AT THIS POINT (Case insensitive)
                
                // CHECK FOR CONTENT MATCH
                unsigned short highClus = rootEntry->DIR_FstClusHI;
                unsigned short lowClus = rootEntry->DIR_FstClusLO;
                unsigned int clus = highClus*(pow(2,16))+lowClus;
                unsigned int sizeOfFile = rootEntry->DIR_FileSize;
                unsigned int fileLoc = fatAreaStartIndex+fatAreaSize+((clus-2)*bytesPerClus);
                // locate data cluster
                unsigned char* cmpContent = (unsigned char*) &disk[fileLoc];
                unsigned char nonConvHash[20];           
                SHA1(cmpContent, sizeOfFile, nonConvHash);
                // converted to compare
                char cmpHash[40];
                // loop through each character in returned hash (20 characters) 
                for(unsigned int j = 0; j < 20; j++){
                    // convert to 40 characters
                    sprintf(&cmpHash[j*2], "%02x", nonConvHash[j]);
                }
                if (strcmp(cmpHash, (char*) shaHash) == 0){
                    preservedEntry = rootEntry;
                    shaFound = TRUE;
                    break;
                }
                else{
                    continue;
                }
            }
            else{
                continue;
            }
        }
    }
    // if user did not specify hash command
    else{
        for (unsigned int i = 0; i < bytesPerClus/32 ; i++){
            DirEntry* rootEntry = (DirEntry*) &disk[rootDirStartIndex+(i*32)];
            unsigned char* dirName = rootEntry->DIR_Name;
            // BASE CASE 1 - if no more files in directory
            if(!dirName[0]){
                break;
            }
            // if file is a deleted file
            else if(dirName[0] == 0xe5){
                unsigned char compareFileName[11] = "           ";
                compareFileName[0] = 0xe5;
                // convert fileName to dirName format
                for(unsigned int j = 1; j < 9; j++){
                    if(fileName[j]){
                        if(fileName[j] == '/'){
                            break;
                        }
                        else if(fileName[j] == '.'){
                            if(fileName[j+1]){
                                compareFileName[8] = (unsigned char) toupper(fileName[j+1]);
                                if(fileName[j+2]){
                                    compareFileName[9] = (unsigned char) toupper(fileName[j+2]);
                                    if(fileName[j+3]){
                                        compareFileName[10] = (unsigned char) toupper(fileName[j+3]);
                                    }
                                }
                            }
                            break;
                        }
                        else{
                            compareFileName[j] = fileName[j];
                        }
                    }
                    else{
                        break;
                    }
                }
                int matches = TRUE;
                for(unsigned int k = 1; k < 11; k++){
                    if (compareFileName[k] == dirName[k]){
                        continue;
                    }
                    else{
                        matches = FALSE;
                        break;
                    }
                } 
                if(matches == FALSE){
                    continue;
                }
                else{
                    preservedEntry = rootEntry;
                    matchCount+=1;
                }
            }
            // BASE CASE 2 - if valid file
            else{
                continue;
            }
        }
    }
    
    // check FAT if root directory continues
    unsigned int rootDirOffset = fatAreaStartIndex+(4*rootClusIndex);
    // if new root directory location exceeds the FAT area
    if(rootDirOffset >= fatAreaStartIndex+fatAreaSize){
        printf("here\n");
        return;
    }
    FatEntry* fatEntryy = (FatEntry*) &disk[rootDirOffset];
    unsigned int rootDirFatLoc = fatEntryy->clusterIndex;
    // RECURSIVE - if not end of root directory and not a bad cluster
    if(rootDirFatLoc < 0x0ffffff8){
        unsigned int newOffset = (rootDirFatLoc-2) * bytesPerClus;
        unsigned int newRootStartIndex = fatAreaStartIndex+fatAreaSize+newOffset;
        searchDeletedFiles(disk, fileName, newRootStartIndex, bytesPerClus, fatAreaStartIndex, 
        fatAreaSize, rootDirFatLoc, bytesPerFat, numOfFats, matchCount, shaHash, preservedEntry, 
        shaFound, startDataSect, sValid);
        return;
    }
    // DONE SEARCHING ROOT DIR AT THIS POINT
    unsigned char* fileNameUpper = (unsigned char*) malloc(strlen( (char*) fileName)+1);
    unsigned int k = 0;
    while(fileName[k]){
        fileNameUpper[k] = (unsigned char) toupper(fileName[k]);
        k++;
    }
    // print options if user specified a sha option
    if (sValid == TRUE){
        if(preservedEntry){
            recoverContFile(disk, fileName, preservedEntry, fatAreaStartIndex, bytesPerClus, numOfFats, bytesPerFat);
            printf("%s: successfully recovered with SHA-1\n", fileNameUpper);
        }
        else{
            printf("%s: file not found\n", fileNameUpper);
        }
        free(fileNameUpper);
        return;
    }
    // if user never specified a sha option
    else{
        // exactly one file matches the given name
        if(matchCount == 1){
            recoverContFile(disk, fileName, preservedEntry, fatAreaStartIndex, bytesPerClus, numOfFats, bytesPerFat);
            printf("%s: successfully recovered\n", fileNameUpper);
        }
        // more than one file matches the given name
        else if (matchCount > 1){
            printf("%s: multiple candidates found\n", fileNameUpper);
        }
        else{
            printf("%s: file not found\n", fileNameUpper);
        }
        free(fileNameUpper);
        return;
    }   
}
void recoverContFile(unsigned char* disk, unsigned char* fileName, DirEntry* rootEntry, unsigned int fatAreaStartIndex
, unsigned int bytesPerClus, unsigned int numOfFats, unsigned int bytesPerFat){
    // recover first character in filename 
    rootEntry->DIR_Name[0] = (unsigned char) toupper(fileName[0]);
    // find the cluster index of file
    unsigned short highClus = rootEntry->DIR_FstClusHI;
    unsigned short lowClus = rootEntry->DIR_FstClusLO;
    unsigned int clus = highClus*(pow(2,16))+lowClus;
    // get file cluster offset
    unsigned int fileClusOffset = fatAreaStartIndex+(4*clus);
    // check if file is larger than one cluster
    unsigned int fileSize = rootEntry->DIR_FileSize;
    if(fileSize>bytesPerClus){
        unsigned int clusterCount = fileSize/bytesPerClus;
        if (fileSize%bytesPerClus!=0){
            clusterCount+=1;
        }
        // for each FAT
        for(unsigned int j = 0; j < numOfFats; j++){
            // for each cluster 
            for(unsigned int k = 0; k < clusterCount; k++){
                FatEntry* file = (FatEntry*) &disk[fileClusOffset+(4*k)+(bytesPerFat*j)];
                // last cluster of file data
                if(k == clusterCount-1){
                    file->clusterIndex = (unsigned int) 0x0ffffff8;
                }
                else{
                    file->clusterIndex = (unsigned int) clus+k+1;
                }  
            }  
        }
    }
    // short file recovery
    else{
        for(unsigned int j = 0; j < numOfFats; j++){
            FatEntry* file = (FatEntry*) &disk[fileClusOffset+(bytesPerFat*j)];
            // change first FAT entry of file back to starting cluster
            file->clusterIndex = (unsigned int) 0x0ffffff8;
        }   
    }
    return;
}
