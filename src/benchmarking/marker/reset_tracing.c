#define GNU_SOURCE
#include <stdio.h>
#include "cca_benchmark.h"

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>


int aes256_ctr_decrypt_openssl(
    unsigned char *m, int *mlen,
    const unsigned char *c, int clen,
    const unsigned char *npub,
    const unsigned char *k
)
{
    int ret = EXIT_FAILURE;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_CIPHER const *cipher = EVP_aes_256_ctr();
    if (ctx == NULL || cipher == NULL)
        goto openssl_err;

    if (EVP_DecryptInit_ex(ctx, cipher, NULL, k, npub) != 1)
        goto openssl_err;

    // get/set params if necessary, eg IVLEN and TAG

    // encrypt m into ciphertext c
    if (EVP_DecryptUpdate(ctx, m, mlen, c, clen) != 1)
        goto openssl_err;

    // add trailing padding
    if (EVP_DecryptFinal_ex(ctx, m, mlen) != 1)
        goto openssl_err;

    ret = EXIT_SUCCESS;
    goto cleanup;

    openssl_err:
    ERR_print_errors_fp(stderr);
    cleanup:
    EVP_CIPHER_CTX_free(ctx);
    return ret;
}

int aes256_ctr_encrypt_openssl(
    unsigned char *c, int *clen,
    const unsigned char *m, int mlen,
    const unsigned char *npub,
    const unsigned char *k
)
{
    int ret = EXIT_FAILURE;


    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_CIPHER const *cipher = EVP_aes_256_ctr();


    if (ctx == NULL || cipher == NULL)
        goto openssl_err;

    if (EVP_EncryptInit_ex(ctx, cipher, NULL, k, npub) != 1)
        goto openssl_err;



    if (EVP_EncryptUpdate(ctx, c, clen, m, mlen) != 1)
        goto openssl_err;
    if (EVP_EncryptFinal_ex(ctx, c, clen) != 1)
        goto openssl_err;


    ret = EXIT_SUCCESS;
    goto cleanup;

    openssl_err:
    ERR_print_errors_fp(stderr);
    cleanup:
    EVP_CIPHER_CTX_free(ctx);

    return ret;
}

extern void *CRYPTO_malloc(size_t num, const char *file, int line);
#define tsan_add(ptr, n) __atomic_fetch_add((ptr), (n), __ATOMIC_RELAXED)
int main()
{
    CRYPTO_malloc_fn malloc_fn;
    CRYPTO_realloc_fn realloc_fn;
    CRYPTO_free_fn free_fn;

    CRYPTO_get_mem_functions(&malloc_fn, &realloc_fn, &free_nf);

    printf("malloc: %lx\n", malloc_fn);
    printf("remalloc: %lx\n", realloc_fn);

    int byte_count = 0x1000;
    char *src_host = malloc(byte_count);
    char *dst_host = malloc(byte_count);

    char h_key[] = "0123456789abcdeF0123456789abcdeF";
    char h_IV[] = "12345678876543211234567887654321";


    CCA_BENCHMARK_INIT;
    CCA_TRACE_START;
    __asm__ volatile ("ISB");
    CCA_MARKER_BENCH_START;



    volatile void * addr = malloc(byte_count); // CRYPTO_malloc(byte_count, NULL, 0 );

    CCA_MARKER_BENCH_STOP;
    __asm__ volatile ("ISB");
    CCA_TRACE_STOP
    return 0;

    int clen;
    if (aes256_ctr_encrypt_openssl(
        dst_host, &clen,   // c
        src_host, byte_count, // m
        h_IV, h_key) != EXIT_SUCCESS) {
        printf("error\n");
        return -1;
    }



    printf("ok\n");

//    __asm__ volatile("SMC 0");
//     __asm__ volatile("MOV WZR, #0x1227");
//    __asm__ volatile("mrs x0, SPSR_EL2");



    // __asm__ volatile("NOP #0x123");
//     __asm__ volatile("NOP #0x123");
    // __builtin_arm_nop(0x123);


    //__asm__ volatile("MOV WZR, #0x123");
    // __asm__ volatile("uaba #0x12345678");

    // __asm__ volatile("stp x0, x29, [sp, #-16]!");
    // __asm__ volatile("add x0, x1, x2, lsl #4");
    //__asm__ volatile("ldp x0, x29, [sp], #16");
    return 0;
}
