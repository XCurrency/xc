#ifndef MESSAGE_H
#define MESSAGE_H

#include "../uint256.h"
#include "../sync.h"
#include "../key.h"
#include <vector>
#include <string>
#include <set>

#include <boost/thread/recursive_mutex.hpp>

class CNode;

struct Message {
    enum DESCRIPTION {
        MAX_MESSAGE_SIZE = 1024,
        SIGNATURE_SIZE = 65,
        // length of header, 4 + 1 + 8 + 20 + 16 + 33 + 32 + 4 +4
                HEADER_LENGTH = 122,
        // length of encrypted header
                ENCRYPTE_HEADER_LENGTH = 1 + 20 + 65 + 4
    };
    /**
     * @brief from
     */
    std::string from;
    /**
     * @brief to
     */
    std::string to;
    /**
     * @brief date
     */
    std::string date;
    /**
     * @brief text
     */
    std::string text;
    /**
     * @brief signature
     */
    std::vector<unsigned char> signature;
    /**
     * @brief publicRKey - generated key for encription
     */
    std::vector<unsigned char> publicRKey;
    /**
     * @brief encryptedData - encrypted data
     */
    std::vector<unsigned char> encryptedData;
    /**
     * @brief mac - message auth code
     */
    std::vector<unsigned char> mac;
    /**
     * @brief iv
     */
    std::vector<unsigned char> iv;
    /**
     * @brief timestamp
     */
    time_t timestamp;


    ADD_SERIALIZE_METHODS

    template<typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action, int nType, int nVersion) {
        READWRITE(from);
        READWRITE(to);
        READWRITE(date);
        READWRITE(text);
        READWRITE(signature);
        READWRITE(publicRKey);
        READWRITE(encryptedData);
        READWRITE(mac);
        READWRITE(iv);

    }
    /**
     *
     */
    Message() : timestamp(std::time(nullptr)) {}

    /**
     * @Message -default  constructor
     */
    Message(const Message &) = default;

    /**
     *
     * @return
     */
    Message &operator=(const Message &other) {
        from = other.from;
        to = other.to;
        date = other.date;
        text = other.text;
        signature = other.signature;
        publicRKey = other.publicRKey;
        encryptedData = other.encryptedData;
        mac = other.mac;
        iv = other.iv;
        timestamp = other.timestamp;
        return *this;
    }

    /**
     *
     * @param other
     * @return
     */
    bool operator<(const Message &other) {
        return date < other.date;
    }

    // full hash
    /**
     * @brief getNetworkHash
     * @return
     */
    uint256 getNetworkHash() const;

    //
    /**
     * @hash without timestamp
     * @return
     */
    uint256 getHash() const;


    /**
     * @brief getStaticHash
     * @return hash without date
     */
    uint256 getStaticHash() const;

    /**
     * @brief appliesToMe
     * @return
     */
    bool appliesToMe() const;

    /**
     * @brief getTime
     * @return
     */
    time_t getTime() const;

    /**
     * @brief isExpired
     * @return
     */
    bool isExpired() const;

    /**
     * @brief send
     * @return
     */
    bool send();

    /**
     * @brief process
     * @param isForMe
     * @return
     */
    bool process(bool &isForMe);

    /**
     * @brief processReceived
     * @param hash
     * @return
     */
    static bool processReceived(const uint256 &hash);

    /**
     * @brief relayTo
     * @param pnode
     * @return
     */
    bool relayTo(CNode *pnode) const;

    /**
     * @brief broadcast
     * @return
     */
    bool broadcast() const;

    /**
     * @brief sign
     * @param key
     * @return
     */
    bool sign(CKey &key);

    /**
     * @brief encrypt
     * @param senderPubKey
     * @return
     */
    bool encrypt(const CPubKey &senderPubKey);

    /**
     * @brief decrypt
     * @param receiverKey
     * @param isMessageForMy
     * @param senderPubKey
     * @return
     */
    bool decrypt(const CKey &receiverKey,
                 bool &isMessageForMy,
                 CPubKey &senderPubKey);

    /**
     * @brief
     * @return
     */
    bool isEmpty() const;

private:
//    static CCriticalSection &knownMessagesLocker_;

    static std::set<uint256> knownMessages_;

};

class CKeyIdPpn : public CKeyID {
public:
    unsigned int *getPpn() { return pn; }
};

#endif // MESSAGE_H
