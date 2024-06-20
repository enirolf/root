/// \ingroup NTuple ROOT7
/// \file ROOT/RNTupleIndexVector.hxx
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

#ifndef ROOT7_RNTupleIndexVector
#define ROOT7_RNTupleIndexVector

#include <ROOT/RField.hxx>
#include <ROOT/RNTupleUtil.hxx>
#include <ROOT/RNTupleIndex.hxx>

#include <unordered_map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace ROOT {
namespace Experimental {
namespace Internal {
class RNTupleIndex;
// clang-format off
/**
\class ROOT::Experimental::Internal::RNTupleIndexVector
\ingroup NTuple
\brief Build an index for an RNTuple so it can be joined onto other RNTuples.
*/
// clang-format on
class RNTupleIndexVector : public RNTupleIndex {
   friend std::unique_ptr<RNTupleIndex>
   CreateRNTupleIndex(std::vector<std::string_view> fieldNames, RPageSource &pageSource);

private:
   std::vector<NTupleIndexValue_t> fIndexValues;
   std::vector<NTupleSize_t> fEntryIndices;

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Create an RNTupleIndexVector for an existing RNTuple.
   ///
   /// \param[in] The fields that will make up the index.
   /// \param[in] The page source of the RNTuple to build the index for.
   ///
   /// \note The page source is assumed be attached already.
   RNTupleIndexVector(std::vector<std::unique_ptr<RFieldBase>> fields) : RNTupleIndex(std::move(fields)) {}

   void AddEntry(std::vector<RFieldBase::RValue> &values, NTupleSize_t entryIdx) final;

public:
   RNTupleIndexVector(const RNTupleIndexVector &other) = delete;
   RNTupleIndexVector &operator=(const RNTupleIndexVector &other) = delete;
   RNTupleIndexVector(RNTupleIndexVector &&other) = delete;
   RNTupleIndexVector &operator=(RNTupleIndexVector &&other) = delete;
   ~RNTupleIndexVector() override = default;

   std::size_t GetNElems() const final { return fIndexValues.size(); }

   void Build(NTupleSize_t firstEntry, NTupleSize_t lastEntry) final;

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Get the entry number containing the given index value.
   ///
   /// \param[in] value The indexed value
   /// \return The entry number, containing the specified index value. When no such entry exists, return
   /// `kInvalidNTupleIndex`
   ///
   /// Note that in case multiple entries corresponding to the provided index value exist, the first occurrence is
   /// returned. Use RNTupleIndexVector::GetEntryIndices to get all entries.
   NTupleSize_t GetEntryIndex(std::vector<void *> valuePtrs) const final;

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Get all entry numbers for the given index.
   ///
   /// \param[in] value The indexed value
   /// \return The entry numbers containing the specified index value. When no entries exists, return an empty vector.
   std::vector<NTupleSize_t> GetEntryIndices(std::vector<void *> valuePtrs) const final;
};
} // namespace Internal
} // namespace Experimental
} // namespace ROOT

#endif // ROOT7_RNTupleIndexVector
