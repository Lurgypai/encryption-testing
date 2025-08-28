#include <iostream>
#include <fstream>
#include <sstream>
#include <print>
#include <exception>
#include <random>
#include <chrono>
#include <memory>

#include <cstdint>
#include <cstring>

#include "ELgcrypt.h"

static std::string generateBuffer(size_t intCount) {
    std::mt19937_64 engine{std::random_device{}()};
    std::vector<std::mt19937_64::result_type> ints(intCount);
    for(auto& value : ints) {
        value = engine();
    }

    std::string ret{};
    ret.resize(intCount * sizeof(std::mt19937_64::result_type));
    std::memcpy(ret.data(), ints.data(), ret.size());

    return ret;
}

class Timer {
public:
    void reset() {
        start = std::chrono::high_resolution_clock::now();
    }
    double getElapsed() {
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now() - start);
        return elapsed.count();
    }
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
};

unsigned int constexpr CHUNK_SIZE{4096};

int main(int argc, char** argv) {
    if(argc != 4) {
        std::println("Error, incorrect arg count");
        std::println("Usage: {} <lib> <algo> <size>", argv[0]);
    }

    // read args
    std::string lib{argv[1]};
    std::string algo{argv[2]};
    size_t intCount = std::stoull(argv[3]);
    size_t remaining = intCount * sizeof(std::uint64_t); // bytes left

    // prep space for readingwriting
    std::string plainText;
    plainText.resize(CHUNK_SIZE);
    std::string cipherText;
    cipherText.resize(CHUNK_SIZE);
    std::string decryptText;
    decryptText.resize(CHUNK_SIZE);
    std::fstream cipherFile{"cipherfile", std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary};
    std::fstream plainFile{"plainfile.txt", std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary};
    if(!cipherFile.good() || !plainFile.good()) {
        std::println("Error, one or more files aren't good.");
        return 1;
    }

    // select lib
    std::unique_ptr<EncryptionLibrary> el{};
    if(lib == "gcrypt") {
        el = std::make_unique<ELgcrypt>();
    }


    // select algorithm
    size_t keySize = 0;
    if(algo == "camellia") {
        keySize = el->prepare(EncryptionLibrary::Algorithm::camellia256);
    }
    else if (algo == "chacha20") {
        keySize = el->prepare(EncryptionLibrary::Algorithm::chacha20);
    }
    else if (algo == "aes256") {
        keySize = el->prepare(EncryptionLibrary::Algorithm::camellia256);
    }
    else if (algo == "twofish") {
        keySize = el->prepare(EncryptionLibrary::Algorithm::twofish);
    }

    //keygen
    std::string key = EncryptionLibrary::MakeKey(keySize);
    el->setKey(key.data(), key.size());


    //encrypt
    Timer t{};
    double elapsed;
    for(; remaining >= CHUNK_SIZE; remaining -= CHUNK_SIZE) {
        plainText = generateBuffer(CHUNK_SIZE / sizeof(std::uint64_t));
        plainFile.write(plainText.data(), CHUNK_SIZE);

        t.reset();
        el->encrypt(plainText.data(), CHUNK_SIZE, cipherText.data(), CHUNK_SIZE);
        elapsed += t.getElapsed();

        cipherFile.write(cipherText.data(), CHUNK_SIZE);
    }
    std::println("Encryption time: {}s", elapsed / 1000000000);

    // reset for decrypting
    el->reset();
    elapsed = 0;
    remaining = intCount * sizeof(std::uint64_t);
    plainFile.seekg(0);
    cipherFile.seekg(0);

    // decrypt
    int count = 0;
    for(; remaining >= CHUNK_SIZE; remaining -= CHUNK_SIZE) {
        ++count;
        cipherFile.read(cipherText.data(), CHUNK_SIZE);

        t.reset();
        el->decrypt(cipherText.data(), CHUNK_SIZE, decryptText.data(), CHUNK_SIZE);
        elapsed += t.getElapsed();

        plainFile.read(plainText.data(), CHUNK_SIZE);
        if(plainText != decryptText) {
            std::println("error, chunk {} doesn't match", count);
            return 1;
        }
    }
    std::println("Decryption time: {}s", elapsed / 1000000000);

    return 0;
}

