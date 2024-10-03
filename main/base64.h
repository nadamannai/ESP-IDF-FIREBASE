#ifndef BASE64_H
#define BASE64_H
#include <stdio.h>
#include<string.h>


size_t b64_encoded_size(size_t inlen);
char *b64_encode(unsigned char* in, size_t len);


#endif 
