// Copyright (c) 2016-2019 The CryptoCoderz Team / Espers
// Copyright (c) 2018-2019 The CryptoCoderz Team / INSaNe project
// Copyright (c) 2018-2019 The Rubix project
// Copyright (c) 2018-2019 The FerretCoin project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockparams.h"
#include "chainparams.h"
#include "checkpoints.h"
#include "db.h"
#include "init.h"
#include "kernel.h"
#include "net.h"
#include "txdb.h"
#include "velocity.h"
#include "main.h"
#include "mnengine.h"
#include "masternodeman.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

using namespace std;
using namespace boost;

//////////////////////////////////////////////////////////////////////////////
//
// Standard Global Values
//

//
// Section defines global values for retarget logic
//

double VLF1 = 0;
double VLF2 = 0;
double VLF3 = 0;
double VLF4 = 0;
double VLF5 = 0;
double VLFtmp = 0;
double VRFsm1 = 1;
double VRFdw1 = 0.75;
double VRFdw2 = 0.5;
double VRFup1 = 1.25;
double VRFup2 = 1.5;
double VRFup3 = 2;
double TerminalAverage = 0;
double TerminalFactor = 10000;
CBigNum newBN = 0;
CBigNum oldBN = 0;
int64_t VLrate1 = 0;
int64_t VLrate2 = 0;
int64_t VLrate3 = 0;
int64_t VLrate4 = 0;
int64_t VLrate5 = 0;
int64_t VLRtemp = 0;
int64_t DSrateNRM = BLOCK_SPACING;
int64_t DSrateMAX = BLOCK_SPACING_MAX;
int64_t FRrateDWN = DSrateNRM - 60;
int64_t FRrateFLR = DSrateNRM - 90;
int64_t FRrateCLNG = DSrateMAX + 180;
int64_t difficultyfactor = 0;
int64_t AverageDivisor = 5;
int64_t scanheight = 6;
int64_t scanblocks = 1;
int64_t scantime_1 = 0;
int64_t scantime_2 = 0;
int64_t prevPoW = 0; // hybrid value
int64_t prevPoS = 0; // hybrid value
const CBlockIndex* pindexPrev = 0;
const CBlockIndex* BlockVelocityType = 0;
CBigNum bnOld;
CBigNum bnNew;
unsigned int retarget = DIFF_VRX; // Default with VRX
uint64_t cntTime = 0;
uint64_t prvTime = 0;
uint64_t difTimePoS = 0;
uint64_t difTimePoW = 0;
string loggedpayee = "";


//////////////////////////////////////////////////////////////////////////////
//
// Debug section
//

//
// Debug log printing
//

void VRXswngPoSdebug()
{
    // Print for debugging
    LogPrintf("Previously discovered PoS block: %u: \n",prvTime);
    LogPrintf("Current block-time: %u: \n",cntTime);
    LogPrintf("Time since last PoS block: %u: \n",difTimePoS);
    if(difTimePoS > 1 * 60 * 60) { TerminalAverage /= 2; LogPrintf("diffTimePoS is greater than 1 Hours: %u \n",cntTime);}
    if(difTimePoS > 2 * 60 * 60) { TerminalAverage /= 2; LogPrintf("diffTimePoS is greater than 2 Hours: %u \n",cntTime);}
    if(difTimePoS > 3 * 60 * 60) { TerminalAverage /= 2; LogPrintf("diffTimePoS is greater than 3 Hours: %u \n",cntTime);}
    if(difTimePoS > 4 * 60 * 60) { TerminalAverage /= 2; LogPrintf("diffTimePoS is greater than 4 Hours: %u \n",cntTime);}
    return;
}

void VRXswngPoWdebug()
{
    // Print for debugging
    LogPrintf("Previously discovered PoW block: %u: \n",prvTime);
    LogPrintf("Current block-time: %u: \n",cntTime);
    LogPrintf("Time since last PoW block: %u: \n",difTimePoW);
    if(difTimePoW > 1 * 60 * 60) { TerminalAverage /= 2; LogPrintf("diffTimePoW is greater than 1 Hours: %u \n",cntTime);}
    if(difTimePoW > 2 * 60 * 60) { TerminalAverage /= 2; LogPrintf("diffTimePoW is greater than 2 Hours: %u \n",cntTime);}
    if(difTimePoW > 3 * 60 * 60) { TerminalAverage /= 2; LogPrintf("diffTimePoW is greater than 3 Hours: %u \n",cntTime);}
    if(difTimePoW > 4 * 60 * 60) { TerminalAverage /= 2; LogPrintf("diffTimePoW is greater than 4 Hours: %u \n",cntTime);}
    return;
}

void VRXdebug()
{
    // Print for debugging
    LogPrintf("Terminal-Velocity 1st spacing: %u: \n",VLrate1);
    LogPrintf("Terminal-Velocity 2nd spacing: %u: \n",VLrate2);
    LogPrintf("Terminal-Velocity 3rd spacing: %u: \n",VLrate3);
    LogPrintf("Terminal-Velocity 4th spacing: %u: \n",VLrate4);
    LogPrintf("Terminal-Velocity 5th spacing: %u: \n",VLrate5);
    LogPrintf("Desired normal spacing: %u: \n",DSrateNRM);
    LogPrintf("Desired maximum spacing: %u: \n",DSrateMAX);
    LogPrintf("Terminal-Velocity 1st multiplier set to: %f: \n",VLF1);
    LogPrintf("Terminal-Velocity 2nd multiplier set to: %f: \n",VLF2);
    LogPrintf("Terminal-Velocity 3rd multiplier set to: %f: \n",VLF3);
    LogPrintf("Terminal-Velocity 4th multiplier set to: %f: \n",VLF4);
    LogPrintf("Terminal-Velocity 5th multiplier set to: %f: \n",VLF5);
    LogPrintf("Terminal-Velocity averaged a final multiplier of: %f: \n",TerminalAverage);
    LogPrintf("Prior Terminal-Velocity: %u\n", oldBN);
    LogPrintf("New Terminal-Velocity:  %u\n", newBN);
    return;
}

void GNTdebug()
{
    // Print for debugging
    // Retarget ignoring invalid selection
    if (retarget != DIFF_VRX)
    {
        // debug info for testing
        LogPrintf("GetNextTargetRequired() : Invalid retarget selection, using default \n");
        return;
    }

    // Retarget using Terminal-Velocity
    // debug info for testing
    LogPrintf("Terminal-Velocity retarget selected \n");
    LogPrintf("Espers retargetted using: Terminal-Velocity difficulty curve \n");
    return;
}


//////////////////////////////////////////////////////////////////////////////
//
// Difficulty retarget (current section)
//

//
// This is VRX revised implementation
//
// Terminal-Velocity-RateX, v10-Beta-R7, written by Jonathan Dan Zaretsky - cryptocoderz@gmail.com
void VRX_BaseEngine(const CBlockIndex* pindexLast, bool fProofOfStake)
{
       // Set base values
       VLF1 = 0;
       VLF2 = 0;
       VLF3 = 0;
       VLF4 = 0;
       VLF5 = 0;
       VLFtmp = 0;
       TerminalAverage = 0;
       TerminalFactor = 10000;
       VLrate1 = 0;
       VLrate2 = 0;
       VLrate3 = 0;
       VLrate4 = 0;
       VLrate5 = 0;
       VLRtemp = 0;
       difficultyfactor = 0;
       scanblocks = 1;
       scantime_1 = 0;
       scantime_2 = pindexLast->GetBlockTime();
       prevPoW = 0; // hybrid value
       prevPoS = 0; // hybrid value
       // Set prev blocks...
       pindexPrev = pindexLast;
       // ...and deduce spacing
       while(scanblocks < scanheight)
       {
           scantime_1 = scantime_2;
           pindexPrev = pindexPrev->pprev;
           scantime_2 = pindexPrev->GetBlockTime();
           // Set standard values
           if(scanblocks > 0){
               if     (scanblocks < scanheight-4){ VLrate1 = (scantime_1 - scantime_2); VLRtemp = VLrate1; }
               else if(scanblocks < scanheight-3){ VLrate2 = (scantime_1 - scantime_2); VLRtemp = VLrate2; }
               else if(scanblocks < scanheight-2){ VLrate3 = (scantime_1 - scantime_2); VLRtemp = VLrate3; }
               else if(scanblocks < scanheight-1){ VLrate4 = (scantime_1 - scantime_2); VLRtemp = VLrate4; }
               else if(scanblocks < scanheight-0){ VLrate5 = (scantime_1 - scantime_2); VLRtemp = VLrate5; }
           }
           // Round factoring
           if(VLRtemp >= DSrateNRM){ VLFtmp = VRFsm1;
               if(VLRtemp > DSrateMAX){ VLFtmp = VRFdw1;
                   if(VLRtemp > FRrateCLNG){ VLFtmp = VRFdw2; }
               }
           }
           else if(VLRtemp < DSrateNRM){ VLFtmp = VRFup1;
               if(VLRtemp < FRrateDWN){ VLFtmp = VRFup2;
                   if(VLRtemp < FRrateFLR){ VLFtmp = VRFup3; }
               }
           }
           // Record factoring
           if      (scanblocks < scanheight-4) VLF1 = VLFtmp;
           else if (scanblocks < scanheight-3) VLF2 = VLFtmp;
           else if (scanblocks < scanheight-2) VLF3 = VLFtmp;
           else if (scanblocks < scanheight-1) VLF4 = VLFtmp;
           else if (scanblocks < scanheight-0) VLF5 = VLFtmp;
           // Log hybrid block type
           //
           // v1.0
           if(pindexBest->GetBlockTime() < 1520198278) // ON Sunday, March 4, 2018 9:17:58 PM
           {
                if     (fProofOfStake) prevPoS ++;
                else if(!fProofOfStake) prevPoW ++;
           }
           // v1.1
           if(pindexBest->GetBlockTime() > 1520198278) // ON Sunday, March 4, 2018 9:17:58 PM
           {
               if(pindexPrev->IsProofOfStake())
               {
                   prevPoS ++;
               }
               else if(pindexPrev->IsProofOfWork())
               {
                   prevPoW ++;
               }
           }

           // move up per scan round
           scanblocks ++;
       }
       // Final mathematics
       TerminalAverage = (VLF1 + VLF2 + VLF3 + VLF4 + VLF5) / AverageDivisor;
       return;
}

void VRX_ThreadCurve(const CBlockIndex* pindexLast, bool fProofOfStake)
{
    // Run VRX engine
    VRX_BaseEngine(pindexLast, fProofOfStake);

    //
    // Skew for less selected block type
    //

    // Version 1.0
    //
    int64_t nNow = nBestHeight; int64_t nThen = 1493596800; // Toggle skew system fork - Mon, 01 May 2017 00:00:00 GMT
    if(nNow > nThen){if(prevPoW < prevPoS && !fProofOfStake){if((prevPoS-prevPoW) > 3) TerminalAverage /= 3;}
    else if(prevPoW > prevPoS && fProofOfStake){if((prevPoW-prevPoS) > 3) TerminalAverage /= 3;}
    if(TerminalAverage < 0.5) TerminalAverage = 0.5;} // limit skew to halving

    // Version 1.1 curve-patch
    //
    if(pindexBest->GetBlockTime() > 1520198278) // ON Sunday, March 4, 2018 9:17:58 PM
    {
        // Define time values
        cntTime = BlockVelocityType->GetBlockTime();
        prvTime = BlockVelocityType->pprev->GetBlockTime();

        if(fProofOfStake)
        {
            difTimePoS = cntTime - prvTime;

            // Debug print toggle
            if(fDebug) VRXswngPoSdebug();
            // Normal Run
            else if(!fDebug)
            {
                if(difTimePoS > 1 * 60 * 60) { TerminalAverage /= 2; }
                if(difTimePoS > 2 * 60 * 60) { TerminalAverage /= 2; }
                if(difTimePoS > 3 * 60 * 60) { TerminalAverage /= 2; }
                if(difTimePoS > 4 * 60 * 60) { TerminalAverage /= 2; }
            }
        }
        else if(!fProofOfStake)
        {
            difTimePoW = cntTime - prvTime;

            // Debug print toggle
            if(fDebug) VRXswngPoWdebug();
            // Normal Run
            else if(!fDebug)
            {
                if(difTimePoW > 1 * 60 * 60) { TerminalAverage /= 2; }
                if(difTimePoW > 2 * 60 * 60) { TerminalAverage /= 2; }
                if(difTimePoW > 3 * 60 * 60) { TerminalAverage /= 2; }
                if(difTimePoW > 4 * 60 * 60) { TerminalAverage /= 2; }
            }
        }
    }
    return;
}

unsigned int VRX_Retarget(const CBlockIndex* pindexLast, bool fProofOfStake)
{
    const CBigNum bnVelocity = fProofOfStake ? Params().ProofOfStakeLimit() : Params().ProofOfWorkLimit();

    // Check for blocks to index | Allowing for initial chain start
    if (pindexLast->nHeight < scanheight+124)
        return bnVelocity.GetCompact(); // can't index prevblock

    // Live fork toggle diff reset
    if(nLiveForkToggle > 0)
    {
        if(pindexLast->nHeight > nLiveForkToggle) // Selectable toggle
        {
            if(pindexLast->nHeight < nLiveForkToggle+5) {
                return bnVelocity.GetCompact(); // diff reset
            }
        }
    }

    // Differentiate PoW/PoS prev block
    BlockVelocityType = GetLastBlockIndex(pindexLast, fProofOfStake);

    // Run VRX threadcurve
    VRX_ThreadCurve(pindexLast, fProofOfStake);

    // Retarget
    TerminalFactor *= TerminalAverage;
    difficultyfactor = TerminalFactor;
    bnOld.SetCompact(BlockVelocityType->nBits);
    bnNew = bnOld / difficultyfactor;
    bnNew *= 10000;

    // Limit
    if (bnNew > bnVelocity)
      bnNew = bnVelocity;

    // Final log
    oldBN = bnOld.GetCompact();
    newBN = bnNew.GetCompact();

    // Debug print toggle
    if(fDebug) VRXdebug();

    // Return difficulty
    return bnNew.GetCompact();
}

//////////////////////////////////////////////////////////////////////////////
//
// Difficulty retarget (function)
//

unsigned int GetNextTargetRequired(const CBlockIndex* pindexLast, bool fProofOfStake)
{
    // Default with VRX
    unsigned int retarget = DIFF_VRX;

    // Check selection
    if (retarget != DIFF_VRX)
    {
        // debug info for testing
        if(fDebug) GNTdebug();
        return VRX_Retarget(pindexLast, fProofOfStake);
    }

    // Retarget using Terminal-Velocity
    // debug info for testing
    if(fDebug) GNTdebug();
    return VRX_Retarget(pindexLast, fProofOfStake);
}

//////////////////////////////////////////////////////////////////////////////
//
// Coin base subsidy
//
//
// Reward calculations for 25-years of FRTC emissions
// 10 Billion Total     | 8 Billion Premine
// 100% Remaining FRTC   : 2,000,000,000
// ----------------------------------
// Block numbers based on 2-minute blocktime average
// (Not including initial 250 starting blocks)
// Blocks per day       :     720
// Blocks per month     :  21,600
// Blocks per year      : 262,800
// ----------------------------------
// 100% for Calculations: 720 blocks per day
// Payout per block     : 300 FRTC
// Payout per day       : 300 * ((1 * 60 * 60) / (2 * 60) * 24)                   =       216,000 FRTC
// Payout per month     : 300 * (((1 * 60 * 60) / (2 * 60) * 24) * 30)            =     6,480,000 FRTC
// Payout per year      : 300 * (((1 * 60 * 60) / (2 * 60) * 24) * 365)           =    78,840,000 FRTC
// Mineout              : 300 * (25.36 * (((1 * 60 * 60) / (2 * 60) * 24) * 365)) = 2,000,000,000 FRTC
// ----------------------------------
// (Network Allocation) (BLOCKS | 25-Years of minting)
// Singular Payout      : 150-->50 FRTC
// Maternode Payout     : 100-->200 FRTC
// DevOps Payout        : 50 FRTC
// ----------------------------------
// (PLEASE NOTE)
// Masternode Payout is calculated based on the assumption of starting payout date of Masternode Payout and
// DevOps payout matching in terms of start date. 
// DevOps may or may not start at at the same time as the Masternode Payouts at which point the numbers
// will be skewed off slightly in either direction.
// This is the same for DevOps payout, it is assumed for its calculations that it starts with Masternode
// Payouts.
// ----------------------------------
// (Masternode | Network) SeeSaw
// Increment step       : 20% step
// Interval             : 30 blocks
// Step per Interval    : 1 (20% step per interval)
// Steps per swing      : 5 Steps up or down 
// Epoch (SeeSaw finish): 15 Intervals
// Upswing Duration     : 5 Intervals
// Downswing Duration   : 5 Intervals
// Idle Duration        : 5 Intervals (no adjustment)
//

int static generateMTRandom(unsigned int s, int range)
{
    random::mt19937 gen(s);
    random::uniform_int_distribution<> dist(0, range);
    return dist(gen);
}

int randreward()
{
    // Superblock calculations
    uint256 prevHash = 0;
    if(pindexBest->pprev)
        prevHash = pindexBest->pprev->GetBlockHash();
    std::string cseed_str = prevHash.ToString().substr(7,7);
    const char* cseed = cseed_str.c_str();
    long seed = hex2long(cseed);
    int rand1 = generateMTRandom(seed, 1000000);
    return rand1;
}

//
// PoW coin base reward
//
int64_t GetProofOfWorkReward(int nHeight, int64_t nFees)
{
    int64_t nSubsidy = nBlockStandardReward;

    if(nHeight > nReservePhaseStart) {
        if(pindexBest->nMoneySupply < (nBlockRewardReserve * 100)) {
            nSubsidy = nBlockRewardReserve;
        }
    }

    int chance_1 = 390000; // 39%
    int chance_2 = 10000;  // 1%
                           // 60%

    if(randreward() <= chance_2 && nHeight > nReservePhaseEnd) { // 17% Chance of superblock
        nSubsidy = 25 * COIN;
    }
    else if(randreward() <= chance_1 && nHeight > nReservePhaseEnd) {
      nSubsidy = 8 * COIN;
    }
    else {
      nSubsidy = 4 * COIN;
    }

    // hardCap v2.1
    if(pindexBest->nMoneySupply > MAX_SINGLE_TX)
    {
        LogPrint("MINEOUT", "GetProofOfWorkReward(): create=%s nFees=%d\n", FormatMoney(nFees), nFees);
        return nFees;
    }
    LogPrint("creation", "GetProofOfWorkReward() : create=%s nSubsidy=%d\n", FormatMoney(nSubsidy), nSubsidy);
    return nSubsidy + nFees;
}

//
// PoS coin base reward
//
int64_t GetProofOfStakeReward(const CBlockIndex* pindexPrev, int64_t nCoinAge, int64_t nFees)
{
    int64_t nSubsidy = nBlockStandardReward;

    if(pindexPrev->nHeight+1 > nReservePhaseStart) {
        if(pindexBest->nMoneySupply < (nBlockRewardReserve * 100)) {
            nSubsidy = nBlockRewardReserve;
        }
    }

    int64_t nHeight = pindexPrev->nHeight+1;

    int chance_1 = 390000; // 39%
    int chance_2 = 10000;  // 1%
                           // 60%

    if(randreward() <= chance_2 && nHeight > nReservePhaseEnd) { // 17% Chance of superblock
        nSubsidy = 25 * COIN;
    }
    else if(randreward() <= chance_1 && nHeight > nReservePhaseEnd) {
      nSubsidy = 8 * COIN;
    }
    else {
      nSubsidy = 4 * COIN;
    }

    // hardCap v2.1
    if(pindexBest->nMoneySupply > MAX_SINGLE_TX)
    {
        LogPrint("MINEOUT", "GetProofOfStakeReward(): create=%s nFees=%d\n", FormatMoney(nFees), nFees);
        return nFees;
    }

    LogPrint("creation", "GetProofOfStakeReward(): create=%s nCoinAge=%d\n", FormatMoney(nSubsidy), nCoinAge);
    return nSubsidy + nFees;
}

//
// Masternode coin base reward
//
int64_t GetMasternodePayment(int nHeight, int64_t blockValue)
{
    // Define values
    int64_t ret = 0;
    int64_t swingSubsidy = 2 * COIN; // 2 FRTC ceiling
    int64_t seesawHeight = nHeight;
    int64_t seesawInterval = seesawHeight / 30;
    int64_t seesawEpoch = seesawInterval / 15;
    int64_t seesawRollover = 0;
    int64_t seesawArcstart = 5;
    int64_t seesawArc = 10;
    int64_t seesawArcend = 15;
    double retDouble = 0;
    double seesawBase = 0;
    double seesawIncrement = 0.1;
    double seesawMidIncrmt = 0.5;
    double seesawCeiling = 1;
    // Set bottom of seesaw arc
    seesawBase = seesawMidIncrmt;
    // Adjust for arc epochs
    if(seesawEpoch >= 1)
    {
        seesawRollover = seesawArcend * seesawEpoch;
        seesawInterval =- seesawRollover;
    }
    // Seesaw downswing (first for logic order)
    if(seesawInterval > seesawArc)
    {
        if(seesawInterval <= seesawArcend)
        {
            seesawBase = seesawCeiling - ((seesawIncrement * seesawInterval) - 1);
            // Limit seesaw arc
            if(seesawBase < seesawMidIncrmt)
            {
                seesawBase = seesawMidIncrmt;
            }
        }
    }
    // Seesaw upswing
    else if(seesawInterval > seesawArcstart)
    {
        if(seesawInterval <= seesawArc)
        {
            seesawBase = seesawIncrement * seesawInterval;
            // Limit seesaw arc
            if(seesawBase > seesawCeiling)
            {
                seesawBase = seesawCeiling;
            }
        }
    }
    // Set calculated position of seesaw arc
    retDouble = swingSubsidy * seesawBase;
    // v1.1 payment subsidy patch
    if(nLiveForkToggle > 0)
    {
        if(pindexBest->nHeight > nLiveForkToggle) // Selectable toggle
        {
            // set returned value to calculated value
            ret = retDouble;
        }
    }
    // Return our seesaw arc value (reward in current position of arc)
    return ret;
}

//
// DevOps coin base reward
//
int64_t GetDevOpsPayment(int nHeight, int64_t blockValue)
{
    int64_t ret2 = 0;
    ret2 = 1 * COIN; // 1 FRTC per block

    return ret2;
}
