#include "MemoryManager.h"
#include <stdio.h>
#include <unistd.h>
#include <functional>
#include <fcntl.h>
#include <string>
#include <sstream>
#include <iostream>
#include <array>
#include <fstream>
#include <vector>
#include <utility>
#include <algorithm>
#include <bitset>
#include <cstring>
using namespace std;

MemoryManager::MemoryManager(unsigned wordSize, std::function<int(int, void *)> allocator){
    memoryWordSize = wordSize;
    defaultAllocator = allocator;
    totalSize = 0;
    memoryStart = nullptr;
    memoryBlock = nullptr;
    list = nullptr;
    bitmap = nullptr;
}

MemoryManager::~MemoryManager(){ 

    if (memoryBlock != nullptr) {
        delete[] memoryBlock;
    }

    if (list != nullptr) {
        delete[] list;
    }

    if (bitmap != nullptr) {
        delete[] bitmap;
    }
    //if heap memory exists, delete and free it

}

void MemoryManager::initialize(size_t sizeInWords){
    if(memoryStart != nullptr){
        shutdown();
        initialize(sizeInWords);
    }
    if(sizeInWords > 65536){
        return;
    }

    blockVec.clear();

    totalSize = (sizeInWords * memoryWordSize);
    memoryBlock = new bool[totalSize];
    memoryStart = (bool*)memoryBlock;
    list = new uint16_t[totalSize];
    bitmap = new uint8_t[totalSize];
    //initialization of all heap memory

    return;

    if(!memoryBlock){
        perror("Memory Block failed to initialize");
        return;
    }
}

void MemoryManager::shutdown(){ 

    //loop through memorystart and call free everywhere there is memory
    if(memoryBlock){
        bool* temp = memoryStart;
    
        for(unsigned int i = 0; i < totalSize; i++){
            if(temp){
                free(temp);
            }
            temp++;
        }

        temp = nullptr;
        blockVec.clear();
        totalSize = 0;
        list = nullptr;
        bitmap = nullptr;
        memoryBlock = nullptr;
        memoryStart = nullptr;

        return;
    }
    
}

void MemoryManager::addBlock(uint16_t index, uint16_t length){

    blockVec.push_back(make_pair(index, length));
    sort(blockVec.begin(), blockVec.end());
    //sort if we are adding in specific place (in between other blocks)
    return;
}

void MemoryManager::removeBlock(uint16_t index, uint16_t length){

    //find block with that index
    for(unsigned int i = 0; i < blockVec.size(); i++){
        if(blockVec.at(i).first == index){
            blockVec.erase(blockVec.begin()+i);
            sort(blockVec.begin(), blockVec.end());
        }
    }
    
    return;
}

void* MemoryManager::allocate(size_t sizeInBytes){ 

    int spotStart = defaultAllocator((sizeInBytes/memoryWordSize), getList());
    //find best index to put mem

    if(spotStart == -1){ 
        return nullptr;
    } else{
        addBlock(spotStart, (sizeInBytes/memoryWordSize));
    }

    //find where we actually put it and return that address
    bool* ret = memoryStart;
    for(unsigned int i = 0; i < (spotStart*memoryWordSize); i++){
        ret++;
    }

    for (unsigned int i = spotStart; i < spotStart + (sizeInBytes / memoryWordSize); i++) {
        memoryBlock[i] = true; // Mark as allocated
    }

    return ret;

}

void MemoryManager::free(void* address){ 

    bool* addr = static_cast<bool*>(address);

    for (unsigned int i = 0; i < blockVec.size(); i++) {
        bool* blockAddr = memoryStart + (blockVec[i].first * memoryWordSize);

        if (blockAddr == addr) {
            removeBlock(blockVec[i].first, blockVec[i].second);
            //update blocklist

            // Update memoryBlock
            for (unsigned int j = 0; j < blockVec[i].second; j++) {
                memoryBlock[blockVec[i].first + j] = false; // free the actual memory
            }
            return; 
        }
    }

}

void MemoryManager::setAllocator(std::function<int(int, void*)> allocator){
    defaultAllocator = allocator;
}

int MemoryManager::dumpMemoryMap(char* filename){ 

   int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0777);
   //posix call

    if (fd > -1) {

        int numHoles = 0;
        uint16_t* holeList = static_cast<uint16_t*>(getList());
        //grab list of holes

        if (holeList != nullptr) {
            numHoles = holeList[0];
        }

        for (unsigned int i = 1; i <= numHoles; ++i) {
            dprintf(fd, "[%d, %d]", holeList[i * 2 - 1], holeList[i * 2]);

            if (i < numHoles) {
                dprintf(fd, " - ");
            }
        }
        //print holes accordingly

        close(fd);
        return 0;

    } else {
        return -1;
    }

    // Use write to print the list of holes to the file
    
}

void* MemoryManager::getList(){

    int numHoles = 0;

    //test for invalid allocate
    int tempy = 0;
    for(unsigned int i = 0; i < blockVec.size(); i++){
        tempy += blockVec.at(i).second;
    }

    if(tempy == totalSize/memoryWordSize){
        cout << "No room! All full" << endl;
        return nullptr;
    }
    

    //one big hole
    if(blockVec.empty()){
        list[0] = 1;
        list[1] = 0;
        list[2] = (totalSize/memoryWordSize);
        return list;
    }

    //cases for only one block
    if(blockVec.size() == 1){
        //block at the start, hole at the end
        if(blockVec.at(0).first == 0){
            numHoles = 1;
            list[0] = numHoles;
            list[1] = blockVec.at(0).second;
            list[2] = (totalSize/memoryWordSize) - blockVec.at(0).second;
            return list;
        //block at the end, hole at start
        } else if(blockVec.at(0).first == ((totalSize/memoryWordSize) - (blockVec.at(0).second))){
            numHoles = 1;
            list[0] = numHoles;
            list[1] = 0;
            list[2] = blockVec.at(0).first;
            return list;
        //block in middle, surrounded by 2 holes
        } else{
            numHoles = 2;
            list[0] = numHoles;
            list[1] = 0;
            list[2] = blockVec.at(0).first;
            list[3] = blockVec.at(0).first + blockVec.at(0).second;
            list[4] = ((totalSize/memoryWordSize) - (blockVec.at(0).second));
            return list;
        }
    }

    //IF HERE, COUNTING HOLES
    //hole at very start
    if(blockVec.at(0).first != 0){
        numHoles++;
    }

    //hole at very end
    if((blockVec.at(blockVec.size()-1).first + blockVec.at(blockVec.size()-1).second) != (totalSize/memoryWordSize)){
        numHoles++;
    }

    //holes between two blocks
    for(unsigned int i = 1; i < blockVec.size(); i++){
        if(blockVec.at(i-1).first + blockVec.at(i-1).second < blockVec.at(i).first){
            numHoles++;
        }
    }

    list = new uint16_t[(numHoles*2) + 1];
    list[0] = numHoles;
    int listIndex = 1;

    //hole at the start
    if(blockVec.at(0).first != 0){
        list[listIndex] = 0;
        listIndex++;
        list[listIndex] = blockVec.at(0).first;
        listIndex++;
    }

    for(unsigned int i = 1; i < blockVec.size(); i++){ 

        //hole between two blocks
        if(blockVec.at(i-1).first + blockVec.at(i-1).second < (blockVec.at(i).first)){
            list[listIndex] = blockVec.at(i-1).first + blockVec.at(i-1).second;
            listIndex++;
            list[listIndex] = blockVec.at(i).first - list[listIndex-1];
            listIndex++;
        } 
    }

    //hole at the very end
    if((blockVec.at(blockVec.size()-1).first + blockVec.at(blockVec.size()-1).second) != (totalSize/memoryWordSize)){
        list[listIndex] = (blockVec.at(blockVec.size()-1).first + blockVec.at(blockVec.size()-1).second);
        listIndex++;
        list[listIndex] = (totalSize/memoryWordSize) - list[listIndex-1];
    }


    return list;
}

//Found online from geeks for geeks, simple algorithm to convert 8 bit bytes to decimal for getBitmap
int MemoryManager::byteToDecimal(string byte){
    int dec = 0;
    for (int i = byte.size() - 1; i >= 0; --i) {
        int bit = byte[i] - '0';
        dec += bit << (byte.size() - 1 - i);
    }
    return dec;
}

void* MemoryManager::getBitmap(){ 
    
    if(memoryBlock){
        
        int numWords = totalSize / memoryWordSize; //26 words for example
        int extraSize = numWords % 8; //should be equal to 2 (extra bits to add on)
        int numBytes = numWords / 8; //should be equal to 3
        
        if(extraSize != 0){
            numBytes++; //should now be 4
        }

        bitmap = new uint8_t[(numBytes+2)];

        uint8_t sizeByte1 = static_cast<uint8_t>(numBytes & 0xFF);       // least sig
        uint8_t sizeByte2 = static_cast<uint8_t>((numBytes >> 8) & 0xFF); // most sig
        
        bitmap[0] = byteToDecimal(bitset<8>(sizeByte1).to_string());
        bitmap[1] = byteToDecimal(bitset<8>(sizeByte2).to_string());
        //First two indexes in array are now set

        string theRest(numBytes * 8, '0');
        //initialize large string for holes all equalling 0

        for (const auto& block : blockVec) {
            for (int j = block.first; j < block.first + block.second; ++j) {
                if (j < numBytes * 8) {
                    theRest[j] = '1';
                }
            }
        }

        for (unsigned int i = 0; i < numBytes; ++i) {
            // Create a string from the substring of theRest
            string substring = theRest.substr(i * 8, 8);

            // Reverse the substring
            reverse(substring.begin(), substring.end());

            //convert to decimal and put in array
            bitmap[i+2] = byteToDecimal(substring);
        }

        return bitmap;

    } else{
        return nullptr;
    }

}

//simple getter functions

unsigned MemoryManager::getWordSize(){
    return memoryWordSize;
}

void* MemoryManager::getMemoryStart(){
    return memoryStart;
}

unsigned MemoryManager::getMemoryLimit(){
    return totalSize;
}

void MemoryManager::printVec(){ 

    //Used for debugging purposes

    if(blockVec.size() > 0){
        for(unsigned int i = 0; i < blockVec.size(); i++){
            cout << "[" << blockVec.at(i).first << "] - [" << blockVec.at(i).second << "], ";
        }
        cout << endl;

        return;
    } else{
        cout << "BlockVec is empty, nothing to print" << endl;
        return;
    }
    
}

