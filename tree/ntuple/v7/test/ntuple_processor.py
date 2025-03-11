import unittest

import ROOT

RNTupleModel = ROOT.Experimental.RNTupleModel
RNTupleProcessor = ROOT.Experimental.RNTupleProcessor
RNTupleOpenSpec = ROOT.Experimental.RNTupleOpenSpec
RNTupleWriter = ROOT.Experimental.RNTupleWriter

class NTupleProcessor(unittest.TestCase):
   """Tests for using the RNTupleProcessor from Python."""

   def test_single_processor_basic(self):
      """Can process a single RNTuple."""

      model = RNTupleModel.Create()
      model.MakeField["float"]("x")

      with RNTupleWriter.Recreate(model, "ntuple", "test_ntuple_processor_py_single_proc_basic.root") as writer:
         entry = writer.CreateEntry()
         for i in range(5):
            entry["x"] = i * .1
            writer.Fill(entry)

      ntuple = RNTupleOpenSpec("ntuple", "test_ntuple_processor_py_single_proc_basic.root")
      processor = RNTupleProcessor.Create(ntuple)

      for i, entry in enumerate(processor):
         self.assertAlmostEqual(i * .1, entry["x"])

      self.assertEqual(5, processor.GetNEntriesProcessed())

   def test_single_processor_model(self):
      """Can process a single RNTuple."""

      write_model = RNTupleModel.Create()
      write_model.MakeField["float"]("x")
      write_model.MakeField["float"]("y")

      with RNTupleWriter.Recreate(write_model, "ntuple", "test_ntuple_processor_py_single_proc_model.root") as writer:
         entry = writer.CreateEntry()
         for i in range(5):
            entry["x"] = i * .1
            entry["y"] = i * .2
            writer.Fill(entry)

      read_model = RNTupleModel.Create()
      read_model.MakeField["float"]("x")

      ntuple = RNTupleOpenSpec("ntuple", "test_ntuple_processor_py_single_proc_model.root")
      processor = RNTupleProcessor.Create(ntuple, read_model)

      for i, proc_entry in enumerate(processor):
         self.assertAlmostEqual(i * .1, proc_entry["x"])
         with self.assertRaises(Exception):
            # Field y does not exist in the model passed to processor
            proc_entry["y"]

      self.assertEqual(5, processor.GetNEntriesProcessed())
