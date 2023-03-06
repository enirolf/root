// Author: Enrico Guiraud, David Poulton 2022

/*************************************************************************
 * Copyright (C) 1995-2022, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "ReadSpeedCLI.hxx"
#include "ReadSpeedTTree.hxx"
#include "ReadSpeedRNTuple.hxx"

using namespace ReadSpeed;

int main(int argc, char **argv)
{
   auto args = ParseArgs(argc, argv);

   if (!args.fShouldRun)
      return 1; // ParseArgs has printed the --help, has run the --test or has encountered an issue and logged about it

   if (args.fEvalNTuple) {
      PrintThroughput(RSRNTuple::EvalThroughput(args.fData, args.fNThreads));
   }

   PrintThroughput(RSTTree::EvalThroughput(args.fData, args.fNThreads));

   return 0;
}
