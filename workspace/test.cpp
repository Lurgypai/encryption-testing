#include <iostream>
#include <fstream>
#include <sstream>
#include <print>
#include <exception>
#include <random>

#include <cstdint>

#include <gcrypt.h>

unsigned int constexpr CHUNK_SIZE{4096};

static void check_gcry(gcry_error_t err) {
    if(err != 0) {
        throw std::runtime_error{gcry_strerror(err)};
    }
}
static void setupBlowfish(gcry_cipher_hd_t* handle, int* keysize, int* blklen) {
    int cypher = GCRY_CIPHER_BLOWFISH;
    int mode = GCRY_CIPHER_MODE_ECB;

    check_gcry(gcry_cipher_open(handle, cypher, mode, 0));

    *keysize  = gcry_cipher_get_algo_keylen(cypher);
    *blklen = gcry_cipher_get_algo_blklen(cypher);
}

static void setupAes256(gcry_cipher_hd_t* handle, int* keysize, int* blklen) {
    int cypher = GCRY_CIPHER_AES256;
    int mode = GCRY_CIPHER_MODE_CBC;

    check_gcry(gcry_cipher_open(handle, cypher, mode, 0));


    *keysize  = gcry_cipher_get_algo_keylen(cypher);
    *blklen = gcry_cipher_get_algo_blklen(cypher);

    std::string iv{"this is an initialization vector."};
    iv.resize(*blklen);
    check_gcry(gcry_cipher_setiv(*handle, iv.data(), iv.size()));
}

static void setupChacha20(gcry_cipher_hd_t* handle, int* keysize, int* blklen) {
    int cypher = GCRY_CIPHER_CHACHA20;
    int mode = GCRY_CIPHER_MODE_STREAM;

    check_gcry(gcry_cipher_open(handle, cypher, mode, 0));

    *keysize  = gcry_cipher_get_algo_keylen(cypher);
    *blklen = gcry_cipher_get_algo_blklen(cypher);

    std::string iv{"this is a really bad nonce. Not recommended."};
    iv.resize(12);
    check_gcry(gcry_cipher_setiv(*handle, iv.data(), iv.size()));
}

static void setupCamellia256(gcry_cipher_hd_t* handle, int* keysize, int* blklen) {
    int cypher = GCRY_CIPHER_CAMELLIA256;
    int mode = GCRY_CIPHER_MODE_CBC;

    check_gcry(gcry_cipher_open(handle, cypher, mode, 0));

    *keysize  = gcry_cipher_get_algo_keylen(cypher);
    *blklen = gcry_cipher_get_algo_blklen(cypher);
}

int main(int argc, char** argv) {
    gcry_check_version(NULL);
    gcry_control( GCRYCTL_DISABLE_SECMEM_WARN );
    gcry_control( GCRYCTL_INIT_SECMEM, 16384, 0 );

    gcry_cipher_hd_t handle;
    int keysize, blklen;

    // setupCamellia256(&handle, &keysize, &blklen);
     setupChacha20(&handle, &keysize, &blklen);
    // setupAes256(&handle, &keysize, &blklen);
    // setupBlowfish(&handle, &keysize, &blklen);

    std::print("Key size: {}, Block size: {}\n", keysize, blklen);

    // load file
    std::fstream inFile{"./poirot.txt", std::ios::in | std::ios::binary};
    if(!inFile.good()) {
        std::print("Error opening input file\n");
        return 1;
    }
    std::stringstream ss{};
    ss<< inFile.rdbuf();
    std::string plainText = ss.str();
    

    // padding
    int remain = plainText.size() % CHUNK_SIZE;
    if(remain != 0) {
        plainText.resize(plainText.size() + (CHUNK_SIZE - remain));
    }
    std::vector<unsigned char> cipherText;
    cipherText.resize(plainText.size());
    std::string decryptText;
    decryptText.resize(plainText.size());


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
    // for(round = 0; round != plainText.size() / CHUNK_SIZE - 1; ++round) {
        // check_gcry(gcry_cipher_encrypt(handle, cipherText.data() + round * CHUNK_SIZE, CHUNK_SIZE,
                    // plainText.data() + round * CHUNK_SIZE, CHUNK_SIZE));
    // }
    check_gcry(gcry_cipher_final(handle));
    check_gcry(gcry_cipher_encrypt(handle, cipherText.data() + round * CHUNK_SIZE, CHUNK_SIZE,
                plainText.data() + round * CHUNK_SIZE, CHUNK_SIZE));


    //decrypt
    // for(round = 0; round != cipherText.size() / CHUNK_SIZE - 1; ++round) {
        // check_gcry(gcry_cipher_decrypt(handle, decryptText.data() + round * CHUNK_SIZE, CHUNK_SIZE,
                    // cipherText.data() + round * CHUNK_SIZE, CHUNK_SIZE));
    // }
    check_gcry(gcry_cipher_final(handle));
    check_gcry(gcry_cipher_decrypt(handle, decryptText.data() + round * CHUNK_SIZE, CHUNK_SIZE,
                cipherText.data() + round * CHUNK_SIZE, CHUNK_SIZE));


    // writing for comparison
    // std::ofstream cipherFile{"cipherFile", std::ios::out | std::ios::binary};
    std::ofstream decryptFile{"decryptFile.txt", std::ios::out | std::ios::binary};
    // cipherFile << cipherText;
    decryptFile << decryptText;


    //checking
    for(int i = 0; i != plainText.size(); ++i) {
        if(plainText[i] != decryptText[i]) {
            std::print("Character at index {} is not equal, {} != {}\n", i, plainText[i], decryptText[i]);
            return 1;
        }
    }
    std::print("plainText == decryptText\n");

    return 0;
}

