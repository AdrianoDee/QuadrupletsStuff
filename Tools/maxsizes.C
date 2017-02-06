#include <iostream>
#include <fstream>
#include <algorithm>
#include "TGraph.h"
#include "TGraph.h"


void maxsizes()
{

  std::ifstream files[7];

  files[0].open("./barel0.txt", std::ifstream::in);
  files[1].open("./barel1.txt", std::ifstream::in);
  files[2].open("./barel2.txt", std::ifstream::in);
  files[3].open("./barel3.txt", std::ifstream::in);
  files[4].open("./forward0.txt", std::ifstream::in);
  files[5].open("./forward1.txt", std::ifstream::in);
  files[6].open("./forward2.txt", std::ifstream::in);


  double X = 0, Y = 0;
  double maxX = 0, maxY = 0;
  double minX = 0, minY = 0;

  for (size_t i = 0; i < 7; i++) {
    if(files[i].good()){
      while(!files[i].eof()){

        files[i] >>X>>Y;

        maxX = std::max(X,maxX);
        maxY = std::max(Y,maxY);

        minX = -std::max(-X,-maxX);
        minY = -std::max(-Y,-maxY);

      }
    }

    std::cout<<"Max X : "<<maxX<<" Max Y : "<<maxY<<" for "<<i<<" layer"<<std::endl;
    std::cout<<"Min X : "<<minX<<" Min Y : "<<minY<<" for "<<i<<" layer"<<std::endl;

  }

}
