#pragma once
#ifndef sha256_h
#define sha256_h
char* StrSHA256(const char* str, long long length);
char* FileSHA256(const char* file, char* sha256);
#endif