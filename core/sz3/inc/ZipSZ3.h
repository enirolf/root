#ifndef ROOT_ZipSZ3
#define ROOT_ZipSZ3

// NOTE: the ROOT compression libraries aren't consistently written in C++; hence the
// #ifdef's to avoid problems with C code.
#ifdef __cplusplus
extern "C" {
#endif
void R__zipSZ3(int *srcsize, char *src, int *tgtsize, char *tgt, int *irep, float errbound);
void R__unzipSZ3(int *srcsize, unsigned char *src, int *tgtsize, unsigned char *tgt, int *irep);
#ifdef __cplusplus
}
#endif

#endif // ROOT_ZipSZ3
