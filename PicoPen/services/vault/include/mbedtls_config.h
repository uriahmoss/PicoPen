#ifndef PICOPEN_MBEDTLS_CONFIG_H
#define PICOPEN_MBEDTLS_CONFIG_H

/* Minimal SDK-pinned crypto surface for the credential vault. Network TLS is
 * configured separately when the IP stack is introduced. */
#define MBEDTLS_AES_C
#define MBEDTLS_GCM_C
#define MBEDTLS_MD_C
#define MBEDTLS_PKCS5_C
#define MBEDTLS_SHA256_C
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_CIPHER_C

#endif
