#include <iostream>
#include <fstream>
#include <algorithm>

#include "TGraph.h"
#include "TFile.h"
#include "TH2D.h"
#include "TKey.h"
#include "TIter.h"

void maximum(int pad)
{

  TFile *inputFile = TFile::Open("clusters.root");

  TIter next(f.GetListOfKeys());
   TKey *key;
   while ((key=(TKey*)next())) {
      printf("key: %s points to an object of class: %s at %dn",
      key->GetName(),
      key->GetClassName(),key->GetSeekKey());
   }

}
