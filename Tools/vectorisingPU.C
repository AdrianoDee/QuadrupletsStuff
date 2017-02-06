#include <iostream>
#include <fstream>
#include <algorithm>
#include <map>

#include <string>
#include <sstream>

#include <iterator>

#include "TFile.h"
#include "TCanvas.h"
#include "TH2D.h"
#include "TKey.h"
#include "TCollection.h"

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

void vectorise(std::string path = "",long int noFiles = 20000000, bool test = false, long int limit = 10E10, int shrink = 8)
{

   int hitId,hitIdKey,phiHitId,pdgId,buf;
   float check;

    ////////////////////////////////////////////////////////
    //Getting File List

   std::string files = "ls " + path + "/clusters/" + "*";
   std::string ls = GetStdoutFromCommand(files);
   std::vector<std::string> fileNames = split(ls,'\n');
   std::vector<std::string> hitInfos;
   std::vector<std::string> evInfos;

  //  for (size_t i = 0; i < fileNames.size(); i++) {
   //
   //
  //    hitInfos = split(fileNames[i],'_');
  //   //  std::cout<<"Evt : "<< atoi((hitInfos[0].data())) <<" Det : "<< atoi((hitInfos[1].data())) <<"Filename : "<<fileNames[i]<<std::endl;
   //
  //  }

   ////////////////////////////////////////////////////////
   //Getting Key List

   files = "ls " + path + "/keys/" + "*";
   ls = GetStdoutFromCommand(files);

   std::vector<std::string> keyNames = split(ls,'\n');

  //  for (size_t i = 0; i < keyNames.size(); i++) {
   //
  //    hitInfos = split(keyNames[i],'_');
  //   //  std::cout<<"Evt : "<< atoi((hitInfos[0].data())) <<" Det : "<< atoi((hitInfos[1].data())) <<"Filename : "<<keyNames[i]<<std::endl;
   //
  //  }

  ////////////////////////////////////////////////////////
  //Getting PdgIds List

  files = "ls " + path + "/pdgIds/"+ "*";
  ls = GetStdoutFromCommand(files);
  std::vector<std::string> pdgIdsNames = split(ls,'\n');
  std::map< std::pair<int,int> , std::map <int,std::pair <int,float > > > pdgMap;

  for (size_t i = 0; i < pdgIdsNames.size(); i++) {

    hitInfos = split(pdgIdsNames[i],'_');

    evInfos = split(hitInfos[0],'/');

    std::pair<int,int> evDet(atoi((evInfos[2].data())),atoi((hitInfos[1].data())));

    ifstream pdgTxt(pdgIdsNames[i]);

    if (!(pdgTxt.good())) continue;
    else
    {
      std:map< int, std::pair <int,float >> singleMap;
      pdgTxt.clear(); pdgTxt.seekg (0, ios::beg);

      // std::cout<<"Evt : "<< atoi((hitInfos[0].data())) <<"Det : "<< atoi((hitInfos[1].data())) <<"Filename : "<<pdgIdsNames[i]<<std::endl;


      while( (pdgTxt >> buf >> hitIdKey >> pdgId >> check) )
      {
        std::pair <int,float > idCheck(pdgId,check);
        singleMap[hitIdKey] = idCheck;

        // std::cout << buf << " " << hitIdKey << " " << pdgId << " " << check <<std::endl;

        pdgTxt.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        pdgTxt.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      }

      pdgMap[evDet] = singleMap;

    }



  }



  //  std::vector< std::map <int,int> > keyPhiMaps;
  //  std::vector< std::map <int,int> > pdgsIdMaps;

   //VectorPerEvents[Map[detector,hitIdInPhi]] =  pdgId

   std::map< int, std::map <std::pair<int,int>,std::pair<int,float>> > theMap;

   if(noFiles>keyNames.size()) noFiles=(long int)keyNames.size();

   for (size_t i = 0; i < noFiles; ++i)
   {
     hitInfos = split(pdgIdsNames[i],'_');

   }

   for (size_t i = 0; i < noFiles; ++i)
   {
     std::cout<<"=================================================="<<std::endl;
     std::cout<<"Reading Keys - "<<keyNames[i]<<" no. "<<i<<std::endl;
    //  std::cout<<"Reading Pdgs - "<<pdgIdsNames[i]<<" no. "<<i<<std::endl;

     ifstream keyTxt(keyNames[i]);

     hitInfos = split(keyNames[i],'_');
     evInfos = split(hitInfos[0],'/');

     int event = atoi((evInfos[2].data()));
     int detId = atoi((hitInfos[1].data()));
     std::pair<int,int> evtDet(event,detId);

    //  for (size_t i = 0; i < hitInfos.size(); i++) {
    //      std::cout<<"Token "<<i<<" : "<<hitInfos[i]<<std::endl;
    //  }

     std::cout<<"Event "<<event<<" Det "<<detId<<std::endl;


     if ( !(keyTxt.good()) )
     {
       std::cout <<"No valid input provided.\nReturning." <<std::endl;
       return ;
     }
     else
     {
       std::map< std::pair<int,int> , std::map <int,std::pair <int,float > > >::iterator it1;
       it1 = pdgMap.find(evtDet);

       if (it1 != pdgMap.end())
       {
         std::cout<<it1->first.first<<" - "<<it1->first.second<<std::endl;
         keyTxt.clear(); keyTxt.seekg (0, ios::beg);
         std::map <int,std::pair<int,float> > phiMap;
         std::map < std::pair<int,int>, std::pair<int,float> > aMap;

          while( (keyTxt >> hitId >> phiHitId >> check ) )
          {
            // std::cout<< hitId << " " << phiHitId << " " << check <<std::endl;

            std::pair<int,float> checkPhi(phiHitId,check);

            phiMap[hitId]    = checkPhi;

            keyTxt.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            // keyTxt.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

          }

          for (std::map<int,std::pair<int,float> >::iterator it2=phiMap.begin(); it2!=phiMap.end(); ++it2)
          {
            std::map <int,std::pair <int,float > >::iterator it3;
            it3 = it1->second.find(it2->first);

            // std::cout<<"Phi map element : "<<std::endl;
            // std::cout<<(it2->first) << " " << (it2->second).first << " " << (it2->second).second<<std::endl;

            if (it3 != it1->second.end())
            {
              // std::cout<<"Pdg map element : "<<std::endl;
              // std::cout<<it3->first << " " << (it3->second).first << " " << (it3->second).second<<std::endl;

              if((it3->second).second == (it2->second).second)
              {
                // std::cout<<detId << " " << event << " " << it2->first << " " << (it3->second).first<<std::endl;

                std::pair<int,int> keyDetHit(detId,it2->first);
                std::pair<int,float> idCheck((it3->second).first,(it3->second).second);
                aMap[keyDetHit]  = idCheck;
            }
          }
          }

          theMap[event] = aMap;
       }



      }


     }

    //  int ev = 0, de = 0, hit = 100;
    //  std::pair<int,int> testPair(de,hit);
     //
    //  std::cout<<(theMap[ev])[testPair]<<std::endl;
     //
    //  std::cout<<" Insert event number : ";
     //
    //  while((cin >> ev ))
    //  {
    //    std::cout<<" Insert detector number : ";
    //    cin >> de;
     //
    //    std::map <std::pair<int,int>,int> evMap = theMap[ev];
     //
    //    for (std::map <std::pair<int,int>,int>::iterator it=evMap.begin(); it!=evMap.end(); ++it)
    //    {
    //      std::cout<<"Event "<< ev <<" Det : "<< ((it->first).first) <<" Hit : "<< ((it->first).second) << " Id : " << (it->second) <<std::endl;
    //    }
     //
     //
    //  }

    std::cout<<"Map phi sorted phi and pdgIds created "<<std::endl;

    for (size_t i = 0; i < fileNames.size(); i++)
    {
      td::cout<<"=================================================="<<std::endl;
      std::cout<<"Reading Clusters - "<<fileNames[i]<<" no. "<<i<<std::endl;

      ifstream clusterTxt(fileNames[i]);

      hitInfos = split(keyNames[i],'_');
      evInfos = split(hitInfos[0],'/');

      int event = atoi((evInfos[2].data()));
      int detId = atoi((hitInfos[1].data()));
      std::pair<int,int> evtDet(event,detId);

      std::cout<<"Event "<<event<<" Det "<<detId<<std::endl;


    }


   }
