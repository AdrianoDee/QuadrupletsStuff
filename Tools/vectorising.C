#include <iostream>
#include <fstream>
#include <algorithm>
#include <map>

#include <string>
#include <sstream>

#include <iterator>

#include "TGraph.h"
#include "TFile.h"
#include "TCanvas.h"
#include "TH2D.h"
#include "TKey.h"
#include "TCollection.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

//#define DEBUG 1

void debug(int line) {
  #ifdef DEBUG
  std::cout <<"Debugging on line " <<line <<std::endl;
  #endif
}

string GetStdoutFromCommand(string cmd) {

        string data;
        FILE * stream;
        const int max_buffer = 256;
        char buffer[max_buffer];
        cmd.append(" 2>&1");

        stream = popen(cmd.c_str(), "r");
        if (stream) {
        while (!feof(stream))
        if (fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
        pclose(stream);
        }
        return data;
}

//#define LEVELS 65535
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

void vectorise(std::string clusterpath = "clusters200.root",long int noFiles = 20000000, bool test = false, long int limit = 10E10, int shrink = 8)
{

   //Getting File List
   string files = "ls " + clusterpath + "*";
   string ls = GetStdoutFromCommand(files);
   std::vector<std::string> fileNames = split(ls,'\n');

   for (size_t i = 0; i < fileNames.size(); i++) {
     std::cout<<fileNames[i]<<std::endl;
   }

   std::string buffer;
   char bufferstring[256];

   std::ofstream cluster("./txts/clusterstotal.txt",std::ofstream::out);
   std::ofstream clusterlabels("./txts/clusterslabelstotal.txt",std::ofstream::out);;

   std::ofstream clustertrain;
   std::ofstream clusterlabelstrain;

   std::ofstream clusters[10][10];
   std::ofstream clusterslabels[10][10];

   std::ofstream clusterstrain[10][10];
   std::ofstream clusterslabelstrain[10][10];

  //  std::map< std::pair<std::pair<int,int>,std::pair<int,int>>, std::ofstream> clustersmodules;
  //  std::map< std::pair<std::pair<int,int>,std::pair<int,int>>, std::ofstream> clustersmoduleslabels;
   std::map< std::pair<std::pair<int,int>,std::pair<int,int>>, int> clustersmodulescounter;
   std::map< std::pair<std::pair<int,int>,std::pair<int,int>>, int> clustersmodulescountertrain;
  //  std::map< std::pair<std::pair<int,int>,std::pair<int,int>>, std::ofstream>::iterator itModules;
   std::map< std::pair<std::pair<int,int>,std::pair<int,int>>, int>::iterator itModules;

   int trueCounterLimit[10][10];
   int fakeCounterLimit[10][10];

   int trueCounterTrain[10][10];
   int fakeCounterTrain[10][10];

   int trueCounter[10][10];
   int fakeCounter[10][10];

   int trueCount=0;
   int fakeCount=0;

   int trueCountLimit;
   int fakeCountLimit;

   int trueCountTrain=0;
   int fakeCountTrain=0;

   int detsGlobalDoubCounter[10][10];
   int detsGlobalDoubCounterTrain[10][10];

   TH2D * thisCluster = 0;
   TH2D * prevCluster = 0;

     for (int i = 0; i < 10; ++i) {
       for (int j = 0; j < 10; ++j) {
         sprintf(bufferstring,"./txts/clusters%d_%d.txt",i,j);
         clusters[i][j].open(bufferstring,std::ofstream::out);
         sprintf(bufferstring,"./txts/clusterslabels%d_%d.txt",i,j);
         clusterslabels[i][j].open(bufferstring,std::ofstream::out);
         trueCounter[i][j] = 0;
         trueCounter[i][j] = 0;
         detsGlobalDoubCounter[i][j] = 0;
         detsGlobalDoubCounterTrain[i][j] = 0;
       }
     }


     if(!test)
     {
       for (int i = 0; i < 10; ++i) {
         for (int j = 0; j < 10; ++j) {
           sprintf(bufferstring,"./txts/test/clusterstrain%d_%d.txt",i,j);
           clusterstrain[i][j].open(bufferstring,std::ofstream::out);
           sprintf(bufferstring,"./txts/test/clusterstrainlabels%d_%d.txt",i,j);
           clusterslabelstrain[i][j].open(bufferstring,std::ofstream::out);
           trueCounterTrain[i][j] = 0;
           fakeCounterTrain[i][j] = 0;
         }
       }
     }
     else
     {
       for (int i = 0; i < 10; ++i) {
         for (int j = 0; j < 10; ++j) {
           sprintf(bufferstring,"./txts/test/clusterstest%d_%d.txt",i,j);
           clusterstrain[i][j].open(bufferstring,std::ofstream::out);
           sprintf(bufferstring,"./txts/test/clusterstestlabels%d_%d.txt",i,j);
           clusterslabelstrain[i][j].open(bufferstring,std::ofstream::out);
           trueCounterTrain[i][j] = 0;
           fakeCounterTrain[i][j] = 0;
         }
       }
     }

   // std::vector< TH2D *> quadrupletCluster;
   // TH2D * secondCluster = 0;
   // TH2D * thirdCluster = 0;
   // TH2D * fourthCluster = 0;

  std::string detector;
  std::string hitnumber;

  int thisHitCounter = -1;
  int nextHitCounter = -1;
  int prevHitCounter = -1;

  int globalCounter = 0;
  int globalDoubCounter = 0;

  TFile *inputFile;

   TCanvas *canvas = new TCanvas("canvas","canvas",1000,1000);
   for (size_t i = 0; i < fileNames.size(); i++) {
     debug(__LINE__);

     std::cout<<"=================================================="<<std::endl;
     std::cout<<"Reading File - "<<fileNames[i]<<" no. "<<i<<" on "<<fileNames.size()<<std::endl;

      inputFile = new TFile(fileNames[i].data(),"READ");

     if(!(inputFile)) {
       std::cout<<"Invalid filename "<<fileNames[i]<<" - skipping "<<std::endl;
       continue;
     }else{
       std::cout<<"With "<<inputFile->GetNkeys()<<" keys "<<std::endl;
     }
     //std::vector< std::vector<uint16_t> > doublets;

     // trueOrNot | det1 | det2 | x10y10 x11y10 x12y10 .... x1ny1n | x20 y20 ... x2n y2n |

     TIter next(inputFile->GetListOfKeys());
     TKey *key;


     int counter = 0;
     int doubcounter = 0;
     int clusterPacked = 0;
     int fakecounter = 0;

     int detId = -1;
     int prevDetId = -1;

     int moduleId = -1;
     int prevmodId = -1;

     int flag = 0;

     while ((key=(TKey*)next()) && flag == 0) {

        std::string name(key->GetName());
        std::vector<std::string> tokens;

        int trueOrNot = -1;
        int module = -1;
        std::vector<Int_t> hit;


        tokens = split(name,'_');

        thisCluster = (TH2D*)key->ReadObj();

        //std::cout<<"Histo name = "<<thisCluster->GetName()<<std::endl;
        for (size_t i = 0; i < tokens.size(); i++) {
            // std::cout<<"Token "<<i<<" : "<<tokens[i]<<std::endl;
        }


        hitnumber = tokens[0];
        hitnumber += tokens[1];
        hitnumber += tokens[2];
        hitnumber += tokens[3];
        hitnumber += tokens[4];

        thisHitCounter = atoi(hitnumber.data());


        detector = tokens[5];
        detector += tokens[6];
        detector += tokens[7];

        trueOrNot = atoi(&tokens[9][0]);

        moduleId = atoi(&tokens[8][0]);
        //std::cout<<detector<<std::endl;

        if(detector == "B00") detId = 0;
        if(detector == "B01") detId = 1;
        if(detector == "B02") detId = 2;
        if(detector == "BO3") detId = 3;
        if(detector == "F10") detId = 4;
        if(detector == "F11") detId = 5;
        if(detector == "F12") detId = 6;
        if(detector == "F20") detId = 7;
        if(detector == "F21") detId = 8;
        if(detector == "F22") detId = 9;



        debug(__LINE__);
        if(prevHitCounter == thisHitCounter - 1 && detId!=prevDetId){

          int modCounter = 0;

          std::pair<int,int> dets(prevDetId,detId);
          std::pair<int,int> mods(prevmodId,moduleId);

          // std::cout<<"Dets : "<<prevDetId<<" - "<<detId<<std::endl;
          // std::cout<<"Mods : "<<prevmodId<<" - "<<moduleId<<std::endl;

          std::pair<std::pair<int,int>,std::pair<int,int>> modulesIds (dets,mods);

          ios_base::openmode mode;

          itModules = clustersmodulescountertrain.find(modulesIds);
          if (itModules != clustersmodulescountertrain.end() || itModules->second==0)
          {
            modCounter = itModules->second;
            mode = ios_base::app;
            // std::cout<<"Counter : not zero ... "<<modCounter<<std::endl;
          }
          else
          {
            modCounter = 0;
            mode = ios_base::out;
            clustersmodulescountertrain[modulesIds]=modCounter;
            // std::cout<<"Counter : ZERO!"<<std::endl;
          }



            //clustersmodules.erase (it);
          debug(__LINE__);
          buffer = "./txts/modules/dets_" + std::to_string(prevDetId) + "_" + std::to_string(detId) + "_mods_" + std::to_string(prevmodId) + "_" + std::to_string(moduleId) + ".txt";
          std::ofstream moduleCluster(buffer.data(),mode);

          buffer = "./txts/:/dets_" + std::to_string(prevDetId) + "_" + std::to_string(detId) + "_mods_" + std::to_string(prevmodId) + "_" + std::to_string(moduleId) + "labels.txt";
          std::ofstream moduleLabels(buffer.data(),mode);
          //doublet.push_back((uint16_t)trueOrNot);

          int nBinsX = prevCluster->GetNbinsX();
          int nBinsY = prevCluster->GetNbinsY();

          if(globalDoubCounter==0) clusterlabels<<(uint16_t)trueOrNot;
          if(globalDoubCounter!=0) clusterlabels<<" "<<(uint16_t)trueOrNot;

          if(detsGlobalDoubCounter[prevDetId][detId]==0) clusterslabels[prevDetId][detId]<<(uint16_t)trueOrNot;
          if(detsGlobalDoubCounter[prevDetId][detId]!=0) clusterslabels[prevDetId][detId]<<" "<<(uint16_t)trueOrNot;
          if(modCounter == 0) moduleLabels<<(uint16_t)trueOrNot;
          if(modCounter != 0) moduleLabels<<" "<<(uint16_t)trueOrNot;

          // if(trueOrNot==1.0 && trueCounter>limit*0.5) continue;
          // if(trueOrNot==0.0 && fakeCounter>limit*0.5) continue;

          size_t start = 1;
          size_t end = 0;
          debug(__LINE__);
          if(shrink<nBinsX && nBinsX==nBinsY && (nBinsX-shrink)%2==0)
          {
            start = (size_t)((nBinsX-shrink)/2)+1;
            end = (size_t)((nBinsX-shrink)/2);

          }

          for (size_t i = start; i <= nBinsX-end; ++i) {
            for (size_t j = start; j <= nBinsY-end; ++j) {

              //doublet.push_back((uint16_t)prevCluster->GetBinContent(i,j));
              if(detsGlobalDoubCounter[prevDetId][detId]!=0 || (i!=start || j!=start))
                clusters[prevDetId][detId]<<" "<<prevCluster->GetBinContent(i,j)/LEVELS;
              else
                clusters[prevDetId][detId]<<prevCluster->GetBinContent(i,j)/LEVELS;

            }
          }
          debug(__LINE__);
          for (size_t i = start; i <= nBinsX-end; ++i) {
            for (size_t j = start; j <= nBinsY-end; ++j) {

              //doublet.push_back((uint16_t)thisCluster->GetBinContent(i,j));
              clusters[prevDetId][detId]<<" "<<thisCluster->GetBinContent(i,j)/LEVELS;

            }
          }

          for (size_t i = start; i <= nBinsX-end; ++i) {
            for (size_t j = start; j <= nBinsY-end; ++j) {

              //doublet.push_back((uint16_t)prevCluster->GetBinContent(i,j));
              if(globalDoubCounter!=0 || (i!=start || j!=start))
                cluster<<" "<<prevCluster->GetBinContent(i,j)/LEVELS;
              else
                cluster<<prevCluster->GetBinContent(i,j)/LEVELS;

            }
          }
          debug(__LINE__);
          for (size_t i = start; i <= nBinsX-end; ++i) {
            for (size_t j = start; j <= nBinsY-end; ++j) {

              //doublet.push_back((uint16_t)thisCluster->GetBinContent(i,j));
              cluster<<" "<<thisCluster->GetBinContent(i,j)/LEVELS;

            }
          }

          debug(__LINE__);
          for (size_t i = start; i <= nBinsX-end; ++i) {
            for (size_t j = start; j <= nBinsY-end; ++j) {

              //doublet.push_back((uint16_t)prevCluster->GetBinContent(i,j));
              if(modCounter!=0 || (i!=start || j!=start))
                moduleCluster<<" "<<prevCluster->GetBinContent(i,j)/LEVELS;
              else
                moduleCluster<<prevCluster->GetBinContent(i,j)/LEVELS;

            }
          }

          for (size_t i = start; i <= nBinsX-end; ++i) {
            for (size_t j = start; j <= nBinsY-end; ++j) {

              //doublet.push_back((uint16_t)thisCluster->GetBinContent(i,j));
              moduleCluster<<" "<<thisCluster->GetBinContent(i,j)/LEVELS;

            }
          }
          debug(__LINE__);
          ++detsGlobalDoubCounter[prevDetId][detId];
          ++globalDoubCounter;
          ++doubcounter;
          ++modCounter;

          itModules->second = modCounter;

          clustersmodulescounter[modulesIds]=modCounter;

          if(globalDoubCounter == 1)
          {

            thisCluster->Draw("COLZ2TEXT");
            canvas->SaveAs("secondTest.png");
            canvas->Clear();

            prevCluster->Draw("COLZ2TEXT");
            canvas->SaveAs("firstTest.png");
            canvas->Clear();

          }

          // std::cout<<"Prev Id :"<<prevDetId<<" Det Id : "<<detId<<std::endl;

          //if(counter>5) return;
          debug(__LINE__);
          if(trueOrNot==1.) ++trueCounter[prevDetId][detId];
          if(trueOrNot==0.) ++fakeCounter[prevDetId][detId];

          if(trueOrNot==1.) ++trueCount;
          if(trueOrNot==0.) ++fakeCount;
          debug(__LINE__);
          if(trueOrNot == 0) ++fakecounter;

        }

        debug(__LINE__);
        prevHitCounter = thisHitCounter;
        prevCluster = thisCluster;
        prevDetId = detId;
        prevmodId = moduleId;

        if(globalDoubCounter >= limit)
        {
          flag = -1;
          std::cout<<"Limit hit!! : "<<limit<<std::endl;
        }

        ++counter;
        ++globalCounter;

        debug(__LINE__);
      }

      std::cout<<std::endl<<"On "<<counter<<" hits I found "<<doubcounter<<" viable doubltes ("<<fakecounter<<" fake ) "<<std::endl;//each of size : "<<shrink<<". "<<std::endl;
      debug(__LINE__);
      inputFile->Close();
      if((int)i>noFiles-1) i=fileNames.size();
        }//files for
        debug(__LINE__);
        for (int i = 0; i < 10; ++i)
          for (int j = 0; j < 10; ++j)
          {
            int tot = (int)((float)fakeCounter[i][j]/0.4);

            trueCounterLimit[i][j] = (int)(((float)tot)*0.6);
            fakeCounterLimit[i][j] = fakeCounter[i][j];

            std::cout<<i<<" - "<<j<<" ; fakesL = "<<fakeCounter[i][j]<<" - trues = "<<trueCounterLimit[i][j]<<std::endl;

          }
        debug(__LINE__);

        fakeCountLimit = fakeCount;
        trueCountLimit = (int)(((float)fakeCount)*1.5);

        std::cout<<" = Testing and training"<<std::endl;

        globalDoubCounter = 0;
        globalCounter = 0;

        int flag = 0;

        for (size_t i = 0; i < fileNames.size(); i++) {
          debug(__LINE__);
          std::cout<<"=================================================="<<std::endl;
          std::cout<<"Reading File - "<<fileNames[i]<<" no. "<<i<<" on "<<fileNames.size()<<std::endl;

          TFile *inputFile = TFile::Open(fileNames[i].data());

          if(!(inputFile)) {
            std::cout<<"Invalid filename "<<fileNames[i]<<" - skipping "<<std::endl;
            continue;
          }else{
            std::cout<<"With "<<inputFile->GetNkeys()<<" keys "<<std::endl;
          }
          //std::vector< std::vector<uint16_t> > doublets;

          // trueOrNot | det1 | det2 | x10y10 x11y10 x12y10 .... x1ny1n | x20 y20 ... x2n y2n |
          debug(__LINE__);
          TIter next(inputFile->GetListOfKeys());
          TKey *key;


          int counter = 0;
          int doubcounter = 0;
          int clusterPacked = 0;
          int fakecounter = 0;

          int detId = -1;
          int prevDetId = -1;

          int moduleId = -1;
          int prevmodId = -1;



          while ((key=(TKey*)next()) && flag == 0) {
            debug(__LINE__);
             std::string name(key->GetName());
             std::vector<std::string> tokens;

             int trueOrNot = -1;
             int module = -1;
             std::vector<Int_t> hit;


             tokens = split(name,'_');

             thisCluster = (TH2D*)key->ReadObj();

             //std::cout<<"Histo name = "<<thisCluster->GetName()<<std::endl;
             for (size_t i = 0; i < tokens.size(); i++) {
                 // std::cout<<"Token "<<i<<" : "<<tokens[i]<<std::endl;
             }


             hitnumber = tokens[0];
             hitnumber += tokens[1];
             hitnumber += tokens[2];
             hitnumber += tokens[3];
             hitnumber += tokens[4];

             thisHitCounter = atoi(hitnumber.data());

             debug(__LINE__);
             detector = tokens[5];
             detector += tokens[6];
             detector += tokens[7];

             trueOrNot = atoi(&tokens[9][0]);

             moduleId = atoi(&tokens[8][0]);
             //std::cout<<detector<<std::endl;

             if(detector == "B00") detId = 0;
             if(detector == "B01") detId = 1;
             if(detector == "B02") detId = 2;
             if(detector == "BO3") detId = 3;
             if(detector == "F10") detId = 4;
             if(detector == "F11") detId = 5;
             if(detector == "F12") detId = 6;
             if(detector == "F20") detId = 7;
             if(detector == "F21") detId = 8;
             if(detector == "F22") detId = 9;

             debug(__LINE__);
             if(prevHitCounter == thisHitCounter - 1 && detId!=prevDetId){
               debug(__LINE__);
               int modCounter = 0;

               std::pair<int,int> dets(prevDetId,detId);
               std::pair<int,int> mods(prevmodId,moduleId);

               std::pair<std::pair<int,int>,std::pair<int,int>> modulesIds (dets,mods);

               ios_base::openmode mode;

               itModules = clustersmodulescounter.find(modulesIds);
               if (itModules != clustersmodulescounter.end() || itModules->second==0)
               {
                 modCounter = itModules->second;
                 mode = ios_base::app;
               }
               else
               {
                 modCounter = 0;
                 mode = ios_base::out;
                 clustersmodulescounter[modulesIds]=modCounter;
               }
                 //clustersmodules.erase (it);
                 debug(__LINE__);
               if(test)
                  buffer = "./txts/modules/dets_test_";
               else
                  buffer = "./txts/modules/dets_train_";

               buffer += std::to_string(prevDetId) + "_" + std::to_string(detId) + "_mods_" + std::to_string(prevmodId) + "_" + std::to_string(moduleId) + ".txt";
               std::ofstream moduleClusterTrain(buffer.data(),mode);

               if(test)
                  buffer = "./txts/modules/dets_test_";
               else
                  buffer = "./txts/modules/dets_train_";
                  debug(__LINE__);
               buffer += std::to_string(prevDetId) + "_" + std::to_string(detId) + "_mods_" + std::to_string(prevmodId) + "_" + std::to_string(moduleId) + "labels.txt";
               std::ofstream moduleLabelsTrain(buffer.data(),mode);
               //doublet.push_back((uint16_t)trueOrNot);

               int nBinsX = prevCluster->GetNbinsX();
               int nBinsY = prevCluster->GetNbinsY();

               bool TrueCounter = (trueCounterTrain[prevDetId][detId]<trueCounterLimit[prevDetId][detId]);
               bool FakeCounter = (fakeCounterTrain[prevDetId][detId]<fakeCounterLimit[prevDetId][detId]);

               bool TrueCounterTot = (trueCountTrain<trueCountLimit);
               bool FakeCounterTot = (fakeCountTrain<fakeCountLimit);

               if((TrueCounter && trueOrNot==1.) || (FakeCounter && trueOrNot==0.))
               {

                 if(detsGlobalDoubCounterTrain[prevDetId][detId]==0) clusterslabelstrain[prevDetId][detId]<<(uint16_t)trueOrNot;
                 if(detsGlobalDoubCounterTrain[prevDetId][detId]!=0) clusterslabelstrain[prevDetId][detId]<<" "<<(uint16_t)trueOrNot;
                 //if(modCounter == 0) moduleLabelsTrain<<(uint16_t)trueOrNot;
                 //if(modCounter != 0) moduleLabelsTrain<<" "<<(uint16_t)trueOrNot;

                 // if(trueOrNot==1.0 && trueCounter>limit*0.5) continue;
                 // if(trueOrNot==0.0 && fakeCounter>limit*0.5) continue;

                 size_t start = 1;
                 size_t end = 0;

                 if(shrink<nBinsX && nBinsX==nBinsY && (nBinsX-shrink)%2==0)
                 {
                   start = (size_t)((nBinsX-shrink)/2)+1;
                   end = (size_t)((nBinsX-shrink)/2);

                 }

                 for (size_t i = start; i <= nBinsX-end; ++i) {
                   for (size_t j = start; j <= nBinsY-end; ++j) {

                     //doublet.push_back((uint16_t)prevCluster->GetBinContent(i,j));
                     if(detsGlobalDoubCounterTrain[prevDetId][detId]!=0 || (i!=start || j!=start))
                       clusterstrain[prevDetId][detId]<<" "<<prevCluster->GetBinContent(i,j)/LEVELS;
                     else
                       clusterstrain[prevDetId][detId]<<prevCluster->GetBinContent(i,j)/LEVELS;

                   }
                 }
                 debug(__LINE__);
                 for (size_t i = start; i <= nBinsX-end; ++i) {
                   for (size_t j = start; j <= nBinsY-end; ++j) {

                     //doublet.push_back((uint16_t)thisCluster->GetBinContent(i,j));
                     clusterstrain[prevDetId][detId]<<" "<<thisCluster->GetBinContent(i,j)/LEVELS;

                   }
                 }


                 ++detsGlobalDoubCounterTrain[prevDetId][detId];

                 ++doubcounter;
                 ++modCounter;

                 itModules->second = modCounter;
                 clustersmodulescounter[modulesIds]=modCounter;
                 debug(__LINE__);
                 // std::cout<<"Prev Id :"<<prevDetId<<" Det Id : "<<detId<<std::endl;

                 //if(counter>5) return;

                 if(trueOrNot==1.) ++trueCounterTrain[prevDetId][detId];
                 if(trueOrNot==0.) ++fakeCounterTrain[prevDetId][detId];

                 if(trueOrNot == 0) ++fakecounter;
                 debug(__LINE__);
               }

               if((TrueCounterTot && trueOrNot==1.) || (FakeCounterTot && trueOrNot==0.))
               {
                 if(globalDoubCounter==0) clusterlabelstrain<<(uint16_t)trueOrNot;
                 if(globalDoubCounter!=0) clusterlabelstrain<<" "<<(uint16_t)trueOrNot;

                 size_t start = 1;
                 size_t end = 0;

                 if(shrink<nBinsX && nBinsX==nBinsY && (nBinsX-shrink)%2==0)
                 {
                   start = (size_t)((nBinsX-shrink)/2)+1;
                   end = (size_t)((nBinsX-shrink)/2);

                 }

                 for (size_t i = start; i <= nBinsX-end; ++i) {
                   for (size_t j = start; j <= nBinsY-end; ++j) {

                     //doublet.push_back((uint16_t)prevCluster->GetBinContent(i,j));
                     if(globalDoubCounter!=0 || (i!=start || j!=start))
                       clustertrain<<" "<<prevCluster->GetBinContent(i,j)/LEVELS;
                     else
                       clustertrain<<prevCluster->GetBinContent(i,j)/LEVELS;

                   }
                 }
                 debug(__LINE__);
                 for (size_t i = start; i <= nBinsX-end; ++i) {
                   for (size_t j = start; j <= nBinsY-end; ++j) {

                     //doublet.push_back((uint16_t)thisCluster->GetBinContent(i,j));
                     clustertrain<<" "<<thisCluster->GetBinContent(i,j)/LEVELS;

                   }
                 }

                 if(trueOrNot==1.) ++trueCountTrain;
                 if(trueOrNot==0.) ++fakeCountTrain;

                 debug(__LINE__);
               }

             }

             debug(__LINE__);
             prevHitCounter = thisHitCounter;
             prevCluster = thisCluster;
             prevDetId = detId;
             prevmodId = moduleId;
             debug(__LINE__);
             if(globalDoubCounter >= limit)
             {
               flag = -1;
               std::cout<<"Limit hit!! : "<<limit<<std::endl;
             }

             ++counter;
             ++globalCounter;

             debug(__LINE__);
           }

           debug(__LINE__);
           std::cout<<std::endl<<"On "<<counter<<" hits I found "<<doubcounter<<" viable doubltes ("<<fakecounter<<" fake ) "<<std::endl;//each of size : "<<shrink<<". "<<std::endl;
           if((int)i>noFiles-1) i=fileNames.size();
             }//files for
             debug(__LINE__);
        std::cout<<"Fakes & trues : "<<std::endl;
        for (int i = 0; i < 10; ++i)
          for (int j = 0; j < 10; ++j)
            if(fakeCounter[i][j]!=0 || trueCounter[i][j]!=0)
              std::cout<<i<<" - "<<j<<" ; fakes = "<<fakeCounterTrain[i][j]<<" - trues = "<<trueCounterTrain[i][j]<<std::endl;

          debug(__LINE__);
   }
