#include "cpu/pred/gselect.hh"
#include "base/intmath.hh"

GSelectBP::GSelectBP(const GSelectBPParams *params)           // Default constructor
    : BPredUnit(params),
    globalHistoryReg(params->numThreads, 0),                  // Assigning the size of global history register vector to number of threads
    CounterCtrBits(params->CounterCtrBits),                   
    CounterPredictorSize(params->CounterPredictorSize),
    globalHistoryBits(params->globalHistoryBits),
    counters(CounterPredictorSize, SatCounter(CounterCtrBits)) // Assigning the predictor counter's vector variable with CounterPredictorSize and initialzing saturation counter.

{
    DPRINTF(GSelect,"globalHistoryBits=%u\n",globalHistoryBits);
    DPRINTF(GSelect,"CounterCtrBits=%u\n",CounterCtrBits);
    DPRINTF(GSelect,"CounterPredictorSize=%u",CounterPredictorSize);
    /* Check if the size of predictor table is a power of 2 or not */
    if(!isPowerOf2(CounterPredictorSize)) {
        fatal("Invalid local predictor size!\n");
    }

    n = ceilLog2(CounterPredictorSize);
    DPRINTF(GSelect,"ceil of CounterPredictorSize: %u \n",n);
    n = n - globalHistoryBits;                                  // Calculating the number of bits of branch addres to consider
    DPRINTF(GSelect,"Size of n: %u \n",n);
    programCounterMask = mask(n);                               // Generating the mask for branch address
    
    historyRegisterMask = mask(globalHistoryBits);              // Generating the mask for global history register

    indexMask = mask(n+globalHistoryBits);                      // Generating the mask for final index of predictor table
}

/**
 * Looks up the given address in the branch predictor and returns
 * a true/false value as to whether it is taken.
 * @param branch_addr The address of the branch to look up.
 * @param bp_history Pointer to any bp history state.
 * @return Whether or not the branch is taken.
 */
bool GSelectBP::lookup(ThreadID tid, Addr branch_addr, void * &bp_history)
{
    bool taken;                                                                 // Variable to store and return the prediction obtained from prediction table
    DPRINTF(GSelect,"lookup \n");
    unsigned globalHistory = globalHistoryReg[tid];                             // Fetch the global history from global history register
    unsigned index = getIndex(branch_addr, programCounterMask, globalHistory);  // Generate index based on global history and branch address
    uint8_t counter_val = counters[index];                                      // Get the counter value from the predictor table
    
    BPHistory *history = new BPHistory;                                         // Create a new history for the case of squash
    history->globalHistoryReg = globalHistoryReg[tid];                          // Store the new history
    bp_history = static_cast<void*>(history);                                   // Assign the address of bp_history to the new history created

    taken = getPrediction(counter_val);                                         // Get the prediction based on the counter value        
    DPRINTF(GSelect,"prediction = %d \n",taken);
    updateGlobalHistReg(tid, taken);                                            // Update the global history table with the new prediction value
    DPRINTF(GSelect,"exiting lookup \n");
    return taken;                                                               // Return the prediction
}

/**
 * Updates the branch predictor to Not Taken if a BTB entry is
 * invalid or not found.
 * @param branch_addr The address of the branch to look up.
 * @param bp_history Pointer to any bp history state.
 * @return Whether or not the branch is taken.
 */
void GSelectBP::btbUpdate(ThreadID tid, Addr branchAddr, void * &bpHistory)
{
    DPRINTF(GSelect,"btbUpdate \n");
    globalHistoryReg[tid] &= (historyRegisterMask & ~ULL(1));     // Update the global history register with not taken value
    DPRINTF(GSelect,"exiting btbUpdate \n");
}

/**
 * Updates the branch predictor with the actual result of a branch.
 * @param branch_addr The address of the branch to update.
 * @param taken Whether or not the branch was taken.
 * @param bp_history The address of custom structure contatining branch predictor history.
 * @param squashed Whether any outstanding updates are squashed or not.
 */
void GSelectBP::update(ThreadID tid, Addr branch_addr, bool taken, void *bp_history, bool squashed, const StaticInstPtr & inst, Addr corrTarget)
{
    assert(bp_history);                                       // Check if bp_history is null, if it is null then generate error. 

    BPHistory *history = static_cast<BPHistory*>(bp_history);  // Create a new history for the case of squash
    
    DPRINTF(GSelect,"update \n");
    if (squashed) {
        // We do not update the counters speculatively on a squash.
        // We just restore the global history register.
        globalHistoryReg[tid] = (history->globalHistoryReg << 1) | taken;
        globalHistoryReg[tid] &= historyRegisterMask;
        DPRINTF(GSelect,"squashed \n");
        return;
    }


    unsigned globalHistory = globalHistoryReg[tid];                                 // Fetch the global history from global history register
    unsigned index = getIndex(branch_addr, programCounterMask, globalHistory);      // Generate index based on global history and branch address
    DPRINTF(GSelect, "Looking up index %#x\n", index);

    if(taken)                                                                       // If the update is taken, then increment the counter located at the index.
    {
        DPRINTF(GSelect,"Branch updated as taken.\n");
        counters[index]++;  
    }   
    else                                                                            // If the update is not taken, then decrement the counter located at the index.
    {
        DPRINTF(GSelect,"Branch updated as not taken.\n");
        counters[index]--;
    }
    updateGlobalHistReg(tid, taken);                                                // Update the global history register with the updated value

    delete history;                                                                 // Delete history

    DPRINTF(GSelect,"exiting update \n");
}

/**
 * Restore the global history register with the history recorded before prediction.
 * @param bp_history The address of custom structure contatining branch predictor history.
 */
void GSelectBP::squash(ThreadID tid, void *bp_history)
{
    DPRINTF(GSelect,"squash \n");
    if(bp_history==NULL)                       // If there is no history, then return.
    {
        DPRINTF(GSelect,"bpHistory==NULL \n");
        DPRINTF(GSelect,"exiting squash \n");        
        return;
    }
    BPHistory *history = static_cast<BPHistory*>(bp_history);
    globalHistoryReg[tid] = history->globalHistoryReg;        // Update the global history register with the history recorded before generating prediction in lookup.
    delete history;
    DPRINTF(GSelect,"exiting squash \n");
}

/**
 * Updates the global history register with taken.
 * @param branch_addr The address of the branch to update.
 * @param bp_history The address of custom structure contatining branch predictor history.
 */
void GSelectBP::uncondBranch(ThreadID tid, Addr pc, void * &bpHistory)
{
    BPHistory *history = new BPHistory;                                     // Create a new history for the case of squash
    history->globalHistoryReg = globalHistoryReg[tid];                      // Store the new history
    DPRINTF(GSelect,"uncondBranch: Updating global history as taken.\n");   
    bpHistory = static_cast<void*>(history);                                // Assign the address of bp_history to the new history created
    updateGlobalHistReg(tid,1);                                             // Update the global history register with taken
    DPRINTF(GSelect,"exiting uncondBranch \n");
}

/**
 *  Returns the taken/not taken prediction given the value of the
 *  counter.
 *  @param count The value of the counter.
 *  @return The prediction based on the counter value.
 */
inline bool GSelectBP::getPrediction(uint8_t &count)            
{
    
    DPRINTF(GSelect,"getPrediction \n");
    return (count >> (CounterCtrBits - 1));          // Get the MSB of the count   
    DPRINTF(GSelect,"exiting getPrediction \n");
}

/**
 *  Returns the index of the predictor table given the value of the
 *  program counter and global history.
 *  @param branch_addr The address of the branch.
 *  @param programCounterMask The mask to determine how many bits of branch_addr to consider.
 *  @param globalHistory Global history register's value.
 *  @return The index of the predictor table.
 */
unsigned GSelectBP::getIndex(Addr &branch_addr, unsigned programCounterMask, unsigned globalHistory)
{
    DPRINTF(GSelect,"getIndex \n");
    unsigned index;
    DPRINTF(GSelect,"programCounterMask=%u \t globalHistory = %u \t branch_addr= %u \n",programCounterMask,globalHistory,branch_addr);
    unsigned temp = (branch_addr >> instShiftAmt) & programCounterMask;   // Right shift the branch address by instShiftAmt(2) to remove 00 (because of word addressable addresses)
    DPRINTF(GSelect,"branch_addr >> 2 = %u \n",temp);
    index = (temp << globalHistoryBits) | globalHistory;        // Left shift the branch address by the number of bits in global history register and then concatenate it with global history
    index &= indexMask;                                         // Mask the index so that result contains m+n bits only.
    DPRINTF(GSelect,"exiting getIndex \n");
    return index; 
}

/**
 * Updates the global history register with parameter value taken.
 * a true/false value as to whether it is taken.
 * @param branch_addr The address of the branch to look up.
 * @param taken Whether or not the branch was taken.
 */
void GSelectBP::updateGlobalHistReg(ThreadID tid, bool taken)
{
    DPRINTF(GSelect,"updateGlobalHistory \n");
    DPRINTF(GSelect,"TID: %d \t TAKEN=%d \n",tid,taken);
    globalHistoryReg[tid] = taken ? (globalHistoryReg[tid] << 1) | 1 : (globalHistoryReg[tid] << 1);  // Left shift the global history register and or it with 1 if taken is true.
    globalHistoryReg[tid] &= historyRegisterMask;
    DPRINTF(GSelect,"exiting updateGlobalHistory \n");
}

GSelectBP* GSelectBPParams::create()
{
    return new GSelectBP(this);
}

