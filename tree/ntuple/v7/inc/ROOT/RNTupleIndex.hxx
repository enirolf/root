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
private:
   /// The maximum number of index elements we allow to be kept in memory. Used as a failsafe.
   std::size_t fMaxElemsInMemory = 64 * 1024 * 1024;
   std::size_t fNElems = 0;
   std::unique_ptr<RFieldBase> fField;
   std::unordered_map<NTupleIndexValue_t, std::set<NTupleSize_t>> fIndex;

public:
   RNTupleIndex(std::unique_ptr<RFieldBase> field) : fField(std::move(field)) {}
   RNTupleIndex() : fField(nullptr) {}
   RNTupleIndex(const RNTupleIndex &other) { *this = other; }
   RNTupleIndex &operator=(const RNTupleIndex &other);

   void SetMaxElementsInMemory(std::size_t maxElems) { fMaxElemsInMemory = maxElems; }

   std::size_t GetNElems() const { return fNElems; }

   void Add(void *objPtr, NTupleSize_t entry);

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Get the entry number containing the given index value **after** the provided minimum entry.
   ///
   /// \param[in] value The indexed value
   /// \param[in] lowerBound The minimum entry number (inclusive) to retrieve. By default, all entries are considered.
   /// \return The entry number, starting from `lowerBound`, containing the specified index value. When no such entry
   /// exists, return `kInvalidNTupleIndex`
   NTupleSize_t GetEntryIndex(void *valuePtr, NTupleSize_t lowerBound = 0) const;

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Get the entry number containing the given index value **after** the provided minimum entry.
   ///
   /// \sa GetEntry(void *valuePtr, NTupleSize_t lowerBound = 0)
   template <typename T>
   NTupleSize_t GetEntryIndex(std::shared_ptr<T> valuePtr, NTupleSize_t lowerBound = 0) const
   {
      return GetEntryIndex(valuePtr.get(), lowerBound);
   }
};

} // namespace Internal
} // namespace Experimental
} // namespace ROOT

#endif // ROOT7_RNTupleIndex
