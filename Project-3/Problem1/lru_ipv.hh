#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_LRU_IPV_RP_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_LRU_IPV_RP_HH__

#include "mem/cache/replacement_policies/base.hh"



struct LRUIPVRPParams;

class LRUIPVRP : public BaseReplacementPolicy
{

    protected:
    
    int blockCount;   // Variable to keep track of total number of blocks initialized/created. # of blocks = associativity should be indexed by same object
    
    const std::vector<int> IPV_Graph {0,0,1,0,3,0,1,2,1,0,5,1,0,0,1,11,13}; // Insertion and promotion vector to be followed

    const int associativity = 16; // Set associativity

    std::vector<int> *ptr_LRUIPVCache;  // Pointer to vector which will contain current positions of each block initialized

    struct LRUIPVReplData : ReplacementData     // Structure to store information about individual blocks
    {
        std::shared_ptr<std::vector<int>> position;  // Shared pointer among objects to keep track of 16 blocks
        mutable int blockIndex;   // Index of a block to be used as an index to extract new position from IPV
        LRUIPVReplData(int blockIndex,std::shared_ptr<std::vector<int>> position); // Default constructor for the structure
    };

    public:

    typedef LRUIPVRPParams Params;

    LRUIPVRP(const Params *p);

    /**
     * Invalidate replacement data to set it as the next probable victim.
     * Sets its last touch tick as the starting tick.
     *
     * @param replacement_data Replacement data to be invalidated.
     */
    void invalidate(const std::shared_ptr<ReplacementData>& replacement_data)
                                                              const override;

    /**
     * Touch an entry to update its replacement data.
     * Sets its last touch tick as the current tick.
     *
     * @param replacement_data Replacement data to be touched.
     */
    void touch(const std::shared_ptr<ReplacementData>& replacement_data) const
                                                                     override;

    /**
     * Reset replacement data. Used when an entry is inserted.
     * Sets its last touch tick as the current tick.
     *
     * @param replacement_data Replacement data to be reset.
     */
    void reset(const std::shared_ptr<ReplacementData>& replacement_data) const
                                                                     override;

    /**
     * Find replacement victim using LRU timestamps.
     *
     * @param candidates Replacement candidates, selected by indexing policy.
     * @return Replacement entry to be replaced.
     */
    ReplaceableEntry* getVictim(const ReplacementCandidates& candidates) const
                                                                     override;

    /**
     * Instantiate a replacement data entry.
     *
     * @return A shared pointer to the new replacement data.
     */
    std::shared_ptr<ReplacementData> instantiateEntry() override;

};




#endif // __MEM_CACHE_REPLACEMENT_POLICIES_LRU_IPV_RP_HH__