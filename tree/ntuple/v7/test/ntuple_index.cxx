#include "ntuple_test.hxx"

#ifdef R__USE_IMT
struct IMTRaii {
   IMTRaii() { ROOT::EnableImplicitMT(4); }
   ~IMTRaii() { ROOT::DisableImplicitMT(); }
};
#endif

TEST(RNTupleIndex, FromReader)
{
   FileRaii fileGuard("test_ntuple_index_from_reader.root");
   {
      auto model = RNTupleModel::Create();
      auto fld = model->MakeField<std::uint64_t>("fld");

      auto ntuple = RNTupleWriter::Recreate(std::move(model), "ntuple", fileGuard.GetPath());

      for (int i = 0; i < 10; ++i) {
         *fld = i * 2;
         ntuple->Fill();
      }
   }

   auto ntuple = RNTupleReader::Open("ntuple", fileGuard.GetPath());
   auto index = ntuple->CreateIndex("fld");

   auto fld = ntuple->GetView<std::uint64_t>("fld");

   for (unsigned i = 0; i < ntuple->GetNEntries(); ++i) {
      auto fldValue = fld(i);
      EXPECT_EQ(fldValue, i * 2);
      EXPECT_EQ(index->GetEntryIndex(&fldValue), i);
   }
}

TEST(RNTupleIndex, FromPageSource)
{
   FileRaii fileGuard("test_ntuple_index_from_page_source.root");
   {
      auto model = RNTupleModel::Create();
      auto fld = model->MakeField<std::uint64_t>("fld");

      auto ntuple = RNTupleWriter::Recreate(std::move(model), "ntuple", fileGuard.GetPath());

      for (int i = 0; i < 10; ++i) {
         *fld = i * 2;
         ntuple->Fill();
      }
   }

   auto pageSource = RPageSource::Create("ntuple", fileGuard.GetPath());
   pageSource->Attach();

   auto index = ROOT::Experimental::Internal::CreateRNTupleIndex("fld", *pageSource);

   auto ntuple = RNTupleReader::Open("ntuple", fileGuard.GetPath());
   auto fld = ntuple->GetView<std::uint64_t>("fld");

   for (unsigned i = 0; i < ntuple->GetNEntries(); ++i) {
      auto fldValue = fld(i);
      EXPECT_EQ(fldValue, i * 2);
      EXPECT_EQ(index->GetEntryIndex(&fldValue), i);
   }
}

#ifdef R__USE_IMT
TEST(RNTupleIndex, FromReaderMT)
{
   FileRaii fileGuard("test_ntuple_index_from_reader_mt.root");
   {
      auto model = RNTupleModel::Create();
      auto fld = model->MakeField<std::uint64_t>("fld");

      RNTupleWriteOptions opts;
      opts.SetApproxZippedClusterSize(512);
      auto ntuple = RNTupleWriter::Recreate(std::move(model), "ntuple", fileGuard.GetPath(), opts);

      for (int i = 0; i < 10; ++i) {
         *fld = i * 2;
         ntuple->Fill();
      }
   }

   IMTRaii _;

   auto ntuple = RNTupleReader::Open("ntuple", fileGuard.GetPath());
   auto index = ntuple->CreateIndex("fld");

   auto fld = ntuple->GetView<std::uint64_t>("fld");

   for (unsigned i = 0; i < ntuple->GetNEntries(); ++i) {
      auto fldValue = fld(i);
      EXPECT_EQ(fldValue, i * 2);
      EXPECT_EQ(index->GetEntryIndex(&fldValue), i);
   }
}

TEST(RNTupleIndex, FromPageSourceMT)
{
   FileRaii fileGuard("test_ntuple_index_from_page_source.root");
   {
      auto model = RNTupleModel::Create();
      auto fld = model->MakeField<std::uint64_t>("fld");

      RNTupleWriteOptions opts;
      opts.SetApproxZippedClusterSize(512);
      auto ntuple = RNTupleWriter::Recreate(std::move(model), "ntuple", fileGuard.GetPath(), opts);

      for (int i = 0; i < 10; ++i) {
         *fld = i * 2;
         ntuple->Fill();
      }
   }

   IMTRaii _;

   auto pageSource = RPageSource::Create("ntuple", fileGuard.GetPath());
   pageSource->Attach();

   auto index = ROOT::Experimental::Internal::CreateRNTupleIndex("fld", *pageSource);

   auto ntuple = RNTupleReader::Open("ntuple", fileGuard.GetPath());
   auto fld = ntuple->GetView<std::uint64_t>("fld");

   for (unsigned i = 0; i < ntuple->GetNEntries(); ++i) {
      auto fldValue = fld(i);
      EXPECT_EQ(fldValue, i * 2);
      EXPECT_EQ(index->GetEntryIndex(&fldValue), i);
   }
}
#endif
