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

void reshape(int formersize = 14, int newsize = 8, bool test = false)
{
   //TCanvas *canvas = new TCanvas("canvas","canvas",1000,1000);
   TFile *inputFile = TFile::Open(clusterpath);

   std::string buffer;

   std::ifstream clusters;

   if(!test){
     clusters.open("clusterstrain.txt", std::ofstream::in);
   }else{

     clusters.open("clusterstest.txt", std::ofstream::in);

   }

   if (dataTxt.good()) {
     Int_t evt=0;
     cout <<"\n- Reading " <<events <<" out of " <<totEvents <<" events from " <<datasetName <<" and filling variables histograms" <<endl;
     dataTxt.clear(); dataTxt.seekg (0, ios::beg);
     while( (evt < events)  &&  (dataTxt >> var1 >> var2 >> var3 >> var4) ) {
       evt++;
       massKPi->value = var1;
       massPsiPi->value = var2;
       cosMuMu->value = var3;
       phi->value = var4;

       //std::cout << massKPi->value << " - " <<cosMuMu->value << " - " << massPsiPi->value << " - " << phi->value << " - " << std::endl;
       if (Dalitz_contour_host(massKPi->value, massPsiPi->value, kFALSE, (Int_t)psi_nS->value) ) {
           	dataset.addEvent();
             massesDataset.addEvent();
           	massKPiHisto.Fill(massKPi->value);
           	cosMuMuHisto.Fill(cosMuMu->value);
           	massPsiPiHisto.Fill(massPsiPi->value);
           	phiHisto.Fill(phi->value);
       }

       dataTxt.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
     }
   }
   dataTxt.close();

   std::vector< std::vector<uint16_t> > doublets;

   // trueOrNot | det1 | det2 | x10y10 x11y10 x12y10 .... x1ny1n | x20 y20 ... x2n y2n |

   TIter next(inputFile->GetListOfKeys());
   TKey *key;

   TH2D * thisCluster = 0;
   TH2D * prevCluster = 0;

   int thisHitCounter = -1;
   int nextHitCounter = -1;
   int prevHitCounter = -1;

   int counter = 0;
   int doubcounter = 0;
   int fakecounter = 0;

   int detId = -1;
   int prevDetId = -1;

   double fraction = 0.0;

   std::cout<<"Reading ";

   int flag = 0;

   while ((key=(TKey*)next()) && flag == 0) {

      //if(counter%1000==0) std::cout<<" . ";

      std::vector<uint16_t> doublet;

      // std::cout<<"Key "<<key->GetName()<<"points to an object of class "<<  key->GetClassName()<<" at "<<key->GetSeekKey()<<"n"<<std::endl;

      std::string name(key->GetName());
      std::vector<std::string> tokens;

      int trueOrNot = -1;
      std::vector<Int_t> hit;


      tokens = split(name,'_');

      thisCluster = (TH2D*)key->ReadObj();
      //std::cout<<"Histo name = "<<thisCluster->GetName()<<std::endl;
      for (size_t i = 0; i < tokens.size(); i++) {
        //  std::cout<<"Token "<<i<<" : "<<tokens[i]<<std::endl;
      }

      std::string detector;
      std::string hitnumber;

      hitnumber = tokens[0];
      hitnumber += tokens[1];
      hitnumber += tokens[2];
      hitnumber += tokens[3];
      hitnumber += tokens[4];

      thisHitCounter = atoi(hitnumber.data());

      if(tokens[5]=="B")
      {
        // std::cout<<"Barrel"<<std::endl;
        detector = tokens[5];
        detector += tokens[7];

        trueOrNot = atoi(&tokens[8][0]);
        // std::cout<<" detector "<<detector<<std::endl;
      }
      else
      {

        // std::cout<<"Forward "<<std::endl;
        detector = tokens[5];
        detector += tokens[6];


        trueOrNot = atoi(&tokens[7][0]);
        // std::cout<<" detector "<<detector<<std::endl;
      }

      if(detector == "B0") detId = 0;
      if(detector == "B1") detId = 1;
      if(detector == "B2") detId = 2;
      if(detector == "B3") detId = 3;
      if(detector == "F10") detId = 4;
      if(detector == "F11") detId = 5;
      if(detector == "F12") detId = 6;
      if(detector == "F20") detId = 7;
      if(detector == "F21") detId = 8;
      if(detector == "F22") detId = 9;




      if(prevHitCounter == thisHitCounter - 1 && detId!=prevDetId){

         if(doubcounter!=0){
           clusterslabels<<" ";
           //clusters<<" ";
         }

        doublet.push_back((uint16_t)trueOrNot);

        //clusterslabels<<abs((uint16_t)trueOrNot-1)<<" "<<(uint16_t)trueOrNot;
        clusterslabels<<(uint16_t)trueOrNot;
        //detectors
        doublet.push_back((uint16_t)prevDetId);
        doublet.push_back((uint16_t)detId);

        for (size_t i = 1; i <= prevCluster->GetNbinsX(); ++i) {
          for (size_t j = 1; j <= prevCluster->GetNbinsY(); ++j) {

            doublet.push_back((uint16_t)prevCluster->GetBinContent(i,j));
            if(doubcounter!=0 || (i!=1 || j!=1))
            clusters<<" "<<(uint16_t)prevCluster->GetBinContent(i,j)/LEVELS;
            else
            clusters<<(uint16_t)prevCluster->GetBinContent(i,j)/LEVELS;

          }
        }

        for (size_t i = 1; i <= thisCluster->GetNbinsX(); ++i) {
          for (size_t j = 1; j <= thisCluster->GetNbinsY(); ++j) {

            doublet.push_back((uint16_t)thisCluster->GetBinContent(i,j));

            clusters<<" "<<(uint16_t)thisCluster->GetBinContent(i,j)/LEVELS;

          }
        }

        doublets.push_back(doublet);
        ++doubcounter;
        if(trueOrNot == 0) ++fakecounter;

      }

      prevHitCounter = thisHitCounter;
      prevCluster = thisCluster;
      prevDetId = detId;

      if(doubcounter >= limit) flag = -1;

      ++counter;
    }


    std::cout<<std::endl<<"On "<<counter<<" hits I found "<<doubcounter<<" viable doubltes ("<<fakecounter<<" fake ) each of size : "<<doublets[0].size()<<". "<<std::endl;

   }
