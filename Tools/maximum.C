#include <iostream>
#include <fstream>
#include <algorithm>
#include "TGraph.h"
#include "TGraph.h"


void maximum(int pad)
{

  std::ifstream sizeX("./sizesX.txt");
  std::ifstream sizeY("./sizesY.txt");

  int maxX = 0;
  int maxY = 0;

  double sumX = 0.0;
  double sumY = 0.0;
  double diffsX = 0.0;
  double diffsY = 0.0;

  double meanX = 0.0;
  double meanY = 0.0;

  int counterOverX = 0;
  int counterOverY = 0;
  int counterOver = 0;
  int counter = 0;

  int varX, varY;

  if(sizeX.good() && sizeY.good()){
    while(!sizeX.eof() && !sizeY.eof()){

      sizeX >> varX;
      sizeY >> varY;
      maxX = std::max(varX,maxX);
      sumX +=(double) varX;
      maxY = std::max(varY,maxY);
      sumY +=(double) varY;
      if(varX>pad) ++counterOverX;
      if(varY>pad) ++counterOverY;
      if(varX>pad || varY>pad) ++counterOver;
      ++counter;
    }
  }

  meanX = sumX/(double)counter;
  meanY = sumY/(double)counter;

  sizeX.clear();
  sizeX.seekg(0, ios::beg);
  sizeY.clear();
  sizeY.seekg(0, ios::beg);

  if(sizeX.good()){
    while(!sizeX.eof()){
      int var;
      sizeX >> var;
      diffsX += (meanX-(double)var)*(meanX-(double)var);
    }
  }

  if(sizeY.good()){
    while(!sizeY.eof()){
      int var;
      sizeY >> var;
      maxY = std::max(var,maxY);
      diffsY += (meanY-(double)var)*(meanY-(double)var);
    }
  }

  diffsY /= (double)(counter - 1);
  diffsX /= (double)(counter - 1);

  sizeX.clear();
  sizeX.seekg(0, ios::beg);
  sizeY.clear();
  sizeY.seekg(0, ios::beg);


  for (int pad = 7; pad < 66; ++pad) {

  if(sizeX.good() && sizeY.good()){
    while(!sizeX.eof() && !sizeY.eof()){

      sizeX >> varX;
      sizeY >> varY;
      maxX = std::max(varX,maxX);
      sumX +=(double) varX;
      maxY = std::max(varY,maxY);
      sumY +=(double) varY;
      if(varX>pad) ++counterOverX;
      if(varY>pad) ++counterOverY;
      if(varX>pad || varY>pad) ++counterOver;
      ++counter;
    }
  }
  /* code */
  }




std::cout<<"Max X : "<<maxX<<" Max Y : "<<maxY<<" on "<<counter<<" clusters"<<std::endl;
std::cout<<"Avg X : "<<meanX<<" Var X : "<<diffsX<<"Avg Y : "<<meanY<<" Var Y : "<<diffsY<<std::endl;
std::cout<<"Over 16 X : "<<counterOverX<<" Over 16 Y : "<<counterOverY<<" Over YorX"<<counterOver<<std::endl;

}
