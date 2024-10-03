#ifndef ENCRYPTION_H
#define ENCRYPTION_H

int get_key(unsigned char *md);
void file_encrypt(const unsigned char *user_key, const char *input_filename,
                  const char *output_filename);
void file_decrypt(const unsigned char *user_key, const char *input_filename,
                  const char *output_filename);

#endif
