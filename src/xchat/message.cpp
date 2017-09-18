
#include "message.h"
#include "message_db.h"
#include "../util.h"
#include "../net.h"
#include "../ui_interface.h"
#include "../init.h"
#include "../keystore.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>

#include "../base58.h"
#include "../main.h"
#include "../rpcserver.h"
#include "../wallet.h"
#include "../lz4/lz4.h"
#include "messagecrypter.h"
#include <fstream>

#include <boost/algorithm/string.hpp>

//CCriticalSection Message::&m_knownMessagesLocker;
std::set<uint256>      Message::knownMessages_;


uint256 Message::getNetworkHash() const {
    auto hashStr = from + to + date = std::to_string(timestamp);
    return Hash(hashStr.begin(), hashStr.end(),
                encryptedData.begin(), encryptedData.end());

}

uint256 Message::getHash() const {
    auto hashStr = from + to + date;
    return Hash(hashStr.begin(), hashStr.end(),
                encryptedData.begin(), encryptedData.end());

}

uint256 Message::getStaticHash() const {
    std::string hashStr = from + to;
    return Hash(hashStr.begin(), hashStr.end(),
                encryptedData.begin(), encryptedData.end());
}

bool Message::appliesToMe() const {
    // check broadcast message
    if (to.empty()) {
        return true;
    }

    CBitcoinAddress address(to);
    if (!address.IsValid()) {
        return false;
    }

    CKeyID id;
    if (!address.GetKeyID(id)) {
        return false;
    }

    return pwalletMain->HaveKey(id);


}

time_t Message::getTime() const {
    try {
        // date is "yyyy-MM-dd hh:mm:ss"
        auto pt = boost::posix_time::time_from_string(date);

        // local time adjustor
        static boost::date_time::c_local_adjustor<boost::posix_time::ptime> adj;

        auto t = boost::posix_time::to_tm(adj.utc_to_local(pt));

        return std::mktime(&t);
    }
    catch (std::exception &) {
    }
    return 0;
}

bool Message::isExpired() const {
    auto secs = static_cast<int>(std::time(nullptr) - getTime());
//    // +-2 days
    return (secs < -60 * 60 * 24 * 2) || (secs > 60 * 60 * 24 * 2);


}

bool Message::send() {
    return broadcast();
}

bool Message::process(bool &isForMe) {
    isForMe = false;

    if (appliesToMe()) {
        isForMe = true;

        {
            LOCK(knownMessagesLocker_);

            uint256 hash = getHash();
            if (knownMessages_.count(hash) > 0) {
                // already received and processed
                return true;
            }
            knownMessages_.insert(hash);
        }
        uiInterface.NotifyNewMessage(*this);

        // send message received
        LOCK(cs_vNodes);
        BOOST_FOREACH(CNode *pnode, vNodes) {
                        uint256 hash = getStaticHash();
                        pnode->PushMessage("msgack", hash);
                    }
    }
    return true;


}

bool Message::relayTo(CNode *pnode) const {
    auto hash = getHash();

    // returns true if wasn't already contained in the set
    if (pnode->setKnown.insert(hash).second) {
        pnode->PushMessage("message", *this);
        return true;
    }
    return false;
}

bool Message::broadcast() const {
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode *pnode, vNodes) {
                    relayTo(pnode);
                }

    return false;
}

bool Message::sign(CKey &key) {
    signature.clear();

    if (!key.IsValid()) {//TODO: add isNull method
        // TODO alert
        return false;
    }

    CDataStream ss(SER_GETHASH, 0);
    ss << strMessageMagic << text;

    if (!key.SignCompact(Hash(ss.begin(), ss.end()), signature)) {
        // TODO alert
        return false;
    }
    return true;
}

bool Message::encrypt(const CPubKey &senderPubKey) {
    // check empty
    if (text.empty()) {
        // empty message
        return false;
    }

    // check unsigned
    if (signature.empty()) {
        // unsigned message, need to sign it
        return false;
    }

    // generate random iv
    {
        iv.resize(16);
        RandAddSeedPerfmon();
        RAND_bytes(&iv[0], 16);
    }

    // make new compressed key
    CKey keyR;
    keyR.MakeNewKey(true);

    // destination public key
    CKey keyK;
    if (!keyK.SetPubKey(senderPubKey)) {
        // unvalid destination public key
        return false;
    };

    EC_KEY *pkeyr = keyR.GetECKey();
    EC_KEY *pkeyK = keyK.GetECKey();

    // ECDH_compute_key returns the same P if fed compressed or uncompressed public keys
    std::vector<unsigned char> vchP;
    vchP.resize(32);
    ECDH_set_method(pkeyr, ECDH_OpenSSL());
    int lenP = ECDH_compute_key(&vchP[0], 32, EC_KEY_get0_public_key(pkeyK), pkeyr, NULL);
    if (lenP != 32) {
        // ECDH_compute_key failed
        return false;
    };

    CPubKey cpkR = keyR.GetPubKey();
    if (!cpkR.IsValid() || !cpkR.IsCompressed()) {
        // Could not get public key
        return false;
    };

    // store new public key
    publicRKey.resize(33);
    memcpy(&publicRKey[0], &cpkR.Raw()[0], 33);

    // Use public key P and calculate the SHA512 hash H.
    // The first 32 bytes of H are called key_e and the last 32 bytes are called key_m.
    std::vector<unsigned char> vchHashed;
    vchHashed.resize(64); // 512
    SHA512(&vchP[0], vchP.size(), (unsigned char *) &vchHashed[0]);
    std::vector<unsigned char> key_e(&vchHashed[0], &vchHashed[0] + 32);
    std::vector<unsigned char> key_m(&vchHashed[32], &vchHashed[32] + 32);

    std::vector<unsigned char> vchCompressed;
    unsigned char *pMsgData = 0;
    uint32_t lenMsgData = 0;
    uint32_t lenMsg = text.size();

    // compression
    {
        if (lenMsg > 128) {
            // compress if over 128 bytes
            int worstCase = LZ4_compressBound(text.size());
            try {
                vchCompressed.resize(worstCase);
            }
            catch (std::exception &) {
                // compressoin failed
                return false;
            };

            int lenComp = LZ4_compress((char *) text.c_str(), (char *) &vchCompressed[0], lenMsg);
            if (lenComp < 1) {
                printf("Could not compress message data.\n");
                return 9;
            };

            pMsgData = &vchCompressed[0];
            lenMsgData = lenComp;
        } else {
            // no compression
            pMsgData = (unsigned char *) text.c_str();
            lenMsgData = lenMsg;
        };
    }

    // result data for encryption
    std::vector<unsigned char> vchPayload;

    {
        // allocate
        try {
            vchPayload.resize(DESCRIPTION::ENCRYPTE_HEADER_LENGTH + lenMsgData);
        }
        catch (std::exception &) {
            return false;
        };

        // copy compressed data to result
        memcpy(&vchPayload[DESCRIPTION::ENCRYPTE_HEADER_LENGTH], pMsgData, lenMsgData);

        // copy length of uncompressed plain text
        memcpy(&vchPayload[1 + 20 + 65], &lenMsg, 4);
    }

    // version, temporary not used
    // vchPayload[0] = coinAddrDest.nVersion;
    // vchPayload[0] = (static_cast<CBitcoinAddress_B*>(&coinAddrFrom))->getVersion();

    // copy from addr
    {
        CBitcoinAddress fromAddr;
        if (!fromAddr.SetString(from)) {
            // invalid from address
            return false;
        }

        CKeyIdPpn fromKeyId;
        if (!fromAddr.GetKeyID(fromKeyId)) {
            // invalid key id
            return false;
        }

        memcpy(&vchPayload[1], fromKeyId.getPpn(), 20);
    }

    // copy signature
    {
        if (signature.size() != DESCRIPTION::SIGNATURE_SIZE) {
            // invalid signature
            return false;
        }

        memcpy(&vchPayload[1 + 20], &signature[0], signature.size());
        signature.clear();
    }

    // encryption
    {
        MessageCrypter crypter;
        crypter.SetKey(key_e, &iv[0]);

        if (!crypter.Encrypt(&vchPayload[0], vchPayload.size(), encryptedData)) {
            // encryption failed
            return false;
        };

        text.clear();
    }

    // Calculate a 32 byte MAC with HMACSHA256, using key_m as salt
    // Message authentication code, (hash of timestamp + destination + payload)
    {
        mac.resize(32);

        HMAC_CTX ctx;
        HMAC_CTX_init(&ctx);

        unsigned int nBytes = 32;
        if (!HMAC_Init_ex(&ctx, &key_m[0], 32, EVP_sha256(), NULL)
            // || !HMAC_Update(&ctx, (unsigned char*) &smsg.timestamp, sizeof(smsg.timestamp))
            // || !HMAC_Update(&ctx, (unsigned char*) &smsg.destHash[0], sizeof(smsg.destHash))
            // || !HMAC_Update(&ctx, (unsigned char*) &date[0], date.size())
            || !HMAC_Update(&ctx, &encryptedData[0], encryptedData.size())
            || !HMAC_Final(&ctx, &mac[0], &nBytes)
            || nBytes != 32) {
            HMAC_CTX_cleanup(&ctx);

            // generate message auth code error
            return false;
        }

        HMAC_CTX_cleanup(&ctx);
    }

    return true;


}

bool Message::decrypt(const CKey &_receiverKey, bool &isMessageForMy,
                      CPubKey &senderPubKey) {
    CKey &receiverKey = const_cast<CKey &>(_receiverKey);

    isMessageForMy = false;

    if (!encryptedData.size() || !publicRKey.size()) {
        // no data
        return false;
    };

    CKey keyR;
    CPubKey cpkR(publicRKey);
    if (!cpkR.IsValid() || !keyR.SetPubKey(cpkR)) {
        // invalid key
        return false;
    };

    cpkR = keyR.GetPubKey();
    if (!cpkR.IsValid() || !cpkR.IsCompressed()) {
        // invalid key
        return false;
    };

    // Do an EC point multiply with private key k and public key R. This gives you public key P.
    EC_KEY *pkeyk = receiverKey.GetECKey();
    EC_KEY *pkeyR = keyR.GetECKey();

    ECDH_set_method(pkeyk, ECDH_OpenSSL());

    std::vector<unsigned char> vchP;
    vchP.resize(32);
    int lenPdec = ECDH_compute_key(&vchP[0], 32, EC_KEY_get0_public_key(pkeyR), pkeyk, NULL);
    if (lenPdec != 32) {
        // ECDH_compute_key failed
        return false;
    };

    // Use public key P to calculate the SHA512 hash H.
    // The first 32 bytes of H are called key_e and the last 32 bytes are called key_m.
    std::vector<unsigned char> vchHashedDec;
    vchHashedDec.resize(64);    // 512 bits
    SHA512(&vchP[0], vchP.size(), (unsigned char *) &vchHashedDec[0]);
    std::vector<unsigned char> key_e(&vchHashedDec[0], &vchHashedDec[0] + 32);
    std::vector<unsigned char> key_m(&vchHashedDec[32], &vchHashedDec[32] + 32);

    // check mac
    {
        // Message authentication code, (hash of timestamp + destination + payload)
        unsigned char MAC[32];

        HMAC_CTX ctx;
        HMAC_CTX_init(&ctx);

        unsigned int nBytes = 32;
        if (!HMAC_Init_ex(&ctx, &key_m[0], 32, EVP_sha256(), NULL)
            // || !HMAC_Update(&ctx, (unsigned char*) &psmsg->timestamp, sizeof(psmsg->timestamp))
            // || !HMAC_Update(&ctx, (unsigned char*) &psmsg->destHash[0], sizeof(psmsg->destHash))
            // || !HMAC_Update(&ctx, (unsigned char*) &date[0], date.size())
            || !HMAC_Update(&ctx, &encryptedData[0], encryptedData.size())
            || !HMAC_Final(&ctx, MAC, &nBytes)
            || nBytes != 32) {
            HMAC_CTX_cleanup(&ctx);

            // generate message auth code error
            return false;
        }

        HMAC_CTX_cleanup(&ctx);

        if (memcmp(MAC, &mac[0], 32) != 0) {
            // expected if message is not to address on node
            return false;
        };

        isMessageForMy = true;
    }

    MessageCrypter crypter;
    crypter.SetKey(key_e, &iv[0]);

    std::vector<unsigned char> vchPayload;
    if (!crypter.Decrypt(&encryptedData[0], encryptedData.size(), vchPayload)) {
        // decryption failed
        return false;
    };

    uint32_t lenData = 0;
    uint32_t lenPlain = 0;

    unsigned char *pMsgData = 0;

    lenData = vchPayload.size() - (DESCRIPTION::ENCRYPTE_HEADER_LENGTH);
    memcpy(&lenPlain, &vchPayload[1 + 20 + 65], 4);
    pMsgData = &vchPayload[DESCRIPTION::ENCRYPTE_HEADER_LENGTH];

    try {
        // text.resize(lenPlain + 1);
        text.resize(lenPlain);
    }
    catch (std::exception &) {
        // no memory
        return false;
    };


    if (lenPlain > 128) {
        // decompress
        if (LZ4_decompress_safe((char *) pMsgData, &text[0],
                                lenData, lenPlain) != (int) lenPlain) {
            // decompress failed
            return false;
        };
    } else {
        // plaintext
        memcpy(&text[0], pMsgData, lenPlain);
    };

    // text[lenPlain] = '\0';

    // source address check and signature validation
    {
        CBitcoinAddress coinAddrFrom;
        {
            std::vector<unsigned char> vchUint160;
            vchUint160.resize(20);

            memcpy(&vchUint160[0], &vchPayload[1], 20);

            uint160 ui160(vchUint160);
            CKeyID ckidFrom(ui160);

            coinAddrFrom.Set(ckidFrom);
            if (!coinAddrFrom.IsValid()) {
                // invalid address
                return false;
            };
        }

        // signature
        CBitcoinAddress coinAddrFromSig;
        {
            signature.resize(65);
            memcpy(&signature[0], &vchPayload[1 + 20], 65);

            CDataStream ss(SER_GETHASH, 0);
            ss << strMessageMagic << text;

            CKey keyFrom;
            keyFrom.SetCompactSignature(Hash(ss.begin(), ss.end()), signature);
            senderPubKey = keyFrom.GetPubKey();
            if (!senderPubKey.IsValid()) {
                // signature valifation failed
                return false;
            };

            // get address for the compressed public key
            coinAddrFromSig.Set(senderPubKey.GetID());
            if (!coinAddrFromSig.IsValid()) {
                // invalid address
                return false;
            };
        }

        if (!(coinAddrFrom == coinAddrFromSig)) {
            // signature valifation failed
            return false;
        };

        // save source public key
    }

    return true;
}


bool Message::isEmpty() const {
    return encryptedData.empty();
}

bool Message::processReceived(const uint256 &hash) {
    ChatDb &db = ChatDb::instance();


    UndeliveredMap map;
    if (db.loadUndelivered(map)) {
        if (map.erase(hash)) {
            // it is my message, return processed
            db.saveUndelivered(map);
            return true;
        }
    }

    return false;
}

