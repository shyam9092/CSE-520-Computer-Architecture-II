#include "mem/cache/replacement_policies/lru_ipv.hh"

#include <cassert>
#include <memory>

#include "params/LRUIPVRP.hh"
#include "sim/core.hh"
#include "debug/LRUIPVDebug.hh"
#include "base/trace.hh"

/* Constructor for LRUIPVReplData structure */
LRUIPVRP::LRUIPVReplData::LRUIPVReplData(int blockIndex,std::shared_ptr<std::vector<int>> position):blockIndex(blockIndex), position(position) {}

/* Constructor for class LRUIPVRP */
LRUIPVRP::LRUIPVRP(const Params *p)
    : BaseReplacementPolicy(p), blockCount(0), ptr_LRUIPVCache(nullptr)
{
}

/* The functionality of invalidate is provided in getVictim() */
void LRUIPVRP::invalidate(const std::shared_ptr<ReplacementData>& replacement_data) const
{
}

void
LRUIPVRP::touch(const std::shared_ptr<ReplacementData>& replacement_data) const
{
    std::shared_ptr<LRUIPVReplData> replacementData = std::static_pointer_cast<LRUIPVReplData>(replacement_data); // Get pointer to block data's structure
    std::vector<int> *ptr = replacementData->position.get(); // Get pointer to vector containing infromation about positions of each blocks
    int position = IPV_Graph[replacementData->blockIndex]; // Get new position from IPV graph using block index stored in the structure
    int old = ptr->at(replacementData->blockIndex); // Get the current position of the block from the vector containing infromation about positions of each block
    DPRINTF(LRUIPVDebug,"[block index %d] position:%d \t new position %d \n",replacementData->blockIndex,old,position);
    ptr->at(replacementData->blockIndex) = position; // Update the position of the block in the vector containing information about positions of each block
    int vectorSize = ptr->size(); // Get the vector size to iterate over and shift the positions of the block by 1 wherever necessary
    int index=0;

    while(index < vectorSize) // Iterate till the end of the vector
    {
        if((ptr->at(index) >= position) && (ptr->at(index) < old)) // Need to update the position of blocks between new position and old position of the block calling touch
        {   
            
            ptr->at(index) = ptr->at(index) + 1; // Move the position of blocks right by 1
        }
        index++;
    }
}   

void
LRUIPVRP::reset(const std::shared_ptr<ReplacementData>& replacement_data) const
{
    std::shared_ptr<LRUIPVReplData> replacementData = std::static_pointer_cast<LRUIPVReplData>(replacement_data); // Get pointer to block data's structure
    std::vector<int> *ptr = replacementData->position.get(); // Get pointer to vector containing infromation about positions of each blocks
    int vectorSize = ptr->size(); // Get the vector size to iterate over and shift the positions of the block by 1 wherever necessary
    int index=0;

    while(index  < vectorSize)  // Iterate till the end of the vector
    {   
        // The position of blocks >= 13 needs to be shifted by 1 
        if((ptr->at(index) >= IPV_Graph[associativity]) && (ptr->at(index) != -1)) // Each block will contain -1 as it's initial positon when initialized. To avoid shifting these blocks, index != -1 condition is placed 
        {
            DPRINTF(LRUIPVDebug,"block # shifted by 1:%d\n",ptr->at(index));
            ptr->at(index) = ptr->at(index) + 1;
        }
        index++;
    }
    ptr->at(replacementData->blockIndex) = IPV_Graph[associativity]; // Assign 13 as position to the block calling reset
}

ReplaceableEntry*
LRUIPVRP::getVictim(const ReplacementCandidates& candidates) const
{
    // There must be at least one replacement candidate
    assert(candidates.size() > 0);
    DPRINTF(LRUIPVDebug,"LRUIPVRP::getVictim \n");
    std::shared_ptr<LRUIPVReplData> replacementData = std::static_pointer_cast<LRUIPVReplData>(candidates[0]->replacementData); // Get pointer to block data's structure
    std::vector<int> *ptr = replacementData->position.get(); // Get pointer to vector containing infromation about positions of each blocks
    int vectorSize = ptr->size(); // Get the vector size to iterate over and find the victim block's index
    int index=0; // Variable to iterate over all the blocks
    int victim=0;   // Variable to store the victim block's index
    int maxPosition=0; // Variable to store and compare block's current position

    while(index < vectorSize)
    {
        if(maxPosition < ptr->at(index))  // If the position is found to be greater, then that is our victim block
        {
            maxPosition = ptr->at(index); // Store the position to be referenced again
            victim = index; // Store the index of victim block's index
        }
        index++;
    }
    DPRINTF(LRUIPVDebug,"Victim:%d \n",victim);
    return candidates[victim]; // Return the victim block
}

std::shared_ptr<ReplacementData>
LRUIPVRP::instantiateEntry()
{   
    DPRINTF(LRUIPVDebug,"LRUIPVRP::instantiateEntry \n");
    if(blockCount % associativity == 0)  // 16 blocks will be assigned to same vector 
    {
        DPRINTF(LRUIPVDebug,"new vector created \n");
        ptr_LRUIPVCache = new std::vector<int>(associativity,-1); // create new vector whenver multiples of 16 blocks are created
    }
    /* Initialize the structure containing information about block's data. Index is equated to number of blocks initialized + 1 */
    LRUIPVReplData *data = new LRUIPVReplData(blockCount % associativity, std::shared_ptr<std::vector<int>>(ptr_LRUIPVCache)); // Same pointer to the vector containing information about each block is shared among the 16 blocks
    blockCount++; // Increase block count every time instantiate entry is invoked
    DPRINTF(LRUIPVDebug,"blockCount:%d \n",blockCount);
    return std::shared_ptr<ReplacementData>(data);
}

LRUIPVRP*
LRUIPVRPParams::create()
{
    return new LRUIPVRP(this);
}