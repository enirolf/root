/// \file RNTupleStorageDrawer.cxx
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

#include <ROOT/RNTupleStorageDrawer.hxx>

#include <ROOT/RColumnElement.hxx>
#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleDescriptor.hxx>
#include <ROOT/RNTupleUtil.hxx>
#include <Rtypes.h>

#include <Buttons.h>
#include <TBox.h>
#include <TCanvas.h>
#include <TFrame.h>
#include <TH1F.h>
#include <TLatex.h>
#include <TLegend.h>
#include <TLine.h>
#include <TPad.h>
#include <TStyle.h>
#include <TText.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

// std::vector<ROOT::Experimental::Internal::RNTupleStorageDrawer>
//    ROOT::Experimental::Internal::RNTupleStorageDrawer::fgDrawStorageVec;

ROOT::Experimental::Internal::RNTupleStorageDrawer::RMetaDataBox::RMetaDataBox(double x1, double y1, double x2,
                                                                               double y2, std::string description,
                                                                               std::uint32_t nBytes, std::int32_t color)
   : TBox(x1, y1, x2, y2), fDescription{description}, fNBytesInStorage{nBytes}
{
   SetFillColor(color);
}

void ROOT::Experimental::Internal::RNTupleStorageDrawer::RMetaDataBox::Inspect() const
{
   static std::int32_t index = 0;
   // The canvases need to have unique names
   std::string uniqueCanvasName{"MetaDataDetails" + std::to_string(++index)};

   auto canvas = new TCanvas(uniqueCanvasName.c_str(), "Page Details", 500, 300);
   TLatex latex;

   // Draw Title
   latex.SetTextAlign(12);
   latex.SetTextSize(0.08);
   latex.DrawLatex(0.01, 0.96, fDescription.c_str());

   // Write Details
   latex.SetTextSize(0.06);
   std::string sizeString = "Size:" + std::string(30, ' ') + std::to_string(fNBytesInStorage) + " bytes";
   latex.DrawLatex(0.01, 0.85, sizeString.c_str());
   canvas->Draw();
}

ROOT::Experimental::Internal::RNTupleStorageDrawer::RPageBox::RPageBox(
   double x1, double y1, double x2, double y2, std::string fieldName, std::string fieldType, DescriptorId_t fieldId,
   DescriptorId_t columnId, EColumnType columnType, DescriptorId_t clusterId, DescriptorId_t clusterGroupId,
   ClusterSize_t::ValueType nElements, NTupleSize_t globalRangeStart, NTupleSize_t clusterRangeStart,
   RNTupleLocator locator, std::int32_t color, std::size_t pageBoxId)
   : TBox(x1, y1, x2, y2),
     fFieldName{fieldName},
     fFieldType{fieldType},
     fFieldId{fieldId},
     fColumnId{columnId},
     fColumnType{columnType},
     fClusterId{clusterId},
     fClusterGroupId{clusterGroupId},
     fNElements{nElements},
     fGlobalRangeStart{globalRangeStart},
     fClusterRangeStart{clusterRangeStart},
     fLocator{locator},
     fPageBoxId{pageBoxId}
{
   SetFillColor(color);
}

void ROOT::Experimental::Internal::RNTupleStorageDrawer::RPageBox::Inspect() const
{
   static std::int32_t index = 0;
   // The canvases need to have unique names, or else there will be an error saying that not all were found when trying
   // to delete them when quitting the program.
   std::string uniqueCanvasName{"PageDetails" + std::to_string(++index)};
   auto canvas = std::make_unique<TCanvas>(uniqueCanvasName.c_str(), "Page Details", 500, 300);
   TLatex latex;

   // Draw Title
   latex.SetTextAlign(12);
   latex.SetTextSize(0.08);
   std::string pageNumbering = "Page #" + std::to_string(fPageBoxId);
   latex.DrawLatex(0.01, 0.96, pageNumbering.c_str());

   // Write details about page
   latex.SetTextSize(0.06);
   std::string clusterIdString = "Cluster Id:" + std::string(37, ' ') + std::to_string(fClusterId);
   latex.DrawLatex(0.01, 0.85, clusterIdString.c_str());
   std::string fieldIdString = "Field Id:" + std::string(41, ' ') + std::to_string(fFieldId);
   latex.DrawLatex(0.01, 0.80, fieldIdString.c_str());
   std::string fieldName = "FieldName:" + std::string(35, ' ') + fFieldName;
   latex.DrawLatex(0.01, 0.75, fieldName.c_str());
   std::string fieldType = "FieldType:" + std::string(37, ' ') + fFieldType;
   latex.DrawLatex(0.01, 0.70, fieldType.c_str());
   std::string columnIdString = "Column Id:" + std::string(36, ' ') + std::to_string(fColumnId);
   latex.DrawLatex(0.01, 0.65, columnIdString.c_str());
   std::string columnTypeString = "ColumnType:" + std::string(32, ' ') + RColumnElementBase::GetTypeName(fColumnType);
   latex.DrawLatex(0.01, 0.60, columnTypeString.c_str());
   std::string nElements = "NElements:" + std::string(35, ' ') + std::to_string(fNElements);
   latex.DrawLatex(0.01, 0.55, nElements.c_str());
   std::string elementSizeOnDisk = "Element Size On Disk:" + std::string(17, ' ') +
                                   std::to_string(RColumnElementBase::GetBitsOnStorage(fColumnType)) + " bits";
   latex.DrawLatex(0.01, 0.50, elementSizeOnDisk.c_str());
   std::string elementSizeOnStorage = "Element Size On Storage:" + std::string(11, ' ') +
                                      std::to_string(8 * fLocator.fBytesOnStorage / fNElements) + " bits";
   latex.DrawLatex(0.01, 0.45, elementSizeOnStorage.c_str());
   std::string pageSize = "Page Size On Disk:" + std::string(22, ' ') +
                          std::to_string(fNElements * RColumnElementBase::GetBitsOnStorage(fColumnType) / 8) + " bytes";
   latex.DrawLatex(0.01, 0.40, pageSize.c_str());
   std::string pageSizeStorage =
      "Page Size On Storage:" + std::string(16, ' ') + std::to_string(fLocator.fBytesOnStorage) + " bytes";
   latex.DrawLatex(0.01, 0.35, pageSizeStorage.c_str());
   std::string globalRange = "Global Page Range:" + std::string(21, ' ') + std::to_string(fGlobalRangeStart) + " - " +
                             std::to_string(fGlobalRangeStart + fNElements - 1);
   latex.DrawLatex(0.01, 0.30, globalRange.c_str());
   std::string clusterRange = "Cluster Page Range:" + std::string(20, ' ') + std::to_string(fClusterRangeStart) +
                              " - " + std::to_string(fClusterRangeStart + fNElements - 1);
   latex.DrawLatex(0.01, 0.25, clusterRange.c_str());

   canvas->DrawClone();
}

void ROOT::Experimental::Internal::RNTupleStorageDrawer::Draw(const ROOT::Experimental::RNTupleInspector &inspector)
{
   auto descriptor = inspector.GetDescriptor();

   // Prepare title
   std::string title = "Storage layout of " + descriptor->GetName();
   auto titleText = std::make_unique<TText>(.5, .94, title.c_str());
   titleText->SetTextAlign(22);
   titleText->SetTextSize(0.05);

   // Prepare legend
   auto legend = std::make_unique<TLegend>(0.05, 0.425, 0.95, 0.525);
   legend->SetTextSize(0.06);
   std::int32_t nColumnsInLegend = 2;

   auto nFields = descriptor->GetNFields();
   if (nFields > 150) {
      nColumnsInLegend = 10;
   } else if (nFields > 120) {
      nColumnsInLegend = 9;
   } else if (nFields > 100) {
      nColumnsInLegend = 8;
   } else if (nFields > 75) {
      nColumnsInLegend = 7;
   } else if (nFields > 33) {
      nColumnsInLegend = 6;
   } else if (nFields > 26) {
      nColumnsInLegend = 5;
   } else if (nFields > 19) {
      nColumnsInLegend = 4;
   } else if (nFields > 4) {
      nColumnsInLegend = 3;
   }
   legend->SetNColumns(nColumnsInLegend);

   // Create all boxes and colour them
   constexpr double boxY1 = 0;
   constexpr double boxY2 = 1;
   auto headerBox =
      std::make_unique<RMetaDataBox>(0.0, boxY1, 0.0, boxY2, "Header", descriptor->GetOnDiskHeaderSize(), kRed);
   auto footerBox =
      std::make_unique<RMetaDataBox>(0.0, boxY1, 0.0, boxY2, "Footer", descriptor->GetOnDiskFooterSize(), kGray);

   std::vector<RPageBox> pageBoxes;
   std::vector<RMetaDataBox> pageListBoxes;
   auto nColumns = descriptor->GetNPhysicalColumns();
   // auto nClusters = descriptor->GetNClusters();
   auto nClusterGroups = descriptor->GetNClusterGroups();
   for (std::size_t clusterGroupId = 0; clusterGroupId < nClusterGroups; ++clusterGroupId) {
      // for (std::size_t clusterId = 0; clusterId < nClusters; ++clusterId) {
      const auto &clusterGroupDescriptor = descriptor->GetClusterGroupDescriptor(clusterGroupId);

      for (const auto clusterId : clusterGroupDescriptor.GetClusterIds()) {
         for (std::size_t columnId = 0; columnId < nColumns; ++columnId) {
            ClusterSize_t::ValueType localIndex{0};
            auto &pageRange = descriptor->GetClusterDescriptor(clusterId).GetPageRange(columnId);
            for (std::size_t pageIdx = 0; pageIdx < pageRange.fPageInfos.size(); ++pageIdx) {
               const auto &columnDescriptor = descriptor->GetColumnDescriptor(columnId);
               DescriptorId_t fieldId = columnDescriptor.GetFieldId();
               const auto &fieldDescriptor = descriptor->GetFieldDescriptor(fieldId);
               ClusterSize_t::ValueType nElements = pageRange.fPageInfos.at(pageIdx).fNElements;
               auto clusterRangeFirst = localIndex;
               auto globalRangeFirst =
                  localIndex + descriptor->GetClusterDescriptor(clusterId).GetColumnRange(columnId).fFirstElementIndex;

               pageBoxes.emplace_back(0, boxY1, 0, boxY2, descriptor->GetQualifiedFieldName(fieldId),
                                      fieldDescriptor.GetTypeName(), fieldId, columnId,
                                      columnDescriptor.GetModel().GetType(), clusterId, clusterGroupId, nElements,
                                      globalRangeFirst, clusterRangeFirst, pageRange.fPageInfos.at(pageIdx).fLocator,
                                      GetColourFromFieldId(fieldId));

               localIndex += nElements;
            }
         }
      }

      // put an empty page box to signal that we reached a page list. UGLY!!
      pageBoxes.emplace_back();
      pageBoxes.back().SetClusterGroupId(clusterGroupId);

      pageListBoxes.emplace_back(0.0, boxY1, 0.0, boxY2, "Page list",
                                 clusterGroupDescriptor.GetPageListLocator().fBytesOnStorage, kGray + 2);
   }

   // Sort RPageBoxes by page order and set Page Ids
   std::sort(pageBoxes.begin(), pageBoxes.end(), [](const RPageBox &a, const RPageBox &b) -> bool {
      if (a.GetClusterGroupId() != b.GetClusterGroupId())
         return a.GetClusterGroupId() < b.GetClusterGroupId();

      if (a.IsDummyPage())
         return false;
      if (b.IsDummyPage())
         return true;

      if (a.GetClusterId() != b.GetClusterId())
         return a.GetClusterId() < b.GetClusterId();
      return a.GetLocator().fPosition < b.GetLocator().fPosition;
   });
   size_t pageIdx = 1;
   for (auto &pageBox : pageBoxes) {
      pageBox.SetPageId(pageIdx++);
   }

   // Create cumulativeBytes to later to set x1 and x2 values for the boxes
   // The size is the number of pages + 2 to account for the header and footer
   // We don't need to separately add the number page lists because they are already represented as dummy pages in
   // pageBoxes
   std::vector<std::uint64_t> cumulativeBytes(pageBoxes.size() + 2);
   std::size_t pageListIdx = 0;
   cumulativeBytes.at(0) = descriptor->GetOnDiskHeaderSize();
   for (std::size_t i = 1; i < cumulativeBytes.size() - 1; ++i) {
      if (pageBoxes.at(i - 1).IsDummyPage())
         cumulativeBytes.at(i) = cumulativeBytes.at(i - 1) + pageListBoxes.at(pageListIdx++).GetNBytesInStorage();
      else
         cumulativeBytes.at(i) = cumulativeBytes.at(i - 1) + pageBoxes.at(i - 1).GetLocator().fBytesOnStorage;
   }
   cumulativeBytes.back() = descriptor->GetOnDiskFooterSize() + cumulativeBytes.at(cumulativeBytes.size() - 2);

   std::string axisTitle = "Data size (B)";
   auto totalBytes = inspector.GetCompressedSize();
   std::size_t axisScaleFactor = 1;

   if (totalBytes > 1024 * 1024 * 1024) {
      axisScaleFactor = 1024 * 1024 * 1024;
      axisTitle = "Data size (GB)";
   } else if (totalBytes > 1024 * 1024) {
      axisScaleFactor = 1024 * 1024;
      axisTitle = "Data size (MB)";
   } else if (totalBytes > 1024) {
      axisScaleFactor = 1024;
      axisTitle = "Data size (KB)";
   }

   // Set the correct x coordinates for each box
   headerBox->SetX1(0);
   headerBox->SetX2((double)cumulativeBytes.at(0) / axisScaleFactor);
   pageIdx = 0;
   pageListIdx = 0;
   for (auto &pageBox : pageBoxes) {
      if (pageBox.IsDummyPage()) {
         pageListBoxes.at(pageListIdx).SetX1((double)cumulativeBytes.at(pageIdx) / axisScaleFactor);
         pageListBoxes.at(pageListIdx).SetX2((double)cumulativeBytes.at(pageIdx + 1) / axisScaleFactor);
         ++pageListIdx;
      } else {
         pageBox.SetX1((double)cumulativeBytes.at(pageIdx) / axisScaleFactor);
         pageBox.SetX2((double)cumulativeBytes.at(pageIdx + 1) / axisScaleFactor);
      }
      ++pageIdx;
   }
   footerBox->SetX1((double)cumulativeBytes.at(cumulativeBytes.size() - 2) / axisScaleFactor);
   footerBox->SetX2((double)cumulativeBytes.back() / axisScaleFactor);

   // Add metatdata boxes to the legend
   legend->AddEntry(headerBox.get(), "Header", "f");
   legend->AddEntry(&pageListBoxes[0], "Page list", "f");
   legend->AddEntry(footerBox.get(), "Footer", "f");

   // Add page boxes to the legend
   // Start from 1 to skip the zero field
   for (std::size_t i = 0; i < descriptor->GetNFields(); ++i) {
      // For each field, find the first PageBox which represents that field and add it to the legend
      auto vecIt = std::find_if(pageBoxes.begin(), pageBoxes.end(),
                                [i](const RPageBox &a) -> bool { return a.GetFieldId() == i; });
      if (vecIt != pageBoxes.end() && !(*vecIt).IsDummyPage()) {
         legend->AddEntry(&(*vecIt), descriptor->GetQualifiedFieldName(i).c_str(), "f");
      }
   }

   // Prepare the cluster and cluster group axes
   double distanceBetweenLines = 0.002 * (totalBytes / axisScaleFactor);
   std::size_t start = cumulativeBytes.at(0);
   std::size_t end{0};
   std::vector<RClusterHeading> clusterHeadings;
   std::vector<RClusterHeading> clusterGroupHeadings;

   for (std::size_t clusterGroupId = 0; clusterGroupId < nClusterGroups; ++clusterGroupId) {
      const auto &clusterGroupDescriptor = descriptor->GetClusterGroupDescriptor(clusterGroupId);

      std::size_t cgStart = start;

      for (const auto clusterId : clusterGroupDescriptor.GetClusterIds()) {
         auto &cluster = descriptor->GetClusterDescriptor(clusterId);
         std::size_t nBytes = cluster.GetBytesOnStorage();
         // For some data formats (e.g. TFiles) this value is equal to 0
         // In that case get nBytes manually from all columns
         if (nBytes == 0) {
            for (std::size_t columnId = 0; columnId < nColumns; ++columnId) {
               for (std::size_t k = 0; k < cluster.GetPageRange(columnId).fPageInfos.size(); ++k) {
                  nBytes += cluster.GetPageRange(columnId).fPageInfos.at(k).fLocator.fBytesOnStorage;
               }
            }
         }
         end = start + nBytes;
         double x1 = (double)start / axisScaleFactor + distanceBetweenLines / 2;
         double x2 = (double)end / axisScaleFactor - distanceBetweenLines / 2;
         auto clusterText = std::make_unique<TText>((x1 + x2) / 2, 1.2, std::to_string(clusterId).c_str());
         clusterText->SetTextAlign(22);
         clusterText->SetTextSize(0.08);
         auto clusterLine = std::make_unique<TLine>(x1, 1.05, x2, 1.05);
         clusterLine->SetLineWidth(2);
         clusterHeadings.emplace_back(std::move(clusterText), std::move(clusterLine));
         start = end;
      }

      end += clusterGroupDescriptor.GetPageListLocator().fBytesOnStorage;
      double x1 = (double)cgStart / axisScaleFactor + distanceBetweenLines / 2;
      double x2 = (double)end / axisScaleFactor - distanceBetweenLines / 2;
      auto clusterGroupText = std::make_unique<TText>((x1 + x2) / 2, 1.6, std::to_string(clusterGroupId).c_str());
      clusterGroupText->SetTextAlign(22);
      clusterGroupText->SetTextSize(0.08);
      auto clusterGroupLine = std::make_unique<TLine>(x1, 1.45, x2, 1.45);
      clusterGroupLine->SetLineWidth(2);
      clusterGroupLine->SetLineStyle(kDashed);
      clusterGroupHeadings.emplace_back(std::move(clusterGroupText), std::move(clusterGroupLine));
   }

   // Create a new canvas
   static std::int32_t uniqueId = 0;
   // Trying to delete multiple canvases with the same name leads to an error or when two canvases have the same name,
   // only 1 may get deleted, causing a memory leak
   std::string uniqueCanvasName = "RNTupleStorageDrawer" + std::to_string(++uniqueId);
   auto canvas =
      std::make_unique<TCanvas>(uniqueCanvasName.c_str(), inspector.GetDescriptor()->GetName().c_str(), 1000, 300);
   canvas->cd();

   // Create a TPad in the canvas so that when zooming only the boxes and axis get zoomed
   constexpr double marginlength = 0.03;
   std::string uniquePadName = "RDrawStoragePad" + std::to_string(uniqueId);
   auto pad = new TPad(uniquePadName.c_str(), "", marginlength, 0.55, 1 - marginlength, 0.87);
   pad->SetTopMargin(0.5);
   pad->SetBottomMargin(0.2);
   pad->SetLeftMargin(0.01);
   pad->SetRightMargin(0.01);
   pad->Draw();
   pad->cd();

   // Draw an empty histogram without a y-axis for zooming
   std::string uniqueTH1FName = "RDrawStorageTH1F" + std::to_string(uniqueId);
   auto axisHelper =
      new TH1F(uniqueTH1FName.c_str(), "", 500, 0, (double)inspector.GetCompressedSize() / axisScaleFactor);
   axisHelper->SetMaximum(1);
   axisHelper->SetMinimum(0);
   axisHelper->GetYaxis()->SetTickLength(0);
   axisHelper->GetYaxis()->SetLabelSize(0);
   axisHelper->GetXaxis()->SetLabelSize(0.08);
   axisHelper->SetStats(0);
   axisHelper->DrawCopy();

   // Draw all boxes and add possibility to click on RPageBox to obtain information about a page
   headerBox->DrawClone();
   for (const auto &pageBox : pageBoxes) {
      pageBox.DrawClone();
   }
   for (const auto &pageListBox : pageListBoxes) {
      pageListBox.DrawClone();
   }
   footerBox->DrawClone();
   gPad->AddExec("ShowPageDetails", "ROOT::Experimental::Internal::RNTupleStorageDrawer::RPageBoxClicked()");

   // Draw the cluster axis
   for (const auto &c : clusterHeadings) {
      c.fText->DrawClone();
      c.fLine->DrawClone();
   }

   // Draw the cluster group axis
   for (const auto &c : clusterGroupHeadings) {
      c.fText->DrawClone();
      c.fLine->DrawClone();
   }

   // Return to canvas, draw the title, legend and x-axis label
   canvas->cd();
   titleText->DrawClone();
   legend->DrawClone();
   TLatex xLabel;
   xLabel.SetTextSize(0.025);
   xLabel.SetTextAlign(32);
   xLabel.DrawLatex(0.955, 0.55, axisTitle.c_str());

   // Disallow moving the boxes
   pad->SetEditable(kFALSE);
   canvas->DrawClone();
}

void ROOT::Experimental::Internal::RNTupleStorageDrawer::RPageBoxClicked()
{
   int event = gPad->GetEvent();
   if (event != kButton1Up)
      return;
   TObject *select = gPad->GetSelected();
   if (!select)
      return;
   if (select->InheritsFrom(RMetaDataBox::Class())) {
      RMetaDataBox *box = (RMetaDataBox *)select;
      box->Inspect();
   } else if (select->InheritsFrom(RPageBox::Class())) {
      RPageBox *box = (RPageBox *)select;
      box->Inspect();
   }
}

std::int32_t
ROOT::Experimental::Internal::RNTupleStorageDrawer::GetColourFromFieldId(ROOT::Experimental::DescriptorId_t fieldId)
{
   std::int32_t colour = 0;
   fieldId %= 61;
   switch (fieldId % 12) {
   case 0: colour = kRed; break;
   case 1: colour = kMagenta; break;
   case 2: colour = kBlue; break;
   case 3: colour = kCyan; break;
   case 4: colour = kGreen; break;
   case 5: colour = kYellow; break;
   case 6: colour = kPink; break;
   case 7: colour = kViolet; break;
   case 8: colour = kAzure; break;
   case 9: colour = kTeal; break;
   case 10: colour = kSpring; break;
   case 11: colour = kOrange; break;
   default:
      // never here
      assert(false);
      break;
   }
   switch (fieldId / 12) {
   case 0: colour -= 2; break;
   case 1: break;
   case 2: colour += 3; break;
   case 3: colour -= 6; break;
   case 4: colour -= 9; break;
   case 5:
      if (fieldId == 60)
         return kGray;
   default:
      // never here
      assert(false);
      break;
   }
   return colour;
}
