#include <iostream>
#include <fstream>
#include <sstream>
#include <print>
#include <exception>
#include <random>
#include <chrono>

#include <cstdint>
#include <cstring>

#include <gcrypt.h>

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
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - start);
        return elapsed.count();
    }
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
};

unsigned int constexpr CHUNK_SIZE{4096};

static void check_gcry(gcry_error_t err) {
    if(err != 0) {
        throw std::runtime_error{gcry_strerror(err)};
    }
}
static void setupTwofish(gcry_cipher_hd_t* handle, int* keysize, int* blklen, std::string* iv) {
    int cypher = GCRY_CIPHER_TWOFISH;
    int mode = GCRY_CIPHER_MODE_CBC;

    check_gcry(gcry_cipher_open(handle, cypher, mode, 0));

    *keysize  = gcry_cipher_get_algo_keylen(cypher);
    *blklen = gcry_cipher_get_algo_blklen(cypher);
}

static void setupAes256(gcry_cipher_hd_t* handle, int* keysize, int* blklen, std::string* iv) {
    int cypher = GCRY_CIPHER_AES256;
    int mode = GCRY_CIPHER_MODE_CBC;

    check_gcry(gcry_cipher_open(handle, cypher, mode, 0));

    *keysize  = gcry_cipher_get_algo_keylen(cypher);
    *blklen = gcry_cipher_get_algo_blklen(cypher);

    iv->resize(*blklen);
    check_gcry(gcry_cipher_setiv(*handle, iv->data(), iv->size()));
}

static void setupChacha20(gcry_cipher_hd_t* handle, int* keysize, int* blklen, std::string* iv) {
    int cypher = GCRY_CIPHER_CHACHA20;
    int mode = GCRY_CIPHER_MODE_STREAM;

    check_gcry(gcry_cipher_open(handle, cypher, mode, 0));

    *keysize  = gcry_cipher_get_algo_keylen(cypher);
    *blklen = gcry_cipher_get_algo_blklen(cypher);

    iv->resize(12);
    check_gcry(gcry_cipher_setiv(*handle, iv->data(), iv->size()));
}

static void setupCamellia256(gcry_cipher_hd_t* handle, int* keysize, int* blklen) {
    int cypher = GCRY_CIPHER_CAMELLIA256;
    int mode = GCRY_CIPHER_MODE_CBC;

    check_gcry(gcry_cipher_open(handle, cypher, mode, 0));

    *keysize  = gcry_cipher_get_algo_keylen(cypher);
    *blklen = gcry_cipher_get_algo_blklen(cypher);
}

int main(int argc, char** argv) {
    if(argc != 3) {
        std::print("Error, incorrect arg count\n");
        std::print("Usage: {} <algo> <size>\n", argv[0]);
    }

    std::string algo{argv[1]};
    size_t intCount = std::stoi(argv[2]);

    // only hold CHUNK_SIZE in memory at a time
    size_t remaining = intCount * sizeof(std::uint64_t); // bytes left
    std::string plainText;
    plainText.resize(CHUNK_SIZE);
    std::string cipherText;
    cipherText.resize(CHUNK_SIZE);
    std::string decryptText;
    decryptText.resize(CHUNK_SIZE);

    gcry_check_version(NULL);
    gcry_control( GCRYCTL_DISABLE_SECMEM_WARN );
    gcry_control( GCRYCTL_INIT_SECMEM, 16384, 0 );

    gcry_cipher_hd_t handle;
    int keysize, blklen;
    std::string iv;

    if(algo == "camellia") {
        setupCamellia256(&handle, &keysize, &blklen);
    }
    else if (algo == "chacha20") {
        setupChacha20(&handle, &keysize, &blklen, &iv);
    }
    else if (algo == "aes256") {
        setupAes256(&handle, &keysize, &blklen, &iv);
    }
    else if (algo == "twofish") {
        setupTwofish(&handle, &keysize, &blklen, &iv);
    }

    if(remaining % blklen != 0) {
        std::println("Overall size ({}) is not a multiple of the block length ({})",
                remaining,
                blklen);
        return 1;
    }

    //keygen
    std::random_device rd{};
    std::default_random_engine re{rd()};
    std::uniform_int_distribution<unsigned int> dist{0, 255};
    std::string key{};
    key.resize(keysize);
    for(auto& c : key) c = dist(re);
    check_gcry(gcry_cipher_setkey(handle, key.data(), key.size()));

    std::fstream cipherFile{"cipherfile", std::ios::in | std::ios::out | std::ios::binary};
    std::fstream plainFile{"plainFile.txt", std::ios::in | std::ios::out | std::ios::binary};

    //encrypt
    Timer t{};
    double elapsed;
    for(; remaining > CHUNK_SIZE; remaining -= CHUNK_SIZE) {
        std::string plainText = generateBuffer(CHUNK_SIZE / sizeof(std::uint64_t));
        plainFile << plainText;

        t.reset();
        check_gcry(gcry_cipher_encrypt(handle, cipherText.data(), CHUNK_SIZE,
                    plainText.data(), CHUNK_SIZE));
        elapsed += t.getElapsed();

        cipherFile << cipherText;
    }
    std::println("Encryption time: {}ms", elapsed);

    remaining = intCount * sizeof(std::uint64_t);
    plainFile.seekg(0);
    cipherFile.seekg(0);

    //decrypt
    check_gcry(gcry_cipher_reset(handle));
    check_gcry(gcry_cipher_setiv(handle, iv.data(), iv.size()));
    for(; remaining > CHUNK_SIZE; remaining -= CHUNK_SIZE) {
        cipherFile.read(cipherText.data(), CHUNK_SIZE);

        t.reset();
        check_gcry(gcry_cipher_decrypt(handle, decryptText.data(), CHUNK_SIZE,
                    cipherText.data(), CHUNK_SIZE));
        elapsed += t.getElapsed();

        plainFile.read(plainText.data(), CHUNK_SIZE);
        if(plainText != decryptText) {
            std::println("error, chunks don't match");
            return 1;
        }
    }
    std::println("Decryption time: {}ms", t.getElapsed());

    return 0;
}

