#include <stdio.h>
#include <unistd.h>
#include <functional>
#include <fcntl.h>
#include <string>
#include <sstream>
#include <iostream>
#include <array>
#include <vector>
#include <utility>
using namespace std;

class MemoryManager{
private:

    function<int(int, void*)> defaultAllocator;
    unsigned memoryWordSize; //unsigned = unsigned int
    unsigned totalSize;
    bool* memoryStart; //bool = 1 byte for memory management
    bool* memoryBlock;
    vector<pair<uint16_t, uint16_t>> blockVec; //for storing blocks
    uint16_t* list; //for storing holes
    uint8_t* bitmap; //for getBitmap

public:

    MemoryManager(unsigned int wordSize, function<int(int, void *)> allocator); 
    ~MemoryManager();

    void initialize(size_t sizeInWords); 
    void shutdown();
    void *allocate(size_t sizeInBytes); 
    void free(void *address); 
    void setAllocator(std::function<int(int, void*)> allocator); 
    int dumpMemoryMap(char *filename); 

    //Custom helper functions

    void removeBlock(uint16_t index, uint16_t length);
    void addBlock(uint16_t index, uint16_t length);  
    int byteToDecimal(string byte); 
    void printVec();

    void *getList(); 
    void *getBitmap(); 
    unsigned getWordSize(); 
    void *getMemoryStart(); 
    unsigned getMemoryLimit(); 
};

//Memory Allocation Algorithms
int bestFit(int sizeInWords, void *list){

    if(list != nullptr){

        uint16_t* holeList = static_cast<uint16_t*>(list); //convert void* to int*
        int numHoles = holeList[0];

        if(numHoles == 0){
            perror("No Holes in List");
            return -1;
        }

        int bestOffset = -1;
        int bestSize = 100000000; //initialize to abnormally large value
        int tempOffset;
        int tempSize;
        
        for(unsigned int i = 0; i < numHoles; i++){ //checking each hole data
            tempOffset = holeList[(i*2) + 1];
            tempSize = holeList[(i*2)  + 2];

            if(tempSize < bestSize && tempSize >= sizeInWords){ //size is smallest while still holding word
                bestOffset = tempOffset;
                bestSize = tempSize;
            }
        }

        if(bestOffset < 0){
            perror("No available hole large enough");
            return -1;
        } else{
            return bestOffset;
        }
        //Error handling for if no holes available or invalid list

    } else{
        perror("Invalid List");
        return -1;
    }
}

int worstFit(int sizeInWords, void *list){
    
    if(list != nullptr){

        uint16_t* holeList = static_cast<uint16_t*>(list); 
        int numHoles = holeList[0];

        if(numHoles == 0){
            perror("No Holes in List");
            return -1;
        }

        //all same logic from bestFit
        int worstSize = 0;
        int worstOffset;
        int tempSize;
        int tempOffset;

        for(unsigned int i = 0; i < numHoles; i++){
            tempOffset = holeList[(i*2) + 1];
            tempSize = holeList[(i*2)  + 2];

            if(worstSize < tempSize){ //finding largest possible hole
                worstSize = tempSize;
                worstOffset = tempOffset;
            }
        }

        if(worstSize >= sizeInWords){
            return worstOffset;
        } else{
            perror("No available hole large enough");
            return -1;
        }

        return worstOffset;

    } else{
        perror("Invalid List");
        return -1;
    }

}