#ifndef __CPU_PRED_GSELECT_HH__
#define __CPU_PRED_GSELECT_HH__

#include "base/sat_counter.hh"
#include "base/types.hh"
#include "cpu/pred/bpred_unit.hh"
#include "params/GSelectBP.hh"
#include "debug/GSelect.hh"

class GSelectBP : public BPredUnit
{
    public:
        /**
         * Default branch predictor constructor.
         */
        GSelectBP(const GSelectBPParams *params);

        /**
         * Restore the global history register with the history recorded before prediction.
         * @param bp_history The address of custom structure contatining branch predictor history.
         */
        void squash(ThreadID tid, void *bp_history);

        /**
         * Updates the branch predictor with the actual result of a branch.
         * @param branch_addr The address of the branch to update.
         * @param taken Whether or not the branch was taken.
         * @param bp_history The address of custom structure contatining branch predictor history.
         * @param squashed Whether any outstanding updates are squashed or not.
         */
        void update(ThreadID tid, Addr branch_addr, bool taken, void *bp_history,
                    bool squashed, const StaticInstPtr & inst, Addr corrTarget);
        
        /**
         * Updates the global history register with taken.
         * @param branch_addr The address of the branch to update.
         * @param bp_history The address of custom structure contatining branch predictor history.
         */
        void uncondBranch(ThreadID tid, Addr pc, void * &bp_history);

        /**
         * Updates the branch predictor to Not Taken if a BTB entry is
         * invalid or not found.
         * @param branch_addr The address of the branch to look up.
         * @param bp_history Pointer to any bp history state.
         * @return Whether or not the branch is taken.
         */
        void btbUpdate(ThreadID tid, Addr branch_addr, void * &bp_history);

        /**
         * Looks up the given address in the branch predictor and returns
         * a true/false value as to whether it is taken.
         * @param branch_addr The address of the branch to look up.
         * @param bp_history Pointer to any bp history state.
         * @return Whether or not the branch is taken.
         */
        bool lookup(ThreadID tid, Addr branch_addr, void * &bp_history);

    private: 
        /**
         * Updates the global history register with parameter value taken.
         * a true/false value as to whether it is taken.
         * @param branch_addr The address of the branch to look up.
         * @param taken Whether or not the branch was taken.
         */
        void updateGlobalHistReg(ThreadID tid, bool taken);

        /**
         *  Returns the taken/not taken prediction given the value of the
         *  counter.
         *  @param count The value of the counter.
         *  @return The prediction based on the counter value.
         */
        inline bool getPrediction(uint8_t &count);

        /**
         *  Returns the index of the predictor table given the value of the
         *  program counter and global history.
         *  @param branch_addr The address of the branch.
         *  @param programCounterMask The mask to determine how many bits of branch_addr to consider.
         *  @param globalHistory Global history register's value.
         *  @return The index of the predictor table.
         */
        inline unsigned getIndex(Addr &branch_addr, unsigned programCounterMask, unsigned globalHistory);

        /* Branch prediction history structure to hold the value of global history register in case the prediction get's squashed.  */
        struct BPHistory {
            unsigned globalHistoryReg;
        };

        /* Size of local Predictor */    
        unsigned CounterPredictorSize;

        /* Size of counter bits */ 
        unsigned CounterCtrBits;

        /* Size of global Predictor */ 
        unsigned globalHistoryBits;
        
        unsigned n;                                 // Branch address bits to be considered for indexing predictor table 
        unsigned programCounterMask;                // Mask to determine how many bits of branch_addr to consider.
        unsigned historyRegisterMask;               // Mask to determine how many bits of globalHistoryReg to consider.
        unsigned indexMask;                         // Mask to determine how many bits of concatenated index to consider.
        std::vector<SatCounter> counters;           // Vector to store the predictions
        std::vector<unsigned> globalHistoryReg;     // Global History Register
        
};

#endif // __CPU_PRED_GSELECT_HH__