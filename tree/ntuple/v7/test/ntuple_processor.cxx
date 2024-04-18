#include "ntuple_test.hxx"

#include <ROOT/RNTupleHorizontalProcessor.hxx>
#include <ROOT/RNTupleProcessor.hxx>

using ROOT::Experimental::RNTupleHorizontalProcessor;
using ROOT::Experimental::RNTupleProcessor;
using ROOT::Experimental::RNTupleSourceSpec;

TEST(RNTupleProcessor, Basic)
{
   FileRaii fileGuard("test_ntuple_processor_basic.root");
   {
      auto model = RNTupleModel::Create();
      auto fldX = model->MakeField<float>("x");
      auto ntuple = RNTupleWriter::Recreate(std::move(model), "ntuple", fileGuard.GetPath());

      for (unsigned i = 0; i < 10; ++i) {
         *fldX = static_cast<float>(i);
         ntuple->Fill();
      }
   }

   std::vector<RNTupleSourceSpec> ntuples;
   try {
      RNTupleProcessor proc(ntuples);
      FAIL() << "creating a processor without at least one RNTuple should throw";
   } catch (const RException &err) {
      EXPECT_THAT(err.what(), testing::HasSubstr("at least one RNTuple must be provided"));
   }

   ntuples = {{"ntuple", fileGuard.GetPath()}};

   int nEntries = 0;
   for (const auto &entry : RNTupleProcessor(ntuples)) {
      auto x = entry->GetPtr<float>("x");
      EXPECT_EQ(static_cast<float>(entry.GetGlobalEntryIndex()), *x);
      ++nEntries;
   }
   EXPECT_EQ(nEntries, 10);
}

TEST(RNTupleProcessor, SimpleChain)
{
   FileRaii fileGuard1("test_ntuple_processor_simple_chain1.root");
   {
      auto model = RNTupleModel::Create();
      auto fldX = model->MakeField<float>("x");
      auto fldY = model->MakeField<std::vector<float>>("y");
      auto ntuple = RNTupleWriter::Recreate(std::move(model), "ntuple", fileGuard1.GetPath());

      for (unsigned i = 0; i < 5; ++i) {
         *fldX = static_cast<float>(i);
         *fldY = {static_cast<float>(i), static_cast<float>(i * 2)};
         ntuple->Fill();
      }
   }
   FileRaii fileGuard2("test_ntuple_processor_simple_chain2.root");
   {
      auto model = RNTupleModel::Create();
      auto fldX = model->MakeField<float>("x");
      auto fldY = model->MakeField<std::vector<float>>("y");
      auto ntuple = RNTupleWriter::Recreate(std::move(model), "ntuple", fileGuard2.GetPath());

      for (unsigned i = 5; i < 8; ++i) {
         *fldX = static_cast<float>(i);
         *fldY = {static_cast<float>(i), static_cast<float>(i * 2)};
         ntuple->Fill();
      }
   }

   std::vector<RNTupleSourceSpec> ntuples = {{"ntuple", fileGuard1.GetPath()}, {"ntuple", fileGuard2.GetPath()}};

   std::uint64_t nEntries = 0;
   for (const auto &entry : RNTupleProcessor(ntuples)) {
      auto x = entry->GetPtr<float>("x");
      EXPECT_EQ(static_cast<float>(entry.GetGlobalEntryIndex()), *x);

      auto y = entry->GetPtr<std::vector<float>>("y");
      std::vector<float> yExp = {static_cast<float>(entry.GetGlobalEntryIndex()), static_cast<float>(nEntries * 2)};
      EXPECT_EQ(yExp, *y);

      if (entry.GetLocalEntryIndex() == 0) {
         EXPECT_THAT(entry.GetGlobalEntryIndex(), testing::AnyOf(0, 5));
      }

      ++nEntries;
   }
   EXPECT_EQ(nEntries, 8);
}

TEST(RNTupleProcessor, EmptyNTuples)
{
   FileRaii fileGuard1("test_ntuple_processor_empty_ntuples1.root");
   {
      auto model = RNTupleModel::Create();
      auto fldX = model->MakeField<float>("x");
      auto ntuple = RNTupleWriter::Recreate(std::move(model), "ntuple", fileGuard1.GetPath());
   }
   FileRaii fileGuard2("test_ntuple_processor_empty_ntuples2.root");
   {
      auto model = RNTupleModel::Create();
      auto fldX = model->MakeField<float>("x");
      auto ntuple = RNTupleWriter::Recreate(std::move(model), "ntuple", fileGuard2.GetPath());

      for (unsigned i = 0; i < 2; ++i) {
         *fldX = static_cast<float>(i);
         ntuple->Fill();
      }
   }
   FileRaii fileGuard3("test_ntuple_processor_empty_ntuples3.root");
   {
      auto model = RNTupleModel::Create();
      auto fldX = model->MakeField<float>("x");
      auto ntuple = RNTupleWriter::Recreate(std::move(model), "ntuple", fileGuard3.GetPath());
   }
   FileRaii fileGuard4("test_ntuple_processor_empty_ntuples4.root");
   {
      auto model = RNTupleModel::Create();
      auto fldX = model->MakeField<float>("x");
      auto ntuple = RNTupleWriter::Recreate(std::move(model), "ntuple", fileGuard4.GetPath());

      for (unsigned i = 2; i < 5; ++i) {
         *fldX = static_cast<float>(i);
         ntuple->Fill();
      }
   }
   FileRaii fileGuard5("test_ntuple_processor_empty_ntuples5.root");
   {
      auto model = RNTupleModel::Create();
      auto fldX = model->MakeField<float>("x");
      auto ntuple = RNTupleWriter::Recreate(std::move(model), "ntuple", fileGuard5.GetPath());
   }

   std::vector<RNTupleSourceSpec> ntuples = {{"ntuple", fileGuard1.GetPath()},
                                             {"ntuple", fileGuard2.GetPath()},
                                             {"ntuple", fileGuard3.GetPath()},
                                             {"ntuple", fileGuard4.GetPath()},
                                             {"ntuple", fileGuard5.GetPath()}};

   std::uint64_t nEntries = 0;
   for (const auto &entry : RNTupleProcessor(ntuples)) {
      auto x = entry->GetPtr<float>("x");
      EXPECT_EQ(static_cast<float>(nEntries), *x);
      ++nEntries;
   }
   EXPECT_EQ(nEntries, 5);
}

TEST(RNTupleProcessor, ChainUnalignedModels)
{
   FileRaii fileGuard1("test_ntuple_processor_simple_chain1.root");
   {
      auto model = RNTupleModel::Create();
      auto fldX = model->MakeField<float>("x", 0.);
      auto fldY = model->MakeField<char>("y", 'a');
      auto ntuple = RNTupleWriter::Recreate(std::move(model), "ntuple", fileGuard1.GetPath());
      ntuple->Fill();
   }
   FileRaii fileGuard2("test_ntuple_processor_simple_chain2.root");
   {
      auto model = RNTupleModel::Create();
      auto fldX = model->MakeField<float>("x", 1.);
      auto ntuple = RNTupleWriter::Recreate(std::move(model), "ntuple", fileGuard2.GetPath());
      ntuple->Fill();
   }

   std::vector<RNTupleSourceSpec> ntuples = {{"ntuple", fileGuard1.GetPath()}, {"ntuple", fileGuard2.GetPath()}};

   RNTupleProcessor proc(ntuples);
   auto entry = proc.begin();
   auto x = (*entry)->GetPtr<float>("x");
   auto y = (*entry)->GetPtr<char>("y");
   EXPECT_EQ(0., *x);
   EXPECT_EQ('a', *y);

   try {
      entry++;
      FAIL() << "trying to connect a new page source containing additional (unknown) fields is not supported";
   } catch (const RException &err) {
      EXPECT_THAT(err.what(), testing::HasSubstr("field \"y\" not found in current RNTuple"));
   }
}

TEST(RNTupleHorizontalProcessor, Basic)
{
   FileRaii fileGuard1("test_ntuple_processor_simple_join1.root");
   {
      auto model = RNTupleModel::Create();
      auto fldI = model->MakeField<int>("i");
      auto fldX = model->MakeField<float>("x");
      auto ntuple = RNTupleWriter::Recreate(std::move(model), "ntuple1", fileGuard1.GetPath());

      for (*fldI = 0; *fldI < 5; ++(*fldI)) {
         *fldX = static_cast<float>(*fldI);
         ntuple->Fill();
      }
   }
   FileRaii fileGuard2("test_ntuple_processor_simple_join2.root");
   {
      auto model = RNTupleModel::Create();
      auto fldY = model->MakeField<std::vector<float>>("y");
      auto ntuple = RNTupleWriter::Recreate(std::move(model), "ntuple2", fileGuard2.GetPath());

      for (unsigned i = 0; i < 5; ++i) {
         *fldY = {static_cast<float>(i * 0.2), 3.14, static_cast<float>(i * 1.3)};
         ntuple->Fill();
      }
   }
   FileRaii fileGuard3("test_ntuple_processor_simple_join3.root");
   {
      auto model = RNTupleModel::Create();
      auto fldZ = model->MakeField<std::uint64_t>("z");
      auto ntuple = RNTupleWriter::Recreate(std::move(model), "ntuple3", fileGuard3.GetPath());

      for (unsigned i = 0; i < 5; ++i) {
         *fldZ = static_cast<std::uint64_t>(i * 2);
         ntuple->Fill();
      }
   }

   try {
      std::vector<RNTupleSourceSpec> ntuples = {{"ntuple1", fileGuard1.GetPath()}, {"ntuple1", fileGuard1.GetPath()}};
      RNTupleHorizontalProcessor proc(ntuples);
      FAIL() << "ntuples with the same name cannot be joined horizontally";
   } catch (const RException &err) {
      EXPECT_THAT(err.what(), testing::HasSubstr("horizontal joining of RNTuples with the same name is not allowed"));
   }

   std::vector<RNTupleSourceSpec> ntuples = {
      {"ntuple1", fileGuard1.GetPath()}, {"ntuple2", fileGuard2.GetPath()}, {"ntuple3", fileGuard3.GetPath()}};
   RNTupleHorizontalProcessor proc(ntuples);
   auto &model = proc.GetModel();

   EXPECT_EQ(model.GetField("x").GetFieldName(), "x");
   EXPECT_EQ(model.GetField("ntuple2.y").GetFieldName(), "y");
   EXPECT_EQ(model.GetField("ntuple3.z").GetFieldName(), "z");

   std::shared_ptr<int> fldI;
   for (auto entry = proc.begin(); entry != proc.end(); ++entry) {
      fldI = (*entry).GetPtr<int>("i");
      EXPECT_EQ(static_cast<float>(*fldI), *(*entry).GetPtr<float>("x"));
      std::vector<float> yExpected = {static_cast<float>(*fldI * 0.2), 3.14, static_cast<float>(*fldI * 1.3)};
      EXPECT_EQ(yExpected, *(*entry).GetPtr<std::vector<float>>("y"));
      EXPECT_EQ(static_cast<std::uint64_t>(*fldI * 2), *(*entry).GetPtr<std::uint64_t>("z"));
   }
   EXPECT_EQ(*fldI, 4);
}

// TODO(fdegeus) Add test cases for duplicate field names, mismatched entry numbers
