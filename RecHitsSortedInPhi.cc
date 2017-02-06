#include "RecoTracker/TkHitPairs/interface/RecHitsSortedInPhi.h"
#include "DataFormats/TrackerRecHit2D/interface/BaseTrackerRecHit.h"
#include "DataFormats/Provenance/interface/EventID.h"

#include <algorithm>
#include <cassert>
#include <utility>
#include <thread>
#include <iostream>
#include <string>
#include <fstream>


bool pairCompareHitsPhi( const std::pair<int,RecHitsSortedInPhi::HitWithPhi>& firstEl, const std::pair<int,RecHitsSortedInPhi::HitWithPhi>& secondEl) { return firstEl.second.phi() < secondEl.second.phi(); }


RecHitsSortedInPhi::RecHitsSortedInPhi(const std::vector<Hit>& hits, GlobalPoint const & origin, DetLayer const * il, edm::EventID eId) :
  layer(il),
  isBarrel(il->isBarrel()),
  x(hits.size()),y(hits.size()),z(hits.size()),drphi(hits.size()),
  u(hits.size()),v(hits.size()),du(hits.size()),dv(hits.size()),
  lphi(hits.size())
{

  int detOnArr[10] = {0,1,2,3,14,15,16,29,30,31};
  std::vector<int> detOn(detOnArr,detOnArr+sizeof(detOnArr)/sizeof(int));
  std::vector<int>::iterator detOnIt;

  // standard region have origin as 0,0,z (not true!!!!0
  // cosmic region never used here
  // assert(origin.x()==0 && origin.y()==0);

  theHits.reserve(hits.size());

  for (auto const & hp : hits) theHits.emplace_back(hp);

  std::sort( theHits.begin(), theHits.end(), HitLessPhi());

  for (unsigned int i=0; i!=theHits.size(); ++i) {

    auto const & h = *theHits[i].hit();
    auto const & gs = static_cast<BaseTrackerRecHit const &>(h).globalState();
    auto loc = gs.position-origin.basicVector();
    float lr = loc.perp();
    // float lr = gs.position.perp();
    float lz = gs.position.z();
    float dr = gs.errorR;
    float dz = gs.errorZ;
    // r[i] = gs.position.perp();
    // phi[i] = gs.position.barePhi();
    x[i] = gs.position.x();
    y[i] = gs.position.y();
    z[i] = lz;
    drphi[i] = gs.errorRPhi;
    u[i] = isBarrel ? lr : lz;
    v[i] = isBarrel ? lz : lr;
    du[i] = isBarrel ? dr : dz;
    dv[i] = isBarrel ? dz : dr;
    lphi[i] = loc.barePhi();
  }

  detOnIt = find(detOn.begin(),detOn.end(),il->seqNum());
  // std::cout<<"keys"<<il->seqNum()<<std::endl;

  if(detOnIt!=detOn.end())
  {
    // std::cout<<"keys"<<std::endl;
    auto threadId = std::this_thread::get_id();
    std::stringstream streamThreadId;
    streamThreadId << threadId;

    size_t detCounter = detOnIt - detOn.begin();

    Int_t eveNumber = eId.event();
    // Int_t runNumber = eId.run();
    // Int_t lumNumber = eId.luminosityBlock();

    std::string bufferstring = "./Hits/keys/";
    bufferstring += std::to_string(eveNumber);
    // bufferstring +="_Event_";
    bufferstring +="_";
    bufferstring += std::to_string(detCounter);
    bufferstring +="_";
    // bufferstring += std::to_string(lumNumber);
    // bufferstring +="_Lumi_";
    // bufferstring += std::to_string(runNumber);
    // bufferstring +="_Run_";
    bufferstring += streamThreadId.str();
    bufferstring += "_keys.txt";

    std::ofstream keyFile(bufferstring, std::ofstream::app);

    //std::cout<<innerLayer.detLayer()->seqNum()<<" "<<innerLayer.name()<<std::endl;

    std::vector<std::pair<int,RecHitsSortedInPhi::HitWithPhi>> hitsKeys;
    hitsKeys.reserve(hits.size());
    int hitCounter = 0;
    for (auto const & hp : hits)
    {
      std::pair<int,RecHitsSortedInPhi::Hit> thisPair(hitCounter,hp);
      ++hitCounter;
      hitsKeys.emplace_back(thisPair);
    }
    std::sort(hitsKeys.begin(), hitsKeys.end(),pairCompareHitsPhi);

    int orederedIndex = 0;

    for (auto const & pair : hitsKeys)
    {
      auto const & h = *pair.second.hit();
      auto const & gs = static_cast<BaseTrackerRecHit const &>(h).globalState();

      keyFile<<pair.first<<"\t\t"<<orederedIndex++<<"\t\t"<<gs.position.x()<<std::endl;
    }
    // for (auto const & pair : hitsKeys)
    // {
    //   std::cout<<
    //
    // }

    keyFile<<std::endl<<std::endl<<std::endl;

    keyFile.close();

  }

  // for (unsigned int i=0; i!=theHits.size(); ++i) {
  //
  //   auto const & h = *theHits[i].hit();
  //   auto const & gs = static_cast<BaseTrackerRecHit const &>(h).globalState();
  //
  //   float x = gs.position.x();
  //   float y = gs.position.y();
  //   float z = gs.position.z();
  //
  //   //auto loc = gs.position-origin.basicVector();
  //
  //   // std::cout<<"RecHit : "<<i<<" x = "<<y<<" y = "<<z<<" z = "<<z<<"phi"<<loc.barePhi()<<std::endl;
  //
  // }

}

RecHitsSortedInPhi::RecHitsSortedInPhi(const std::vector<Hit>& hits, GlobalPoint const & origin, DetLayer const * il) :
  layer(il),
  isBarrel(il->isBarrel()),
  x(hits.size()),y(hits.size()),z(hits.size()),drphi(hits.size()),
  u(hits.size()),v(hits.size()),du(hits.size()),dv(hits.size()),
  lphi(hits.size())
{


  for (unsigned int i=0; i!=theHits.size(); ++i) {

    auto const & h = *theHits[i].hit();
    auto const & gs = static_cast<BaseTrackerRecHit const &>(h).globalState();
    auto loc = gs.position-origin.basicVector();
    float lr = loc.perp();
    // float lr = gs.position.perp();
    float lz = gs.position.z();
    float dr = gs.errorR;
    float dz = gs.errorZ;
    // r[i] = gs.position.perp();
    // phi[i] = gs.position.barePhi();
    x[i] = gs.position.x();
    y[i] = gs.position.y();
    z[i] = lz;
    drphi[i] = gs.errorRPhi;
    u[i] = isBarrel ? lr : lz;
    v[i] = isBarrel ? lz : lr;
    du[i] = isBarrel ? dr : dz;
    dv[i] = isBarrel ? dz : dr;
    lphi[i] = loc.barePhi();
  }

}


RecHitsSortedInPhi::DoubleRange RecHitsSortedInPhi::doubleRange(float phiMin, float phiMax) const {
  Range r1,r2;
  if ( phiMin < phiMax) {
    if ( phiMin < -Geom::fpi()) {
      r1 = unsafeRange( phiMin + Geom::ftwoPi(), Geom::fpi());
      r2 = unsafeRange( -Geom::fpi(), phiMax);
    }
    else if (phiMax > Geom::pi()) {
     r1 = unsafeRange( phiMin, Geom::fpi());
     r2 = unsafeRange( -Geom::fpi(), phiMax-Geom::ftwoPi());
    }
    else {
      r1 = unsafeRange( phiMin, phiMax);
      r2 = Range(theHits.begin(),theHits.begin());
    }
  }
  else {
    r1 =unsafeRange( phiMin, Geom::fpi());
    r2 =unsafeRange( -Geom::fpi(), phiMax);
  }

  return (DoubleRange){{int(r1.first-theHits.begin()),int(r1.second-theHits.begin())
	,int(r2.first-theHits.begin()),int(r2.second-theHits.begin())}};
}


void RecHitsSortedInPhi::hits( float phiMin, float phiMax, std::vector<Hit>& result) const
{
  if ( phiMin < phiMax) {
    if ( phiMin < -Geom::fpi()) {
      copyResult( unsafeRange( phiMin + Geom::ftwoPi(), Geom::fpi()), result);
      copyResult( unsafeRange( -Geom::fpi(), phiMax), result);
    }
    else if (phiMax > Geom::pi()) {
      copyResult( unsafeRange( phiMin, Geom::fpi()), result);
      copyResult( unsafeRange( -Geom::fpi(), phiMax-Geom::ftwoPi()), result);
    }
    else {
      copyResult( unsafeRange( phiMin, phiMax), result);
    }
  }
  else {
    copyResult( unsafeRange( phiMin, Geom::fpi()), result);
    copyResult( unsafeRange( -Geom::fpi(), phiMax), result);
  }
}

std::vector<RecHitsSortedInPhi::Hit> RecHitsSortedInPhi::hits( float phiMin, float phiMax) const
{
  std::vector<Hit> result;
  hits( phiMin, phiMax, result);
  return result;
}

RecHitsSortedInPhi::Range
RecHitsSortedInPhi::unsafeRange( float phiMin, float phiMax) const
{
  auto low = std::lower_bound( theHits.begin(), theHits.end(), HitWithPhi(phiMin), HitLessPhi());
  return Range( low,
	       std::upper_bound(low, theHits.end(), HitWithPhi(phiMax), HitLessPhi()));
}
