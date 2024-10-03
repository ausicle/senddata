#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AES_256_KEY_SIZE 32
#define AES_BLOCK_SIZE 16
#define BUFFER_SIZE 1024

void
handle_errors(void)
{
	ERR_print_errors_fp(stderr);
}

int
get_key(unsigned char *md)
{
	char password[256];
	size_t password_len;
	fputs("Password: ", stdout);
	fgets((char *)password, sizeof(password), stdin);
	password_len = strlen(password);
	password[password_len - 1] = '\0';
	--password_len;
	SHA256((unsigned char *)password, password_len, md);
	return 0;
}

int
text_encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
             unsigned char *iv, unsigned char *ciphertext)
{
	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	int len, ciphertext_len;

	if (!ctx) {
		handle_errors();
		return -1;
	}
	if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1) {
		handle_errors();
		return -1;
	}
	if (EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len) !=
	    1) {
		handle_errors();
		return -1;
	}
	ciphertext_len = len;
	if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) != 1) {
		handle_errors();
		return -1;
	}
	ciphertext_len += len;

	EVP_CIPHER_CTX_free(ctx);
	return ciphertext_len;
}

int
text_decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
             unsigned char *iv, unsigned char *plaintext)
{
	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	int len, plaintext_len;

	if (!ctx) {
		handle_errors();
		return -1;
	}
	if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1) {
		handle_errors();
		return -1;
	}
	if (EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len) !=
	    1) {
		handle_errors();
		return -1;
	}
	plaintext_len = len;
	if (EVP_DecryptFinal_ex(ctx, plaintext + len, &len) != 1) {
		handle_errors();
		return -1;
	}
	plaintext_len += len;

	EVP_CIPHER_CTX_free(ctx);
	return plaintext_len;
}

int
file_encrypt(unsigned char *key, const char *input_filename,
             const char *output_filename)
{
	FILE *in_file = fopen(input_filename, "rb");
	FILE *out_file = fopen(output_filename, "wb+");
	unsigned char iv[AES_BLOCK_SIZE];
	unsigned char buffer[BUFFER_SIZE];
	unsigned char ciphertext[BUFFER_SIZE + AES_BLOCK_SIZE];
	size_t bytes_read; // size of it
	int ciphertext_len;

	// error handling
	if (!in_file || !out_file) {
		perror("File error");
		exit(1);
	}

	// Generate a random IV and write it to the output file
	if (!RAND_bytes(iv, AES_BLOCK_SIZE)) {
		handle_errors();
		return -1;
	}
	fwrite(iv, 1, AES_BLOCK_SIZE, out_file);

	// Write ciphertext to target file until have nothing left
	while ((bytes_read = fread(buffer, 1, sizeof(buffer), in_file)) > 0) {
		ciphertext_len = text_encrypt(buffer, bytes_read, key, iv, ciphertext);
		fwrite(ciphertext, 1, ciphertext_len, out_file);
	}

	fclose(in_file);
	fclose(out_file);
	return 0;
}

int
file_decrypt(unsigned char *key, const char *input_filename,
             const char *output_filename)
{
	FILE *in_file = fopen(input_filename, "rb");
	FILE *out_file = fopen(output_filename, "wb+");
	unsigned char iv[AES_BLOCK_SIZE];
	unsigned char buffer[BUFFER_SIZE + AES_BLOCK_SIZE];
	unsigned char plaintext[BUFFER_SIZE];
	size_t bytes_read; // size of it
	int plaintext_len;

	if (!in_file || !out_file) {
		perror("File error");
		exit(1);
	}

	// Read the IV from the input file
	fread(iv, 1, AES_BLOCK_SIZE, in_file);

	// Write plaintext to target file until have nothing left
	while ((bytes_read = fread(buffer, 1, sizeof(buffer), in_file)) > 0) {
		plaintext_len = text_decrypt(buffer, bytes_read, key, iv, plaintext);
		if (plaintext_len < 0) {
			return -1;
		}
		fwrite(plaintext, 1, plaintext_len, out_file);
	}

	fclose(in_file);
	fclose(out_file);
	return 0;
}
