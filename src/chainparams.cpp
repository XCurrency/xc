// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2017 The XCurrency developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"

#include "random.h"
#include "util.h"
#include "utilstrencodings.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

using namespace std;
using namespace boost::assign;

struct SeedSpec6 {
    uint8_t addr[16];
    uint16_t port;
};

#include "chainparamsseeds.h"

/**
 * Main network
 */

//! Convert the pnSeeds6 array into usable address objects.
static void convertSeed6(std::vector<CAddress>& vSeedsOut, const SeedSpec6* data, unsigned int count)
{
    // It'll only connect to one or two seed nodes because once it connects,
    // it'll get a pile of addresses with newer timestamps.
    // Seed nodes are given a random 'last seen time' of between one and two
    // weeks ago.
    const int64_t nOneWeek = 7 * 24 * 60 * 60;
    for (unsigned int i = 0; i < count; i++) {
        struct in6_addr ip;
        memcpy(&ip, data[i].addr, sizeof(ip));
        CAddress addr(CService(ip, data[i].port));
        addr.nTime = GetTime() - GetRand(nOneWeek) - nOneWeek;
        vSeedsOut.push_back(addr);
    }
}

//   What makes a good checkpoint block?
// + Is surrounded by blocks with reasonable timestamps
//   (no blocks before with a timestamp after, none after with
//    timestamp before)
// + Contains no strange transactions
static Checkpoints::MapCheckpoints mapCheckpoints =
    boost::assign::map_list_of(0, uint256("0x00000eed6f2f91203829a968e203316662562928d441fdeb14d664163491eae7"));

static const Checkpoints::CCheckpointData data = {
    &mapCheckpoints,
    1513784712, // * UNIX timestamp of last checkpoint block
    0,    // * total number of transactions between genesis and last checkpoint
                //   (the tx=... number in the SetBestChain debug.log lines)
    2000        // * estimated number of transactions per day after checkpoint
};

static Checkpoints::MapCheckpoints mapCheckpointsTestnet =
    boost::assign::map_list_of(0, uint256("0x001"));
static const Checkpoints::CCheckpointData dataTestnet = {
    &mapCheckpointsTestnet,
    1513784712,
    0,
    250};

static Checkpoints::MapCheckpoints mapCheckpointsRegtest =
    boost::assign::map_list_of(0, uint256("0x001"));
static const Checkpoints::CCheckpointData dataRegtest = {
    &mapCheckpointsRegtest,
    1454124731,
    0,
    100};

class CMainParams : public CChainParams
{
public:
    CMainParams()
    {
        networkID = CBaseChainParams::MAIN;
        strNetworkID = "main";
        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 4-byte int at any alignment.
         */
        pchMessageStart[0] = 0xc1;
        pchMessageStart[1] = 0xc0;
        pchMessageStart[2] = 0xc2;
        pchMessageStart[3] = 0xc3;
        vAlertPubKey = ParseHex("0415758705177c87c35dadf7ebf66e93ecc2710253bbac955e695664011fa39ff29a84fa21ae9e203a43debb487170c143ab6eaffe4fa3b12e162d8a6d4da87395");
        nDefaultPort = 14333;
        bnProofOfWorkLimit = ~uint256() >> 20; // XCurrency starting difficulty is 1 / 2^12
        nSubsidyHalvingInterval = 210000;
        nMaxReorganizationDepth = 100;
        nEnforceBlockUpgradeMajority = 750;
        nRejectBlockOutdatedMajority = 950;
        nToCheckBlockUpgradeMajority = 1000;
        nMinerThreads = 0;
        nTargetTimespan = 1 * 60; // XCurrency: 1 day
        nTargetSpacing = 1 * 60;  // XCurrency: 1 minute
        nLastPOWBlock = 2000;
        nMaturity = 100;
        nServicenodeCountDrift = 20;
        nModifierUpdateBlock = 615800;

        /**
         * Build the genesis block. Note that the output of the genesis coinbase cannot
         * be spent as it did not originally exist in the database.
         *
         * CBlock(hash=00000ffd590b14, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=e0028e, nTime=1390095618, nBits=1e0ffff0, nNonce=28917698, vtx=1)
         *   CTransaction(hash=e0028e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
         *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d01044c5957697265642030392f4a616e2f3230313420546865204772616e64204578706572696d656e7420476f6573204c6976653a204f76657273746f636b2e636f6d204973204e6f7720416363657074696e6720426974636f696e73)
         *     CTxOut(nValue=50.00000000, scriptPubKey=0xA9037BAC7050C479B121CF)
         *   vMerkleTree: e0028e
         */
        const char* pszTimestamp = "iMac Pro, the most powerful Mac ever, available today";
        CMutableTransaction txNew;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        txNew.vout[0].nValue = 250 * COIN;
        txNew.vout[0].scriptPubKey = CScript() << ParseHex("0452c91a00518fb8c6d38100341f88499554284d1ba75097cc25ae5a0d811835c63d2cb46c8855304bca81c452b63ce71fcb6897d06f8000450841f72602457f74") << OP_CHECKSIG;
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock = 0;
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();
        genesis.nVersion = 1;
        genesis.nTime = 1513784712;
        genesis.nBits = bnProofOfWorkLimit.GetCompact();
        genesis.nNonce = 1;

//// Create genesis
        printf("block.GetHash() == %s\n", genesis.GetHash().ToString().c_str());
        if (true && (genesis.GetHash() != hashGenesisBlock)) {
                uint256 hashTarget = uint256().SetCompact(genesis.nBits);
                while (genesis.GetHash() > hashTarget) {
                    ++genesis.nNonce;
                    if (genesis.nNonce == 0) { ++genesis.nTime; }
                }
        }

        printf("block.GetHash() == %s\n", genesis.GetHash().ToString().c_str());
        printf("block.hashMerkleRoot == %s\n", genesis.hashMerkleRoot.ToString().c_str());
        printf("block.nTime = %u \n", genesis.nTime);
        printf("block.nNonce = %u \n", genesis.nNonce);
        printf("block.nBits = %u \n", genesis.nBits);
        printf("debug debug debug **********  print");
        printf("current hash %s\n", hashGenesisBlock.ToString().c_str());

//// Create genesis

        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("00000e3fa8a2a4d5ccc1c400d4be59f7134a49db8c3081e864b1fc527b0a8710"));
        assert(genesis.hashMerkleRoot == uint256("14b902a9bbb1edd9723960e8478568ced1bd9942bd8eed1cc988d5e96a01920f"));

        vSeeds.push_back(CDNSSeedData("76.164.207.5", "76.164.207.5")); // danbtcfull3
        vSeeds.push_back(CDNSSeedData("104.238.198.165", "104.238.198.165")); // fbtc1
        vSeeds.push_back(CDNSSeedData("88.99.169.175", "88.99.169.175")); // josip

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 75);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 8);
        base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 203);
	/*
	https://github.com/bitcoin/bips/blob/master/bip-0043.mediawiki
	Because this scheme can be used to generate nodes for more cryptocurrencies at once, or even something totally unrelated to cryptocurrencies, there's no point in using a special version magic described in section "Serialization format" of BIP32. We suggest to use always 0x0488B21E for public and 0x0488ADE4 for private nodes (leading to prefixes "xpub" and "xprv" respectively).
	*/
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >(); // xpub prefix
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >(); // xprv prefix
        // 	BIP44 coin type is from https://github.com/satoshilabs/slips/blob/master/slip-0044.md - TBD Blocknet
        base58Prefixes[EXT_COIN_TYPE] = boost::assign::list_of(0x80)(0x00)(0x00)(0x148).convert_to_container<std::vector<unsigned char> >();

        convertSeed6(vFixedSeeds, pnSeed6_main, ARRAYLEN(pnSeed6_main));

        fRequireRPCPassword = true;
        fMiningRequiresPeers = true;
        fAllowMinDifficultyBlocks = false;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fSkipProofOfWorkCheck = false;
        fTestnetToBeDeprecatedFieldRPC = false;
        fHeadersFirstSyncingActive = false;

        nPoolMaxTransactions = 3;
        strSporkKey = "04d179dd896fab8b4461bf828b5b618649c016c7e04c8361b4fe7abf4131f08d673115409241f71c27ee022bf3e945c941b96174e690858e5030f02e43c0970281";
        strObfuscationPoolDummyAddress = "D87q2gC9j6nNrnzCsg4aY6bHMLsT9nUhEw";
        nStartServicenodePayments = 1403728576; //Wed, 25 Jun 2014 20:36:16 GMT
    }

    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return data;
    }
};
static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CMainParams
{
public:
    CTestNetParams()
    {
        networkID = CBaseChainParams::TESTNET;
        strNetworkID = "test";
        pchMessageStart[0] = 0x45;
        pchMessageStart[1] = 0x76;
        pchMessageStart[2] = 0x65;
        pchMessageStart[3] = 0xba;
        vAlertPubKey = ParseHex("000010e83b2703ccf322f7dbd62dd5855ac7c10bd055814ce121ba32607d573b8810c02c0582aed05b4deb9c4b77b26d92428c61256cd42774babea0a073b2ed0c9");
        nDefaultPort = 15333;
        nEnforceBlockUpgradeMajority = 51;
        nRejectBlockOutdatedMajority = 75;
        nToCheckBlockUpgradeMajority = 100;
        nMinerThreads = 0;
        nTargetTimespan = 1 * 60; // XCurrency: 1 day
        nTargetSpacing = 1 * 60;  // XCurrency: 1 minute
        nLastPOWBlock = 2000;
        nMaturity = 15;
        nModifierUpdateBlock = 51197; 

        //! Modify the testnet genesis block so the timestamp is valid for a later start.
        genesis.nTime = 1513784712;
        genesis.nNonce = 1;

//// Create genesis
        printf("block.GetHash() == %s\n", genesis.GetHash().ToString().c_str());
        if (true && (genesis.GetHash() != hashGenesisBlock)) {
                uint256 hashTarget = uint256().SetCompact(genesis.nBits);
                while (genesis.GetHash() > hashTarget) {
                    ++genesis.nNonce;
                    if (genesis.nNonce == 0) { ++genesis.nTime; }
                }
        }

        printf("block.GetHash() == %s\n", genesis.GetHash().ToString().c_str());
        printf("block.hashMerkleRoot == %s\n", genesis.hashMerkleRoot.ToString().c_str());
        printf("block.nTime = %u \n", genesis.nTime);
        printf("block.nNonce = %u \n", genesis.nNonce);
        printf("block.nBits = %u \n", genesis.nBits);
        printf("debug debug debug **********  print");
        printf("current hash %s\n", hashGenesisBlock.ToString().c_str());

//// Create genesis

        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("000007badb6cd5e85569169613b11f7bd4419cb41a315dad9b8d7699fd65c5ea"));

        vFixedSeeds.clear();
        vSeeds.clear();
	
        vSeeds.push_back(CDNSSeedData("76.164.207.5", "76.164.207.5")); // danbtcfull3
        vSeeds.push_back(CDNSSeedData("104.238.198.165", "104.238.198.165")); // fbtc1
        vSeeds.push_back(CDNSSeedData("88.99.169.175", "88.99.169.175")); // josip

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 111); 
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 196); 
        base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 239);     // Testnet private keys start with '9' or 'c' (Bitcoin defaults)
        // Testnet xc3 BIP32 pubkeys start with 'DRKV'
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x3a)(0x80)(0x61)(0xa0).convert_to_container<std::vector<unsigned char> >();
        // Testnet xc3 BIP32 prvkeys start with 'DRKP'
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x3a)(0x80)(0x58)(0x37).convert_to_container<std::vector<unsigned char> >();
        // Testnet xc3 BIP44 coin type is '1' (All coin's testnet default)
        base58Prefixes[EXT_COIN_TYPE] = boost::assign::list_of(0x80)(0x00)(0x00)(0x01).convert_to_container<std::vector<unsigned char> >();

        convertSeed6(vFixedSeeds, pnSeed6_test, ARRAYLEN(pnSeed6_test));

        fRequireRPCPassword = true;
        fMiningRequiresPeers = true;
        fAllowMinDifficultyBlocks = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = true;

        nPoolMaxTransactions = 2;
        strSporkKey = "04565f429b8c68dabdcdf09608be05b4a10fc704f7de1866aaeed28a729ec5b8c418d90f9510ba115c0f35f353b1aea983fe99397d1c20685d6ab3ed0d0b7ba3ea";
        strObfuscationPoolDummyAddress = "y57cqfGRkekRyDRNeJiLtYVEbvhXrNbmox";
        nStartServicenodePayments = 1420837558; //Fri, 09 Jan 2015 21:05:58 GMT
    }
    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return dataTestnet;
    }
};
static CTestNetParams testNetParams;

/**
 * Regression test
 */
class CRegTestParams : public CTestNetParams
{
public:
    CRegTestParams()
    {
        networkID = CBaseChainParams::REGTEST;
        strNetworkID = "regtest";
        strNetworkID = "regtest";
        pchMessageStart[0] = 0xa1;
        pchMessageStart[1] = 0xcf;
        pchMessageStart[2] = 0x7e;
        pchMessageStart[3] = 0xac;
        nSubsidyHalvingInterval = 150;
        nEnforceBlockUpgradeMajority = 750;
        nRejectBlockOutdatedMajority = 950;
        nToCheckBlockUpgradeMajority = 1000;
        nMinerThreads = 1;
        nTargetTimespan = 24 * 60 * 60; // Blocknetdx: 1 day
        nTargetSpacing = 1 * 60;        // Blocknetdx: 1 minutes
        bnProofOfWorkLimit = ~uint256() >> 1;
        genesis.nTime = 1454124731;
        genesis.nBits = 0x207fffff;
        genesis.nNonce = 12345;

        hashGenesisBlock = genesis.GetHash();
        nDefaultPort = 51476;
        //assert(hashGenesisBlock == uint256("0x4f023a2120d9127b21bbad01724fdb79b519f593f2a85b60d3d79160ec5f29df"));

        vFixedSeeds.clear(); //! Testnet mode doesn't have any fixed seeds.
        vSeeds.clear();      //! Testnet mode doesn't have any DNS seeds.

        fRequireRPCPassword = false;
        fMiningRequiresPeers = false;
        fAllowMinDifficultyBlocks = true;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fTestnetToBeDeprecatedFieldRPC = false;
    }
    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return dataRegtest;
    }
};
static CRegTestParams regTestParams;

/**
 * Unit test
 */
class CUnitTestParams : public CMainParams, public CModifiableParams
{
public:
    CUnitTestParams()
    {
        networkID = CBaseChainParams::UNITTEST;
        strNetworkID = "unittest";
        nDefaultPort = 51478;
        vFixedSeeds.clear(); //! Unit test mode doesn't have any fixed seeds.
        vSeeds.clear();      //! Unit test mode doesn't have any DNS seeds.

        fRequireRPCPassword = false;
        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fAllowMinDifficultyBlocks = false;
        fMineBlocksOnDemand = true;
    }

    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        // UnitTest share the same checkpoints as MAIN
        return data;
    }

    //! Published setters to allow changing values in unit test cases
    virtual void setSubsidyHalvingInterval(int anSubsidyHalvingInterval) { nSubsidyHalvingInterval = anSubsidyHalvingInterval; }
    virtual void setEnforceBlockUpgradeMajority(int anEnforceBlockUpgradeMajority) { nEnforceBlockUpgradeMajority = anEnforceBlockUpgradeMajority; }
    virtual void setRejectBlockOutdatedMajority(int anRejectBlockOutdatedMajority) { nRejectBlockOutdatedMajority = anRejectBlockOutdatedMajority; }
    virtual void setToCheckBlockUpgradeMajority(int anToCheckBlockUpgradeMajority) { nToCheckBlockUpgradeMajority = anToCheckBlockUpgradeMajority; }
    virtual void setDefaultConsistencyChecks(bool afDefaultConsistencyChecks) { fDefaultConsistencyChecks = afDefaultConsistencyChecks; }
    virtual void setAllowMinDifficultyBlocks(bool afAllowMinDifficultyBlocks) { fAllowMinDifficultyBlocks = afAllowMinDifficultyBlocks; }
    virtual void setSkipProofOfWorkCheck(bool afSkipProofOfWorkCheck) { fSkipProofOfWorkCheck = afSkipProofOfWorkCheck; }
};
static CUnitTestParams unitTestParams;


static CChainParams* pCurrentParams = 0;

CModifiableParams* ModifiableParams()
{
    assert(pCurrentParams);
    assert(pCurrentParams == &unitTestParams);
    return (CModifiableParams*)&unitTestParams;
}

const CChainParams& Params()
{
    assert(pCurrentParams);
    return *pCurrentParams;
}

CChainParams& Params(CBaseChainParams::Network network)
{
    switch (network) {
    case CBaseChainParams::MAIN:
        return mainParams;
    case CBaseChainParams::TESTNET:
        return testNetParams;
    case CBaseChainParams::REGTEST:
        return regTestParams;
    case CBaseChainParams::UNITTEST:
        return unitTestParams;
    default:
        assert(false && "Unimplemented network");
        return mainParams;
    }
}

void SelectParams(CBaseChainParams::Network network)
{
    SelectBaseParams(network);
    pCurrentParams = &Params(network);
}

bool SelectParamsFromCommandLine()
{
    CBaseChainParams::Network network = NetworkIdFromCommandLine();
    if (network == CBaseChainParams::MAX_NETWORK_TYPES)
        return false;

    SelectParams(network);
    return true;
}
