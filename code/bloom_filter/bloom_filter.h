#include <functional>
#include <vector>
#include <string>
#include <cstdlib> // For size_t
#include <shared_mutex>

#include "../log/log.h"

template <size_t N>
class BloomFilter {
private:
    std::vector<bool> bits;
    std::hash<std::string> hash1;
    std::hash<std::string> hash2;

public:
    BloomFilter() {
        bits.resize(N);  // Assuming bits are initialized to false
    }

    void add(const std::string& item) {
        size_t hashA = hash1(item) % N;
        size_t hashB = hash2(item) % N;
        bits[hashA] = true;
        bits[hashB] = true;
        // Optionally add more hashes
        for (int i = 0; i < 3; ++i) {
            size_t combinedHash = (hashA + i * hashB) % N;
            bits[combinedHash] = true;
        }
    }

    bool possiblyContains(const std::string& item) const {
        size_t hashA = hash1(item) % N;
        size_t hashB = hash2(item) % N;
        if (!bits[hashA] || !bits[hashB]) return false;
        for (int i = 0; i < 3; ++i) {
            size_t combinedHash = (hashA + i * hashB) % N;
            if (!bits[combinedHash]) {
                LOG_DEBUG("Bloom filter does not contain item");
                return false;
            }
        }
        LOG_DEBUG("Bloom filter possibly contains item");
        return true;
    }
};
