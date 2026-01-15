#include "ZipSZ3.h"

#include "ROOT/RConfig.hxx"

#include <SZ3/api/sz.hpp>
#include <SZ3/utils/Config.hpp>

static constexpr int kHeaderSize = 9;
// NOTE assumes data is FLOAT
static constexpr int kElemSize = sizeof(float);

void R__zipSZ3(int *srcsize, char *src, int *tgtsize, char *tgt, int *irep)
{
   SZ3::Config config(*srcsize / kElemSize);

   config.errorBoundMode = SZ3::EB_ABS;
   config.absErrorBound = 0.0005;

   auto retval =
      SZ_compress<float>(config, (float *)src, &tgt[kHeaderSize], static_cast<std::size_t>(*tgtsize - kHeaderSize));

   *irep = static_cast<std::size_t>(retval + kHeaderSize);

   size_t deflate_size = retval;
   size_t inflate_size = static_cast<size_t>(*srcsize);
   tgt[0] = 'S';
   tgt[1] = 'Z';
   tgt[2] = '\3';
   tgt[3] = deflate_size & 0xff;
   tgt[4] = (deflate_size >> 8) & 0xff;
   tgt[5] = (deflate_size >> 16) & 0xff;
   tgt[6] = inflate_size & 0xff;
   tgt[7] = (inflate_size >> 8) & 0xff;
   tgt[8] = (inflate_size >> 16) & 0xff;
}

void R__unzipSZ3(int *srcsize, unsigned char *src, int *tgtsize, unsigned char *tgt, int *irep)
{
   (void)tgtsize;

   auto floatTgt = (float *)tgt;

   SZ3::Config config;
   SZ_decompress<float>(config, (char *)&src[kHeaderSize], static_cast<size_t>(*srcsize - kHeaderSize), floatTgt);
   *irep = config.num * kElemSize;
}
