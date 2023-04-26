/*
 * Bus-Encryption style overhead for GPU escape.
 * Here we cpu+gpu encrypt on the x86 side to give better
 * perspective for encryption overhead as pure FVP software encryption is quite slow.
 *
 * XXX: Tricky part: We dont explicitly have a cuda runtime initialized
 *      and have to launch cuda kernels
 */
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <glib.h>
#include <memory.h>
#include "gdev_api.h"
#include "gpu_enc.h"
#include "cuda.h"

/*
 * set to 1 to increase trace
 */
#define ENC_TRACE_PRINT 0
#define TRACE_PRINT(fmt, ...) if (GPU_TRACE_PRINT) {printf(fmt, ##__VA_ARGS__);}
#define ERROR_PRINT(fmt, ...) {printf(fmt, ##__VA_ARGS__);}

#define ROUND_DOWN(x, s) (((uint64_t)(x)) & (~((uint64_t)s-1)))
#define ROUND_UP(x, s) ( (((uint64_t)(x)) + (uint64_t)s-1)  & (~((uint64_t)s-1)) )
#define GPU_BLOCK_SIZE (uint64_t)(256 * 16)
#define GPU_BLOCK_MASK (GPU_BLOCK_SIZE - 1)

static unsigned char h_key[33], h_IV[33];

// key: device mem pointer
static GHashTable *enc__hash_alloc = NULL;

struct dev_buf_with_bb {
    unsigned long dev_ptr;  // device buffer
    unsigned long dev_bb;   // device bounce buffer
    void *host_bb;  // host bounce buffer
};

static int enc__bb_free(Ghandle handle,
                        struct dev_buf_with_bb *data,
                        uint64_t *ret_res)
{
    uint64_t res = 0;
    if (data != NULL) {
        if (data->host_bb != NULL) {
            free(data->host_bb);
            data->host_bb = NULL;
        }
        if (data->dev_ptr != 0) {
            res = gfree(handle, data->dev_ptr);
            data->dev_ptr = 0;
        }
        if (data->dev_bb != 0) {
            gfree(handle, data->dev_bb);
            data->dev_bb = 0;
        }
        free(data);
        data = NULL;
    }
    if (ret_res) {
        *ret_res = res;
    }
    return res;
}

int enc__bb_alloc(Ghandle handle,
                  unsigned int size,
                  struct dev_buf_with_bb **ret_alloc)
{
    int ret = 0;
    const unsigned int bb_size = ROUND_UP(size, GPU_BLOCK_SIZE);
    struct dev_buf_with_bb *data = malloc(sizeof(struct dev_buf_with_bb));
    if (!data) {
        return -ENOMEM;
    }
    memset(data, 0, sizeof(struct dev_buf_with_bb));

    data->host_bb = malloc(bb_size);
    if (data->host_bb == NULL) {
        ret = -ENOMEM;
        goto error;
    }
    data->dev_ptr = gmalloc(handle, bb_size);
    if (data->dev_ptr == 0) {
        ret = -ENOMEM;
        goto error;
    }
    data->dev_bb = gmalloc(handle, bb_size);
    if (data->dev_bb == 0) {
        ret = -ENOMEM;
        goto error;
    }

    *ret_alloc = data;
    return 0;

    error:
    if (data != NULL) {
        enc__bb_free(handle, data, NULL);
        data = NULL;
    }
    return ret;
}

static int aes256_ctr_encrypt(
    unsigned char *c, int *clen,
    const unsigned char *m, int mlen,
    const unsigned char *npub,
    const unsigned char *k
)
{
    int ret = EXIT_FAILURE;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_CIPHER const *cipher = EVP_aes_256_ctr();
    if (ctx == NULL || cipher == NULL) {
        goto openssl_err;
    }
    if (EVP_EncryptInit_ex(ctx, cipher, NULL, k, npub) != 1) {
        goto openssl_err;
    }
    // get/set params if necessary, eg IVLEN

    // no AD yet
    // if (EVP_EncryptUpdate(ctx, 0, &outlen, ad,adlen) != 1) goto err;

    // encrypt m into ciphertext c
    if (EVP_EncryptUpdate(ctx, c, clen, m, mlen) != 1) {
        goto openssl_err;
    }

    // add trailing padding
    if (EVP_EncryptFinal_ex(ctx, c, clen) != 1) {
        goto openssl_err;
    }

    ret = EXIT_SUCCESS;
    goto cleanup;

    openssl_err:
    ERR_print_errors_fp(stderr);

    cleanup:
    EVP_CIPHER_CTX_free(ctx);
    return ret;
}

static int aes256_ctr_decrypt(
    unsigned char *m, int *mlen,
    const unsigned char *c, int clen,
    const unsigned char *npub,
    const unsigned char *k
)
{
    int ret = EXIT_FAILURE;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_CIPHER const *cipher = EVP_aes_256_ctr();
    if (ctx == NULL || cipher == NULL) {
        goto openssl_err;
    }
    if (EVP_DecryptInit_ex(ctx, cipher, NULL, k, npub) != 1) {
        goto openssl_err;
    }
    // get/set params if necessary, eg IVLEN and TAG

    // encrypt m into ciphertext c
    if (EVP_DecryptUpdate(ctx, m, mlen, c, clen) != 1) {
        goto openssl_err;
    }
    // add trailing padding
    if (EVP_DecryptFinal_ex(ctx, m, mlen) != 1) {
        goto openssl_err;
    }

    ret = EXIT_SUCCESS;
    goto cleanup;

    openssl_err:
    ERR_print_errors_fp(stderr);

    cleanup:
    EVP_CIPHER_CTX_free(ctx);
    return ret;
}

inline Ghandle enc__gopen(int minor)
{
    Ghandle ret = gopen(minor);
    enc__hash_alloc = g_hash_table_new(g_direct_hash, g_direct_equal);
    if (enc__hash_alloc == NULL) {
        ERROR_PRINT("g_hash_table_new failed for enc__hash_alloc\n");
    }
    return ret;
}

inline int enc__gclose(Ghandle h)
{
    int ret = gclose(h);
    if (enc__hash_alloc != NULL) {
        g_hash_table_destroy(enc__hash_alloc);
    }
    return ret;
}

inline uint64_t enc__gmalloc(Ghandle h, uint64_t size)
{
    int ret;
    struct dev_buf_with_bb *data;
    ret = enc__bb_alloc(h, size, &data);
    if (ret != 0) {
        ERROR_PRINT("enc__bb_alloc failed for req size %lx\n", size);
        return 0 /*invalid */;
    }
    g_hash_table_insert(enc__hash_alloc, (void *) data->dev_ptr, data);
    return data->dev_ptr;
}

inline uint64_t enc__gfree(Ghandle h, uint64_t addr)
{
    int ret = 0;
    uint64_t res = 0;
    struct dev_buf_with_bb *data = g_hash_table_lookup(enc__hash_alloc, (const void *) addr);
    if (data == NULL) {
        ERROR_PRINT("g_hash_table_lookup failed for addr %lx\n", addr);
        return gfree(h, addr);
    }

    ret = enc__bb_free(h, data, &res);
    if (ret != 0) {
        ERROR_PRINT("enc__bb_free failed for addr size %lx\n", addr);
        return 0 /*invalid */;
    }
    g_hash_table_remove(enc__hash_alloc, (const void *) addr);
    return res;
}

inline void *enc__gmalloc_dma(Ghandle h, uint64_t size)
{
    return gmalloc_dma(h, size);
}

inline uint64_t enc__gfree_dma(Ghandle h, void *buf)
{
    return gfree_dma(h, buf);
}

inline int enc__gmemcpy_to_device(Ghandle h, uint64_t dst_addr, const void *src_buf, uint64_t size)
{
    return gmemcpy_to_device(h, dst_addr, src_buf, size);
}

inline int enc__gmemcpy_from_device(Ghandle h, void *dst_buf, uint64_t src_addr, uint64_t size)
{
    return gmemcpy_from_device(h, dst_buf, src_addr, size);
}

inline int enc__glaunch(Ghandle h, struct gdev_kernel *kernel, uint32_t *id)
{
    return glaunch(h, kernel, id);
}
