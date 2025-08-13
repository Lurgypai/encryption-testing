#include <iostream>
#include <print>

#include <gcrypt.h>

int main(int argc, char** argv) {
    gcry_check_version(NULL);
    gcry_control( GCRYCTL_DISABLE_SECMEM_WARN );
    gcry_control( GCRYCTL_INIT_SECMEM, 16384, 0 );

    int cypher = GCRY_CIPHER_TWOFISH;
    int mode = GCRY_CIPHER_MODE_ECB;

    gcry_cipher_hd_t handle;
    gcry_cipher_open(&handle, cypher, mode, 0);

    int keysize  =  gcry_cipher_get_algo_keylen(cypher);
    int blklen = gcry_cipher_get_algo_blklen(cypher);

    std::print("Key size: {}, Block size: {}\n", keysize, blklen);

    std::string plainText = "this is plaintext";
    plainText.resize(blklen);

    std::string key = "this is the key";


    size_t cryptSize = 2 * blklen;

    std::string cipherText;
    cipherText.resize(cryptSize);
    std::string decryptText;
    decryptText.resize(cryptSize);

    gcry_cipher_setkey(handle, key.data(), keysize);
    gcry_cipher_encrypt(handle, cipherText.data(), cryptSize, plainText.data(), blklen);

    gcry_cipher_setkey(handle, key.data(), keysize);
    gcry_cipher_decrypt(handle, decryptText.data(), cryptSize, cipherText.data(), blklen);

    std::print("plainText: {}\n", plainText);
    std::print("cipherText: {}\n", cipherText);
    std::print("decryptText: {}\n", decryptText);

    return 0;
}

