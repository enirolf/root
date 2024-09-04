#include "ntuple_test.hxx"

#include <ROOT/RNTupleJoinProcessor.hxx>

using ROOT::Experimental::RNTupleModel;
using ROOT::Experimental::RNTupleWriter;
using ROOT::Experimental::Internal::RNTupleJoinProcessor;
using ROOT::Experimental::Internal::RNTupleSourceSpec;

TEST(RNTupleJoinProcessor, Basic)
{
   FileRaii fileGuard1("test_ntuple_join_processor_basic1.root");
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
   FileRaii fileGuard2("test_ntuple_join_processor_basic2.root");
   {
      auto model = RNTupleModel::Create();
      auto fldY = model->MakeField<std::vector<float>>("y");
      auto ntuple = RNTupleWriter::Recreate(std::move(model), "ntuple2", fileGuard2.GetPath());

      for (unsigned i = 0; i < 5; ++i) {
         *fldY = {static_cast<float>(i * 0.2), 3.14, static_cast<float>(i * 1.3)};
         ntuple->Fill();
      }
   }
   FileRaii fileGuard3("test_ntuple_join_processor_basic3.root");
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
      RNTupleJoinProcessor proc(ntuples);
      FAIL() << "ntuples with the same name cannot be joined horizontally";
   } catch (const RException &err) {
      EXPECT_THAT(err.what(), testing::HasSubstr("horizontal joining of RNTuples with the same name is not allowed"));
   }

   std::vector<RNTupleSourceSpec> ntuples = {
      {"ntuple1", fileGuard1.GetPath()}, {"ntuple2", fileGuard2.GetPath()}, {"ntuple3", fileGuard3.GetPath()}};
   RNTupleJoinProcessor processor(ntuples);

   std::vector<float> yExpected;

   auto i = processor.GetPtr<int>("i");
   EXPECT_THAT(processor.GetActiveFields(), testing::ElementsAre("i"));
   for (auto &entry : processor) {
      EXPECT_EQ(*i, *entry.GetPtr<float>("x"));

      if (*i > 3) {
         yExpected = {static_cast<float>(*i * 0.2), 3.14, static_cast<float>(*i * 1.3)};
         EXPECT_EQ(yExpected, *entry.GetPtr<std::vector<float>>("ntuple2.y"));
         EXPECT_EQ(*i * 2, *entry.GetPtr<std::uint64_t>("ntuple3.z"));

         EXPECT_THAT(processor.GetActiveFields(), testing::UnorderedElementsAre("i", "x", "ntuple2.y", "ntuple3.z"));
      } else {
         EXPECT_THAT(processor.GetActiveFields(), testing::UnorderedElementsAre("i", "x"));
      }
   }
   EXPECT_EQ(*i, 4);
}

TEST(RNTupleJoinProcessor, IdenticalFieldNames)
{
   FileRaii fileGuard1("test_ntuple_join_processor_field_names1.root");
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
   FileRaii fileGuard2("test_ntuple_join_processor_field_names2.root");
   {
      auto model = RNTupleModel::Create();
      auto fldI = model->MakeField<int>("i");
      auto fldY = model->MakeField<std::vector<float>>("y");
      auto ntuple = RNTupleWriter::Recreate(std::move(model), "ntuple2", fileGuard2.GetPath());

      for (*fldI = 0; *fldI < 5; ++(*fldI)) {
         *fldY = {static_cast<float>(*fldI * 0.2), 3.14, static_cast<float>(*fldI * 1.3)};
         ntuple->Fill();
      }
   }

   std::vector<RNTupleSourceSpec> ntuples = {{"ntuple1", fileGuard1.GetPath()}, {"ntuple2", fileGuard2.GetPath()}};
   RNTupleJoinProcessor processor(ntuples);

   std::vector<float> yExpected;

   auto i = processor.GetPtr<int>("i");
   for (auto &entry : processor) {
      EXPECT_EQ(*i, *entry.GetPtr<int>("ntuple2.i"));
      EXPECT_EQ(static_cast<float>(*i), *entry.GetPtr<float>("x"));

      yExpected = {static_cast<float>(*i * 0.2), 3.14, static_cast<float>(*i * 1.3)};
      EXPECT_EQ(yExpected, *entry.GetPtr<std::vector<float>>("ntuple2.y"));
   }
   EXPECT_EQ(*i, 4);
}

TEST(RNTupleJoinProcessor, UnalignedBasic)
{
   FileRaii fileGuard1("test_ntuple_join_processor_unaligned_basic1.root");
   {
      auto model = RNTupleModel::Create();
      auto fldI = model->MakeField<int>("i");
      auto fldX = model->MakeField<float>("x");
      auto ntuple = RNTupleWriter::Recreate(std::move(model), "ntuple1", fileGuard1.GetPath());

      for (*fldI = 0; *fldI < 5; ++(*fldI)) {
         *fldX = *fldI * 0.5;
         ntuple->Fill();
      }
   }
   FileRaii fileGuard2("test_ntuple_join_processor_unaligned_basic2.root");
   {
      auto model = RNTupleModel::Create();
      auto fldI = model->MakeField<int>("i");
      auto fldY = model->MakeField<float>("y");
      auto ntuple = RNTupleWriter::Recreate(std::move(model), "ntuple2", fileGuard2.GetPath());

      for (*fldI = 0; *fldI < 5; ++(*fldI)) {
         if (*fldI % 2 == 1) {
            *fldY = static_cast<float>(*fldI * 0.2);
            ntuple->Fill();
         }
      }
   }

   std::vector<RNTupleSourceSpec> ntuples = {{"ntuple1", fileGuard1.GetPath()}, {"ntuple2", fileGuard2.GetPath()}};
   RNTupleJoinProcessor processor(ntuples, {"i"});

   auto i = processor.GetPtr<int>("i");
   auto x = processor.GetPtr<float>("x");
   auto y = processor.GetPtr<float>("ntuple2.y");
   for (auto &_ : processor) {
      EXPECT_FLOAT_EQ(static_cast<float>(*i * 0.5), *x);

      if (*i % 2 == 1) {
         EXPECT_FLOAT_EQ(static_cast<float>(*i * 0.2), *y);
      } else {
         EXPECT_EQ(0., *y);
      }
   }
}
