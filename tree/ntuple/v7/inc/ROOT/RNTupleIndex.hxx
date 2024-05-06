/// \file ROOT/RNTupleIndex.hxx
/// \ingroup NTuple ROOT7
/// \author Florine de Geus <florine.de.geus@cern.ch>
/// \date 2024-04-02
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2024, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT7_RNTupleIndex
#define ROOT7_RNTupleIndex

#include <ROOT/RField.hxx>
#include <ROOT/RNTupleUtil.hxx>

#include <unordered_map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace ROOT {
namespace Experimental {
namespace Internal {
// clang-format off
/**
\class ROOT::Experimental::Internal::RNTupleIndex
\ingroup NTuple
\brief Build an index for an RNTuple so it can be joined onto other RNTuples.
*/
// clang-format on
class RNTupleIndex {
   friend std::unique_ptr<RNTupleIndex>
   CreateRNTupleIndex(std::vector<std::string_view> fieldNames, RPageSource &pageSource);

private:
   /////////////////////////////////////////////////////////////////////////////
   /// Container for the combined hash of the indexed fields. Uses the implementation from `boost::hash_combine` (see
   /// https://www.boost.org/doc/libs/1_55_0/doc/html/hash/reference.html#boost.hash_combine).
   struct RIndexValue {
      std::size_t fValue = 0;

      void operator+=(std::size_t other) { fValue ^= other + 0x9e3779b9 + (fValue << 6) + (fValue >> 2); }
   };

   std::vector<std::unique_ptr<RFieldBase>> fFields;
   std::unordered_map<NTupleIndexValue_t, std::set<NTupleSize_t>> fIndex;

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Create an RNTupleIndex for an existing RNTuple.
   ///
   /// \param[in] The fields that will make up the index.
   /// \param[in] The page source of the RNTuple to build the index for.
   ///
   /// \note The page source is assumed be attached already.
   RNTupleIndex(std::vector<std::unique_ptr<RFieldBase>> fields, RPageSource &pageSource);

public:
   RNTupleIndex(const RNTupleIndex &other) = delete;
   RNTupleIndex &operator=(const RNTupleIndex &other) = delete;
   RNTupleIndex(RNTupleIndex &&other) = delete;
   RNTupleIndex &operator=(RNTupleIndex &&other) = delete;

   std::size_t GetNElems() const { return fIndex.size(); }

   void Add(std::vector<void *> objPtrs, NTupleSize_t entry);

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Get the entry number containing the given index value **after** the provided minimum entry.
   ///
   /// \param[in] value The indexed value
   /// \param[in] lowerBound The minimum entry number (inclusive) to retrieve. By default, all entries are considered.
   /// \return The entry number, starting from `lowerBound`, containing the specified index value. When no such entry
   /// exists, return `kInvalidNTupleIndex`
   // NTupleSize_t GetEntryIndex(void *valuePtr, NTupleSize_t lowerBound = 0) const;
   NTupleSize_t GetEntryIndex(std::vector<void *> objPtrs, NTupleSize_t lowerBound = 0) const;

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Get the entry number containing the given index value **after** the provided minimum entry.
   ///
   /// \sa GetEntry(void *valuePtr, NTupleSize_t lowerBound = 0)
   template <typename... Ts>
   NTupleSize_t GetEntryIndex(Ts... values, NTupleSize_t lowerBound = 0) const
   {
      if (sizeof...(Ts) != fFields.size())
         throw RException(R__FAIL("number of value pointers must match number of indexed fields"));

      std::vector<void *> valuePtrs;
      ([&] { valuePtrs.push_back(&values); }(), ...);

      return GetEntryIndex(valuePtrs, lowerBound);
   }
};

////////////////////////////////////////////////////////////////////////////////
/// \brief Create an RNTupleIndex from an existing RNTuple.
///
/// \param[in] fieldNames The names of the fields to index.
/// \param pageSource The page source.
///
/// \return A pointer to the newly-created index.
///
std::unique_ptr<RNTupleIndex> CreateRNTupleIndex(std::vector<std::string_view> fieldNames, RPageSource &pageSource);

} // namespace Internal
} // namespace Experimental
} // namespace ROOT

#endif // ROOT7_RNTupleIndex
