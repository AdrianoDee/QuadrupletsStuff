#include <iostream>
#include <fstream>
#include <algorithm>

#include <string>
#include <sstream>

#include <iterator>

#include "TGraph.h"
#include "TFile.h"
#include "TCanvas.h"
#include "TString.h"
#include "TH2D.h"
#include "TKey.h"
#include "TCollection.h"

//#define LEVELS 65536
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

void duplicates(std::string file, int numberOfFiles, int splitting, int limit = 10E7, int shrink = 8, int offset = 0)
{

    float fileprogress = 0.0;
    int barWidth = 50;

    //std::vector< TH2D* > clustersVector;
    //std::vector< std::string > clustersName;

    TH2D * thisCluster = 0;
    TH2D * prevCluster = 0;

    int filecounter = offset;

    for (int i = 1 + offset; i <= numberOfFiles+offset; ++i) {

      std::string fileName = file;
      fileName += std::to_string(i);
      fileName += ".root";

      TFile* histoFile = new TFile(fileName.data(),"READ");

      int noOfKeys = histoFile->GetNkeys();

      if(!(histoFile)){
        std::cout<<"Wrong file name. Skipping"<<std::endl;
        continue;
      }

      TIter next(histoFile->GetListOfKeys());
      TKey *key;

      int keycounter = 0;
      bool theSame = true;

      std::cout<<"----Reading from "<<fileName<<std::endl;

            //   while ((key=(TKey*)next())) {
            //
            //       fileprogress = (float)i/(float)(noOfKeys);
            //       ++keycounter;
            //
            //       std::cout << "[";
            //       int pos = barWidth * fileprogress;
            //
            //       for (int k = 0; k < barWidth; ++k)
            //       {
            //           if (k < pos) std::cout << "=";
            //           else if (k == pos) std::cout << ">";
            //           else std::cout << " ";
            //       }
            //       std::cout << "] " << int(fileprogress * 100.0) << " %\r";
            //       std::cout.flush();
            //
            //       std::vector<std::string>::iterator stringIterator;
            //
            //       std::string histoName(key->GetName());
            //       thisCluster = (TH2D*)key->ReadObj();
            //       //std::cout<<"Cluster name : "<<histoName<<std::endl;
            //       if(clustersName.size()>0 && clustersVector.size()>0)
            //       {
            //         stringIterator = find (clustersName.begin(), clustersName.end(), histoName);
            //         if (stringIterator != clustersName.end()){
            //           //std::cout<<"Found previous cluster with same name ... ";
            //           int nameIndex = stringIterator - clustersName.begin();
            //           for(int j = 1;j<thisCluster->GetNbinsX();++j)
            //             for(int i = 1;i<thisCluster->GetNbinsY();++i)
            //               if(thisCluster->GetBinContent(j,i)!=clustersVector[nameIndex]->GetBinContent(j,i))
            //               {
            //                 //std::cout<<" BUT not same content (\?\?). Keeping it."<<std::endl;
            //                 j=thisCluster->GetNbinsX()+1;
            //                 i=thisCluster->GetNbinsY()+1;
            //                 theSame = false;
            //               }
            //           if(theSame) ;//std::cout<<" and the same content (!). Not Keeping it."<<std::endl;
            //           else {
            //             clustersVector.push_back(thisCluster);
            //             clustersName.push_back(histoName);
            //           }
            //         }
            //         else
            //         {
            //           //std::cout<<"No duplicate found. Adding it"<<std::endl;
            //           clustersVector.push_back(thisCluster);
            //           clustersName.push_back(histoName);
            //         }
            //       }
            //       else
            //       {
            //         //std::cout<<"Is the first cluster. Adding it."<<std::endl;
            //         clustersVector.push_back(thisCluster);
            //         clustersName.push_back(histoName);
            //       }
            //
            //     if(clustersVector.size()!=clustersName.size())
            //     {
            //       std::cout<<"Something WRONG. Clusters vector size ("<<clustersVector.size()<<") is different from name vector size ("<<clustersName.size()<<"). RETURNING!"<<std::endl;
            //       return;
            //     }
            //
            //   }
            //
            //   //if(clustersVector.size()>limit) i = numberOfFiles+1;
            //
            // }
            //
            // std::cout<<"----Found "<<clustersVector.size()<<" clusters "<<std::endl;
            // std::cout<<"----Writing on a new cleaned .root file "<<std::endl;
            //
            // std::string outputFileName = "clustersOutput";
            // outputFileName += std::to_string(offset);
            // outputFileName += ".root";
            //
            // TFile* outputFile = new TFile(outputFileName.data(),"RECREATE");
            // outputFile->cd();
            //
            // for (size_t i = 0; i < clustersVector.size(); i++) {
            //
            //   fileprogress = (float)i/(float)(clustersVector.size());
            //
            //   std::cout << "[";
            //   int pos = barWidth * fileprogress;
            //
            //   for (int k = 0; k < barWidth; ++k)
            //   {
            //       if (k < pos) std::cout << "=";
            //       else if (k == pos) std::cout << ">";
            //       else std::cout << " ";
            //   }
            //   std::cout << "] " << int(fileprogress * 100.0) << " %\r";
            //   std::cout.flush();
            //
            //   clustersVector[i]->Write();
            // }
            //
            // std::cout<<"----Written "<<clustersVector.size()<<" clusters "<<std::endl;
            //
            // outputFile->Close();

    int doubcounter = 0;
    int counter = 0;
    int trueCounter = 0;
    int fakeCounter = 0;
    int iterations = 0;

    int doubletFlag = 0;
    int nHistoWritten = 0;
    int nHistoCounter = 0;

    int detId = -1;
    int prevDetId = -1;

    int trueOrNot = -1;

    int thisHitCounter = -1;
    int nextHitCounter = -1;
    int prevHitCounter = -1;

    std::string hitnumber;
    std::string detector;

    std::cout<<"----Writing on multiple txt files ( "<<splitting<<" clusters each )"<<std::endl;

    //while (doubletFlag ==0 && iterations<(int)clustersVector.size()) {
    while (doubletFlag ==0 && (key=(TKey*)next()))
    {
      //fileprogress = (float)iterations/(float)(clustersVector.size());
      fileprogress = (float)iterations/(float)(noOfKeys);

      std::cout << "[";
      int pos = barWidth * fileprogress;

      for (int k = 0; k < barWidth; ++k)
      {
          if (k < pos) std::cout << "=";
          else if (k == pos) std::cout << ">";
          else std::cout << " ";
      }
      std::cout << "] " << int(fileprogress * 100.0) << " %\r";
      std::cout.flush();

      std::ofstream clusters;
      std::ofstream clusterslabels;

      std::string clusternamefile = "./txts3/clustersfile";
      std::string labelnamefile = "./txts3/clusterslabel";
      clusternamefile += std::to_string(filecounter);
      labelnamefile += std::to_string(filecounter);
      clusternamefile += ".txt";
      labelnamefile += ".txt";

      clusters.open(clusternamefile, std::ofstream::app);
      clusterslabels.open(labelnamefile, std::ofstream::app);

      //thisCluster = clustersVector[iterations];
      thisCluster = (TH2D*)key->ReadObj();
      std::string thisName(thisCluster->GetName());
      std::vector<std::string> tokens;

      tokens = split(thisName,'_');

      // for (size_t i = 0; i < tokens.size(); i++) {
      //     std::cout<<"Token "<<i<<" : "<<tokens[i]<<std::endl;
      // }

      hitnumber = tokens[0];
      hitnumber += tokens[1];
      hitnumber += tokens[2];
      hitnumber += tokens[3];
      hitnumber += tokens[4];

      thisHitCounter = atoi(hitnumber.data());

      trueOrNot = atoi(&tokens[8][0]);

      detector = tokens[5];
      detector += tokens[6];
      detector += tokens[7];

      if(detector == "B00") detId = 0;
      if(detector == "B01") detId = 1;
      if(detector == "B02") detId = 2;
      if(detector == "B03") detId = 3;
      if(detector == "F10") detId = 4;
      if(detector == "F11") detId = 5;
      if(detector == "F12") detId = 6;
      if(detector == "F20") detId = 7;
      if(detector == "F21") detId = 8;
      if(detector == "F22") detId = 9;

      // std::cout<<"This Hit Counter : "<<thisHitCounter<<std::endl;
      // std::cout<<"Prev Hit Counter : "<<prevHitCounter<<std::endl;

      if(prevHitCounter == thisHitCounter - 1)
      {
        //std::cout<<"Adding"<<std::endl;

        int nBinsX = prevCluster->GetNbinsX();
        int nBinsY = prevCluster->GetNbinsY();

        clusterslabels<<(uint16_t)trueOrNot;

        if(trueOrNot==1.0 && trueCounter>limit*0.5) continue;
        if(trueOrNot==0.0 && fakeCounter>limit*0.5) continue;

        size_t start = 1;
        size_t end = 0;

        if(shrink<=nBinsX && nBinsX==nBinsY && (nBinsX-shrink)%2==0)
        {
          start = (size_t)((nBinsX-shrink)/2); ++start;
          end = (size_t)((nBinsX-shrink)/2);

        }

        for (size_t i = start; i <= nBinsX-end; ++i) {
          for (size_t j = start; j <= nBinsY-end; ++j) {

            //doublet.push_back((uint16_t)prevCluster->GetBinContent(i,j));
            if(counter!=0 || (i!=start || j!=start))
            clusters<<" "<<(uint16_t)prevCluster->GetBinContent(i,j)/LEVELS;
            else
            clusters<<(uint16_t)prevCluster->GetBinContent(i,j)/LEVELS;

          }
        }

        for (size_t i = start; i <= nBinsX-end; ++i) {
          for (size_t j = start; j <= nBinsY-end; ++j) {

            //doublet.push_back((uint16_t)thisCluster->GetBinContent(i,j));
            clusters<<" "<<(uint16_t)thisCluster->GetBinContent(i,j)/LEVELS;

          }
        }

        ++counter;
        ++doubcounter;

        if(trueOrNot==1.) ++trueCounter;
        if(trueOrNot==0.) ++fakeCounter;


      }

      prevHitCounter = thisHitCounter;
      prevCluster = thisCluster;
      prevDetId = detId;

      if(counter>splitting){
        counter = 0;
        ++filecounter;
      }

      if(doubcounter >= limit) doubletFlag = -1;

      ++iterations;

    }

    std::cout<<"Transfered to txt files, added "<<doubcounter<<std::endl;

    }



   }
