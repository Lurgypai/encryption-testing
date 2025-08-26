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
    std::string getElapsed() {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - start);
        return std::to_string(elapsed.count()) + "ms";
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
static void setupBlowfish(gcry_cipher_hd_t* handle, int* keysize, int* blklen, std::string* iv) {
    int cypher = GCRY_CIPHER_BLOWFISH;
    int mode = GCRY_CIPHER_MODE_ECB;

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

    std::string plainText = generateBuffer(intCount);
    std::vector<unsigned char> cipherText;
    cipherText.resize(plainText.size());
    std::string decryptText;
    decryptText.resize(plainText.size());

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
    else if (algo == "blowfish") {
        setupBlowfish(&handle, &keysize, &blklen, &iv);
    }

    if(plainText.size() % blklen != 0) {
        std::print("Somehow the plaintext size ({}) is not a multiple of the block length ({})\n",
                plainText.size(),
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


    //encrypt
    int round = 0;
    Timer t{};
    t.reset();
    for(round = 0; round != plainText.size() / CHUNK_SIZE; ++round) {
        check_gcry(gcry_cipher_encrypt(handle, cipherText.data() + round * CHUNK_SIZE, CHUNK_SIZE,
                    plainText.data() + round * CHUNK_SIZE, CHUNK_SIZE));
    }
    std::print("Encryption time: {}\n", t.getElapsed());


    //decrypt
    check_gcry(gcry_cipher_reset(handle));
    check_gcry(gcry_cipher_setiv(handle, iv.data(), iv.size()));
    t.reset();
    for(round = 0; round != cipherText.size() / CHUNK_SIZE; ++round) {
        check_gcry(gcry_cipher_decrypt(handle, decryptText.data() + round * CHUNK_SIZE, CHUNK_SIZE,
                    cipherText.data() + round * CHUNK_SIZE, CHUNK_SIZE));
    }
    std::print("Decryption time: {}\n", t.getElapsed());


    //checking
    for(int i = 0; i != plainText.size(); ++i) {
        if(plainText[i] != decryptText[i]) {
            std::print("Character at index {} is not equal, {} != {}\n", i, plainText[i], decryptText[i]);
            return 1;
        }
    }

    return 0;
}

