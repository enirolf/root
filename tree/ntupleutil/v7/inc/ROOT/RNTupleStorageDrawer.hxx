/// \file ROOT/RNTupleStorageDrawer.hxx
/// \ingroup NTuple ROOT7
/// \author Simon Leisibach <simon.leisibach@gmail.com>
/// \date 2019-11-07
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2019, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT7_RNTupleStorageDrawer
#define ROOT7_RNTupleStorageDrawer

#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleDescriptor.hxx>
#include <ROOT/RNTupleInspector.hxx>
#include <ROOT/RNTupleUtil.hxx>

#include <TBox.h>
#include <TCanvas.h>
#include <TH1F.h>
#include <TLegend.h>
#include <TLine.h>
#include <TPad.h>
#include <TText.h>

#include <memory>
#include <string>

namespace ROOT {
namespace Experimental {
namespace Internal {

// clang-format off
/**
\class ROOT::Experimental::RNTupleStorageDrawer
\ingroup NTuple
\brief Main class for drawing the storage layout of a RNTuple

This class coordinates the drawing process of the storage layout of a RNTuple.
*/
// clang-format on
class RNTupleStorageDrawer {
public:
   class RMetaDataBox;
   class RPageBox;

private:
   struct RClusterHeading {
      std::unique_ptr<TText> fText;
      std::unique_ptr<TLine> fLine;

      RClusterHeading(std::unique_ptr<TText> text, std::unique_ptr<TLine> line)
         : fText(std::move(text)), fLine(std::move(line))
      {
      }
   };

   // const RNTupleInspector &fInspector;

public:
   // clang-format off
   /**
   \class ROOT::Experimental::Internal::RMetaDataBox
   \ingroup NTupleDraw
   \brief A TBox which contains metadata information of a RNTuple

   An RMetaDataBox is drawn on the TCanvas showing the RNTuple storage layout and represents some metadata (header or
   footer) in the RNTuple. It also holds some data of the metadata it represents, like its byte size.
   */
   // clang-format on
   class RMetaDataBox : public TBox {
   private:
      std::string fDescription; // e.g. "Header" or "Footer"
      const std::uint32_t fNBytesInStorage;
      // RNTupleStorageDrawer *fParent; //!

   public:
      RMetaDataBox() : RMetaDataBox(0, 0, 0, 0, "", 0) {}
      RMetaDataBox(double x1, double y1, double x2, double y2, std::string description, std::uint32_t nBytes,
                   std::int32_t color = kGray);
      std::uint32_t GetNBytesInStorage() const { return fNBytesInStorage; }

      void Inspect() const final; // *MENU*
      ClassDef(RMetaDataBox, 1);
   };

   // clang-format off
   /**
   \class ROOT::Experimental::RPageBox
   \ingroup NTupleDraw
   \brief A TBox which represents a RPage

   A RPageBox is drawn on the TCanvas showing the RNTuple storage layout and represents a RPage in the RNTuple. It also
   holds various data of a RPage, which allows the user to dump/inspect the RPageBox to obtain information about the RPage.
   */
   // clang-format on
   class RPageBox : public TBox {
   private:
      std::string fFieldName;
      std::string fFieldType;
      DescriptorId_t fFieldId;
      DescriptorId_t fColumnId;
      EColumnType fColumnType;
      DescriptorId_t fClusterId;
      DescriptorId_t fClusterGroupId;
      ClusterSize_t::ValueType fNElements;
      NTupleSize_t fGlobalRangeStart;
      NTupleSize_t fClusterRangeStart;
      RNTupleLocator fLocator; //! required for sorting
      std::size_t fPageBoxId;

   public:
      RPageBox() : RPageBox(0, 0, 0, 0, "", "", 0, 0, EColumnType::kUnknown, 0, 0, 0, 0, 0, RNTupleLocator()) {}
      RPageBox(double x1, double y1, double x2, double y2, std::string fieldName, std::string fieldType,
               DescriptorId_t fieldId, DescriptorId_t columnId, EColumnType columnType, DescriptorId_t clusterId,
               DescriptorId_t clusterGroupId, ClusterSize_t::ValueType nElements, NTupleSize_t globalRangeStart,
               NTupleSize_t clusterRangeStart, RNTupleLocator locator, std::int32_t color = kGray,
               std::size_t pageBoxId = 0);
      DescriptorId_t GetFieldId() const { return fFieldId; }
      DescriptorId_t GetClusterId() const { return fClusterId; }
      DescriptorId_t GetClusterGroupId() const { return fClusterGroupId; }
      const RNTupleLocator &GetLocator() const { return fLocator; }
      void SetClusterGroupId(std::size_t clusterId) { fClusterId = clusterId; }
      void SetPageId(std::size_t pageId) { fPageBoxId = pageId; }
      void Inspect() const final; // *MENU*
      bool IsDummyPage() const { return fNElements == 0 && fColumnId == 0 && fColumnType == EColumnType::kUnknown; }
      ClassDef(RPageBox, 1);
   };

   // RNTupleStorageDrawer(const RNTupleInspector &inspector) : fInspector(inspector) {}
   static std::int32_t GetColourFromFieldId(DescriptorId_t fieldId);
   static void RPageBoxClicked();
   static void Draw(const RNTupleInspector &inspector);
};
} // namespace Internal
} // namespace Experimental
} // namespace ROOT

#endif
