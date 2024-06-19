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
protected:
   /////////////////////////////////////////////////////////////////////////////
   /// Container for the combined hash of the indexed fields. Uses the implementation from `boost::hash_combine` (see
   /// https://www.boost.org/doc/libs/1_55_0/doc/html/hash/reference.html#boost.hash_combine).
   struct RIndexValue {
      std::size_t fValue = 0;

      void operator+=(std::size_t other) { fValue ^= other + 0x9e3779b9 + (fValue << 6) + (fValue >> 2); }
   };

   std::vector<std::unique_ptr<RFieldBase>> fFields;
   bool fIsFrozen;

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Create an RNTupleIndex for an existing RNTuple.
   ///
   /// \param[in] The fields that will make up the index.
   /// \param[in] The page source of the RNTuple to build the index for.
   ///
   /// \note The page source is assumed be attached already.
   RNTupleIndex(std::vector<std::unique_ptr<RFieldBase>> fields) : fFields(std::move(fields)) {}

   virtual void AddEntry(std::vector<RFieldBase::RValue> &values, NTupleSize_t entryIdx) = 0;

public:
   RNTupleIndex(const RNTupleIndex &other) = delete;
   RNTupleIndex &operator=(const RNTupleIndex &other) = delete;
   RNTupleIndex(RNTupleIndex &&other) = delete;
   RNTupleIndex &operator=(RNTupleIndex &&other) = delete;
   virtual ~RNTupleIndex() = default;

   bool IsFrozen() const { return fIsFrozen; }
   void Freeze() { fIsFrozen = true; }
   void Unfreeze() { fIsFrozen = false; }

   virtual void Build(NTupleSize_t firstEntry, NTupleSize_t lastEntry) = 0;

   virtual std::size_t GetNElems() const = 0;

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Get the entry number containing the given index value.
   ///
   /// \param[in] value The indexed value
   /// \return The entry number, containing the specified index value. When no such entry exists, return
   /// `kInvalidNTupleIndex`
   ///
   /// Note that in case multiple entries corresponding to the provided index value exist, the first occurrence is
   /// returned. Use RNTupleIndex::GetEntryIndices to get all entries.
   virtual NTupleSize_t GetEntryIndex(std::vector<void *> valuePtrs) const = 0;

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Get the entry number containing the given index value.
   ///
   /// \sa GetEntryIndex(std::vector<void *> valuePtrs)
   template <typename... Ts>
   NTupleSize_t GetEntryIndex(Ts... values) const
   {
      if (sizeof...(Ts) != fFields.size())
         throw RException(R__FAIL("number of value pointers must match number of indexed fields"));

      std::vector<void *> valuePtrs;
      ([&] { valuePtrs.push_back(&values); }(), ...);

      return GetEntryIndex(valuePtrs);
   }

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Get all entry numbers for the given index.
   ///
   /// \param[in] value The indexed value
   /// \return The entry numbers containing the specified index value. When no entries exists, return an empty vector.
   virtual std::vector<NTupleSize_t> GetEntryIndices(std::vector<void *> valuePtrs) const = 0;

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Get all entry numbers for the given index.
   ///
   /// \sa GetEntryIndices(std::vector<void *> valuePtrs)
   template <typename... Ts>
   std::vector<NTupleSize_t> GetEntryIndices(Ts... values) const
   {
      if (sizeof...(Ts) != fFields.size())
         throw RException(R__FAIL("number of value pointers must match number of indexed fields"));

      std::vector<void *> valuePtrs;
      ([&] { valuePtrs.push_back(&values); }(), ...);

      return GetEntryIndices(valuePtrs);
   }
};

// clang-format off
/**
\class ROOT::Experimental::Internal::RNTupleIndexHash
\ingroup NTuple
\brief Build an index for an RNTuple so it can be joined onto other RNTuples.
*/
// clang-format on
class RNTupleIndexHash : public RNTupleIndex {
   friend std::unique_ptr<RNTupleIndex>
   CreateRNTupleIndex(std::vector<std::string_view> fieldNames, RPageSource &pageSource);

private:
   std::unordered_map<NTupleIndexValue_t, std::vector<NTupleSize_t>> fIndex;

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Create an RNTupleIndexHash for an existing RNTuple.
   ///
   /// \param[in] The fields that will make up the index.
   /// \param[in] The page source of the RNTuple to build the index for.
   ///
   /// \note The page source is assumed be attached already.
   RNTupleIndexHash(std::vector<std::unique_ptr<RFieldBase>> fields) : RNTupleIndex(std::move(fields)) {}

   void AddEntry(std::vector<RFieldBase::RValue> &values, NTupleSize_t entryIdx) final;

public:
   RNTupleIndexHash(const RNTupleIndexHash &other) = delete;
   RNTupleIndexHash &operator=(const RNTupleIndexHash &other) = delete;
   RNTupleIndexHash(RNTupleIndexHash &&other) = delete;
   RNTupleIndexHash &operator=(RNTupleIndexHash &&other) = delete;
   ~RNTupleIndexHash() override = default;

   void Build(NTupleSize_t firstEntry, NTupleSize_t lastEntry) final;

   std::size_t GetNElems() const final { return fIndex.size(); }

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Get the entry number containing the given index value.
   ///
   /// \param[in] value The indexed value
   /// \return The entry number, containing the specified index value. When no such entry exists, return
   /// `kInvalidNTupleIndex`
   ///
   /// Note that in case multiple entries corresponding to the provided index value exist, the first occurrence is
   /// returned. Use RNTupleIndexHash::GetEntryIndices to get all entries.
   NTupleSize_t GetEntryIndex(std::vector<void *> valuePtrs) const final;

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Get all entry numbers for the given index.
   ///
   /// \param[in] value The indexed value
   /// \return The entry numbers containing the specified index value. When no entries exists, return an empty vector.
   std::vector<NTupleSize_t> GetEntryIndices(std::vector<void *> valuePtrs) const final;
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
