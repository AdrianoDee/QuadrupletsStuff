#include <iostream>
#include <fstream>
#include <algorithm>

#include <string>
#include <sstream>

#include <iterator>

#include "TGraph.h"
#include "TFile.h"
#include "TCanvas.h"
#include "TH2D.h"
#include "TKey.h"
#include "TCollection.h"

//#define LEVELS 4369
#define LEVELS 1

void split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

void testandtrain(int inDet,int outDet, double testfrac = 0.3 , double fakefrac = 0.3)
{
   //TCanvas *canvas = new TCanvas("canvas","canvas",1000,1000);

   std::string clusterpath = "./clustertrain" + std::to_string(inDet) + "_" + std::to_string(outDet) + ".txt";
   std::string clusterpathlabel = "./clusterstrainlabels" + std::to_string(inDet) + "_" + std::to_string(outDet) + ".txt";


   std::ifstream clusterIn(clusterpath.data());
   std::ifstream clusterLabelsIn(clusterpathlabel.data());

   //TFile *inputFile = TFile::Open(clusterpath);

   std::string buffer;

   std::ofstream clustersTrain;
   std::ofstream clustersTrainLabels;
   std::ofstream clustersTest;
   std::ofstream clustersTestLabels;

   buffer = "clustersTrain";
   buffer += std::to_string(inDet);
   buffer += "_";
   buffer += std::to_string(outDet);
   buffer += ".txt";

   clustersTrain.open(buffer, std::ofstream::out);

   buffer = "clustersTrainlabels";
   buffer += std::to_string(inDet);
   buffer += "_";
   buffer += std::to_string(outDet);
   buffer += ".txt";

   clustersTrainLabels.open(buffer, std::ofstream::out);

   buffer = "clustersTest";
   buffer += std::to_string(inDet);
   buffer += "_";
   buffer += std::to_string(outDet);
   buffer += ".txt";

   clustersTest.open(buffer, std::ofstream::out);

   buffer = "clustersTestlabels";
   buffer += std::to_string(inDet);
   buffer += "_";
   buffer += std::to_string(outDet);
   buffer += ".txt";

   clustersTestLabels.open(buffer, std::ofstream::out);

   int trueTest = 0;
   int trueTrain = 0;
   int fakeTest = 0;
   int fakeTrain = 0;

   int trainCounter = 0;
   int testCounter = 0;

   int counter = 0;
   int trueOrNot = -1;
   std::vector<int> trueOrNotV;
   double cluster[128];

    if (clusterIn.good() && clusterLabelsIn.good())
      while(clusterLabelsIn >> trueOrNot)
      {
        trueOrNotV.push_back(trueOrNot);
        ++counter;
      }

   int trueTestLimit  = (int)(((double)counter)*testfrac*(1.0-fakefrac));
   int trueTrainLimit = (int)(((double)counter)*(1.0-testfrac)*(1.0-fakefrac));
   int fakeTestLimit  = (int)(((double)counter)*testfrac*(fakefrac));
   int fakeTrainLimit = (int)(((double)counter)*(1.0-testfrac)*(fakefrac));

   bool trainTrue = false;
   bool trainFake = false;
   bool testTrue  = false;
   bool testFake  = false;

   int index = 0;
   if (clusterIn.good() && clusterLabelsIn.good() && index<(int)trueOrNotV.size()) {

       double firstPixel = 0.0;
       int thisTrue = trueOrNotV[index];

       testFake = (thisTrue==0.0 && fakeTest<=fakeTestLimit);
       trainFake = (thisTrue==0.0 && fakeTrain<=fakeTrainLimit);
       testTrue = (thisTrue==1.0 && trueTest<=trueTestLimit);
       trainTrue = (thisTrue==1.0 && trueTrain<=trueTrainLimit);

       if((testFake) || (trainFake) || (testTrue) || (trainTrue))
       {
         while(clusterIn >> firstPixel) {

           cluster[0]  = firstPixel;
           for(int k = 1; k<128;++k)
           {
            clusterIn >> cluster[k];
           }
         }

         if(trainTrue)
         {
           if(trueTrain==0) clustersTrainLabels<<thisTrue;
           else clustersTrainLabels<<" "<<thisTrue;

           for(int k = 0; k<128;++k)
           {
            if(k==0 && trueTrain==0) clustersTrain<<cluster[k];
            else clustersTrain<<" "<<cluster[k];
           }
           ++trainCounter;
           ++trueTrain;
         }
         else if (trainFake)
         {
           if(trueTrain==0) clustersTrainLabels<<thisTrue;
           else clustersTrainLabels<<" "<<thisTrue;

           for(int k = 0; k<128;++k)
           {
            if(k==0 && fakeTrain==0) clustersTrain<<cluster[k];
            else clustersTrain<<" "<<cluster[k];
           }
           ++trainCounter;
           ++fakeTrain;
         }
         else if (testTrue)
         {
           if(trueTrain==0) clustersTestLabels<<thisTrue;
           else clustersTestLabels<<" "<<thisTrue;

           for(int k = 0; k<128;++k)
           {
            if(k==0 && testCounter==0) clustersTest<<cluster[k];
            else clustersTest<<" "<<cluster[k];
           }
           ++testCounter;
           ++trueTest;
         }
         else if (testFake)
         {
           if(trueTrain==0) clustersTestLabels<<thisTrue;
           else clustersTestLabels<<" "<<thisTrue;

           for(int k = 0; k<128;++k)
           {
            if(k==0 && testCounter==0) clustersTest<<cluster[k];
            else clustersTest<<" "<<cluster[k];
           }
           ++testCounter;
           ++fakeTest;
         }

         ++index;


       }


   }

   clustersTestLabels.close();
   clustersTest.close();
   clustersTrainLabels.close();
   clustersTrain.close();
   clusterLabelsIn.close();
   clusterIn.close();

   }
