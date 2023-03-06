#include "ReadSpeed.hxx"

#include <numeric> // std::accumulate

ReadSpeed::ByteData ReadSpeed::SumBytes(const std::vector<ByteData> &bytesData)
{
   const auto uncompressedBytes =
      std::accumulate(bytesData.begin(), bytesData.end(), 0ull,
                      [](ULong64_t sum, const ByteData &o) { return sum + o.fUncompressedBytesRead; });
   const auto compressedBytes =
      std::accumulate(bytesData.begin(), bytesData.end(), 0ull,
                      [](ULong64_t sum, const ByteData &o) { return sum + o.fCompressedBytesRead; });

   return {uncompressedBytes, compressedBytes};
};
