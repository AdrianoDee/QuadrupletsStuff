#include "CAHitQuadrupletGenerator.h"

#include "RecoPixelVertexing/PixelTriplets/interface/ThirdHitPredictionFromCircle.h"

#include "RecoPixelVertexing/PixelTriplets/interface/HitQuadrupletGenerator.h"
#include "LayerQuadruplets.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"
#include "FWCore/Framework/interface/Event.h"
#include "DataFormats/Common/interface/Handle.h"


#include "TrackingTools/DetLayers/interface/BarrelDetLayer.h"

#include <sys/time.h> // for timeval
#include <sys/times.h> // for tms

#include <fstream>

#include "CellularAutomaton.h"

#include "CommonTools/Utils/interface/DynArray.h"

#include "FWCore/Utilities/interface/isFinite.h"



namespace
{

  template <typename T>
  T sqr(T x)
  {
    return x*x;
  }
}



using namespace std;
using namespace ctfseeding;

CAHitQuadrupletGenerator::CAHitQuadrupletGenerator(const edm::ParameterSet& cfg, edm::ConsumesCollector& iC) :
theSeedingLayerToken(iC.consumes<SeedingLayerSetsHits>(cfg.getParameter<edm::InputTag>("SeedingLayers"))),
extraHitRPhitolerance(cfg.getParameter<double>("extraHitRPhitolerance")), //extra window in ThirdHitPredictionFromCircle range (divide by R to get phi)
maxChi2(cfg.getParameter<edm::ParameterSet>("maxChi2")),
fitFastCircle(cfg.getParameter<bool>("fitFastCircle")),
fitFastCircleChi2Cut(cfg.getParameter<bool>("fitFastCircleChi2Cut")),
useBendingCorrection(cfg.getParameter<bool>("useBendingCorrection")),
caThetaCut(cfg.getParameter<double>("CAThetaCut")),
caPhiCut(cfg.getParameter<double>("CAPhiCut")),
caHardPtCut(cfg.getParameter<double>("CAHardPtCut"))
{
  if (cfg.exists("SeedComparitorPSet"))
  {
    edm::ParameterSet comparitorPSet =
            cfg.getParameter<edm::ParameterSet>("SeedComparitorPSet");
    std::string comparitorName = comparitorPSet.getParameter<std::string>("ComponentName");
    if (comparitorName != "none")
    {
      theComparitor.reset(SeedComparitorFactory::get()->create(comparitorName, comparitorPSet, iC));
    }
  }
}

CAHitQuadrupletGenerator::~CAHitQuadrupletGenerator()
{
}

void CAHitQuadrupletGenerator::hitQuadruplets(const TrackingRegion& region,
		OrderedHitSeeds & result, const edm::Event& ev,
		const edm::EventSetup& es)
{
	edm::Handle<SeedingLayerSetsHits> hlayers;
	ev.getByToken(theSeedingLayerToken, hlayers);
	const SeedingLayerSetsHits& layers = *hlayers;
	if (layers.numberOfLayersInSet() != 4)
		throw cms::Exception("Configuration")
				<< "CAHitQuadrupletsGenerator expects SeedingLayerSetsHits::numberOfLayersInSet() to be 4, got "
				<< layers.numberOfLayersInSet();

	CAGraph g;


	std::vector<HitDoublets> hitDoublets;
  // std::vector<HitDoublets> hitDoubletsCA;

  // std::ofstream textout ("../Timing/timings.txt", std::ofstream::app);
  // double totTime = 0.0;
  // double totTimeCA = 0.0;
  // double clocks = 0.;
  // 
  // timeval startTime, stopTime, totalTime;
  // clock_t startC, stopC;
  // tms startProc, stopProc;

	HitPairGeneratorFromLayerPair thePairGenerator(0, 1, &theLayerCache);
	for (unsigned int i = 0; i < layers.size(); i++)
	{
		for (unsigned int j = 0; j < 4; ++j)
		{
			auto vertexIndex = 0;
			auto foundVertex = std::find(g.theLayers.begin(), g.theLayers.end(),
					layers[i][j].name());
			if (foundVertex == g.theLayers.end())
			{
				g.theLayers.emplace_back(layers[i][j].name(), layers[i][j].hits().size());
				vertexIndex = g.theLayers.size() - 1;
			}
			else
			{
				vertexIndex = foundVertex - g.theLayers.begin();
			}
			if (j == 0)
			{

				if (std::find(g.theRootLayers.begin(), g.theRootLayers.end(),
						vertexIndex) == g.theRootLayers.end())
				{
					g.theRootLayers.emplace_back(vertexIndex);

				}

			}
			else
			{

				auto innerVertex = std::find(g.theLayers.begin(),
						g.theLayers.end(), layers[i][j - 1].name());

				CALayerPair tmpInnerLayerPair(innerVertex - g.theLayers.begin(),
						vertexIndex);

				if (std::find(g.theLayerPairs.begin(), g.theLayerPairs.end(),
						tmpInnerLayerPair) == g.theLayerPairs.end())
				{
          // gettimeofday(&startTime, NULL);
          // startC = times(&startProc);

					hitDoublets.emplace_back(thePairGenerator.doublets(region, ev, es,
					 		layers[i][j-1], layers[i][j]));

          // stopC = times(&stopProc);
          // gettimeofday(&stopTime, NULL);
          //
          // clocks = (double) (stopC - startC)*10000.;
          //
          // totTime += (clocks /(double)CLOCKS_PER_SEC);
          //
          // gettimeofday(&startTime, NULL);
          // startC = times(&startProc);
          //
          // hitDoubletsCA.emplace_back(thePairGenerator.doubletsCA(region, ev, es,
    			// 				layers[i][j-1], layers[i][j]));
          //
          // stopC = times(&stopProc);
          // gettimeofday(&stopTime, NULL);
          //
          // clocks = (stopC - startC)*10000.;
          //
          // totTimeCA += (clocks / CLOCKS_PER_SEC);

					g.theLayerPairs.push_back(tmpInnerLayerPair);
					g.theLayers[vertexIndex].theInnerLayers.push_back(
							innerVertex - g.theLayers.begin());
					innerVertex->theOuterLayers.push_back(vertexIndex);
					g.theLayers[vertexIndex].theInnerLayerPairs.push_back(
							g.theLayerPairs.size() - 1);
					innerVertex->theOuterLayerPairs.push_back(
							g.theLayerPairs.size() - 1);

				}

			}

      // std::cout<<"Hit Doublets indeces for layers "<<hitDoublets.back().size()<<std::endl;
      // std::cout<<"Inner : "<<i<<" - "<<j-1<<std::endl;
      // std::cout<<"Outer : "<<i<<" - "<<j<<std::endl;
      //
      // for (size_t k = 0; k < hitDoublets.back().size(); ++k) {
      //   std::cout<<"Doublet "<<k<<" : ["<<hitDoublets.back().innerHitId(k)<<" ; "<<hitDoublets.back().outerHitId(k)<<" ]  ---  [("<<hitDoublets.back().x(k,HitDoublets::inner)<<" , "<<hitDoublets.back().y(k,HitDoublets::inner)<<" , "<<hitDoublets.back().z(k,HitDoublets::inner)<<")  ;  "<<hitDoublets.back().x(k,HitDoublets::outer)<<" , "<<hitDoublets.back().y(k,HitDoublets::outer)<<" , "<<hitDoublets.back().z(k,HitDoublets::outer)<<")"<<std::endl;
      // }
      // std::cout<<"Hit Doublets CA indeces for layers - "<<hitDoubletsCA.back().size()<<std::endl;
      // std::cout<<"Inner : "<<i<<" - "<<j-1<<std::endl;
      // std::cout<<"Outer : "<<i<<" - "<<j<<std::endl;
      //
      // for (size_t k = 0; k < hitDoubletsCA.back().size(); ++k) {
      //   std::cout<<"Doublet "<<k<<" : ["<<hitDoubletsCA.back().innerHitId(k)<<" ; "<<hitDoubletsCA.back().outerHitId(k)<<" ]  ---  [("<<hitDoubletsCA.back().x(k,HitDoublets::inner)<<" , "<<hitDoubletsCA.back().y(k,HitDoublets::inner)<<" , "<<hitDoubletsCA.back().z(k,HitDoublets::inner)<<")  ;  "<<hitDoubletsCA.back().x(k,HitDoublets::outer)<<" , "<<hitDoubletsCA.back().y(k,HitDoublets::outer)<<" , "<<hitDoubletsCA.back().z(k,HitDoublets::outer)<<")"<<std::endl;
      // }

		}

	}

  // if(hitDoublets.size()>0 && hitDoubletsCA.size()>0){
  //
  //   int noDoubletsCA = 0;
  //   int noDoublets = 0;
  //
  //   for(int i = 0; i<(int)hitDoublets.size();++i)
  //   {
  //     noDoublets += hitDoublets[i].size();
  //   }
  //
  //   for(int i = 0; i<(int)hitDoubletsCA.size();++i)
  //   {
  //     noDoubletsCA += hitDoubletsCA[i].size();
  //   }
  //
  //   textout<<"------------------------------------------------"<<std::endl;
  //   textout<<"Hit doublets    total time : "<<totTime<<" - no. of Doublets Vectors : "<<hitDoublets.size()<<" no of Doublets : "<<noDoublets<<" avg time : "<< totTime / ((double)hitDoublets.size())<<std::endl;
  //   textout<<"Hit doublets CA total time : "<<totTimeCA<<" - no. of Doublet Vectors : "<<hitDoubletsCA.size()<<" no of Doublets : "<<noDoubletsCA<<" avg time : "<< totTimeCA / ((double) hitDoubletsCA.size())<<std::endl;
  //   textout<<"------------------------------------------------"<<std::endl;
  // }




	if (theComparitor)
		theComparitor->init(ev, es);
	const int numberOfHitsInNtuplet = 4;
	std::vector<CACell::CAntuplet> foundQuadruplets;

	CellularAutomaton ca(g);

	ca.createAndConnectCells(hitDoublets, region, caThetaCut,
			caPhiCut, caHardPtCut);

	ca.evolve(numberOfHitsInNtuplet);

	ca.findNtuplets(foundQuadruplets, numberOfHitsInNtuplet);


	theLayerCache.clear();


	const QuantityDependsPtEval maxChi2Eval = maxChi2.evaluator(es);

  // re-used thoughout, need to be vectors because of RZLine interface
  std::array<float, 4> bc_r;
  std::array<float, 4> bc_z;
  std::array<float, 4> bc_errZ2;
  std::array<GlobalPoint, 4> gps;
  std::array<GlobalError, 4> ges;
  std::array<bool, 4> barrels;

  unsigned int numberOfFoundQuadruplets = foundQuadruplets.size();

  // Loop over quadruplets
  for (unsigned int quadId = 0; quadId < numberOfFoundQuadruplets; ++quadId)
  {

    auto isBarrel = [](const unsigned id) -> bool
    {
      return id == PixelSubdetector::PixelBarrel;
    };
    for(unsigned int i = 0; i< 3; ++i)
    {
        auto const& ahit = foundQuadruplets[quadId][i]->getInnerHit();
        gps[i] = ahit->globalPosition();
        ges[i] = ahit->globalPositionError();
        barrels[i] = isBarrel(ahit->geographicalId().subdetId());
    }

    auto const& ahit = foundQuadruplets[quadId][2]->getOuterHit();
    gps[3] = ahit->globalPosition();
    ges[3] = ahit->globalPositionError();
    barrels[3] = isBarrel(ahit->geographicalId().subdetId());

    PixelRecoLineRZ line(gps[0], gps[2]);
    ThirdHitPredictionFromCircle predictionRPhi(gps[0], gps[2], extraHitRPhitolerance);
    const float curvature = predictionRPhi.curvature(ThirdHitPredictionFromCircle::Vector2D(gps[1].x(), gps[1].y()));
    const float abscurv = std::abs(curvature);
    const float thisMaxChi2 = maxChi2Eval.value(abscurv);

    if (theComparitor)
    {
      SeedingHitSet tmpTriplet(foundQuadruplets[quadId][0]->getInnerHit(), foundQuadruplets[quadId][2]->getInnerHit(), foundQuadruplets[quadId][2]->getOuterHit());


      if (!theComparitor->compatible(tmpTriplet, region) )
      {
        continue;
      }
    }

    float chi2 = std::numeric_limits<float>::quiet_NaN();
    // TODO: Do we have any use case to not use bending correction?
    if (useBendingCorrection)
    {
      // Following PixelFitterByConformalMappingAndLine
      const float simpleCot = ( gps.back().z() - gps.front().z() ) / (gps.back().perp() - gps.front().perp() );
      const float pt = 1.f / PixelRecoUtilities::inversePt(abscurv, es);
      for (int i=0; i < 4; ++i)
      {
        const GlobalPoint & point = gps[i];
        const GlobalError & error = ges[i];
        bc_r[i] = sqrt( sqr(point.x() - region.origin().x()) + sqr(point.y() - region.origin().y()) );
        bc_r[i] += pixelrecoutilities::LongitudinalBendingCorrection(pt, es)(bc_r[i]);
        bc_z[i] = point.z() - region.origin().z();
        bc_errZ2[i] =  (barrels[i]) ? error.czz() : error.rerr(point)*sqr(simpleCot);
      }
      RZLine rzLine(bc_r, bc_z, bc_errZ2, RZLine::ErrZ2_tag());
      chi2 = rzLine.chi2();
    } else
    {
      RZLine rzLine(gps, ges, barrels);
      chi2 = rzLine.chi2();
    }
    if (edm::isNotFinite(chi2) || chi2 > thisMaxChi2)
    {
      continue;

    }
    // TODO: Do we have any use case to not use circle fit? Maybe
    // HLT where low-pT inefficiency is not a problem?
    if (fitFastCircle)
    {
      FastCircleFit c(gps, ges);
      chi2 += c.chi2();
      if (edm::isNotFinite(chi2))
        continue;
      if (fitFastCircleChi2Cut && chi2 > thisMaxChi2)
        continue;
    }

    result.emplace_back(foundQuadruplets[quadId][0]->getInnerHit(), foundQuadruplets[quadId][1]->getInnerHit(), foundQuadruplets[quadId][2]->getInnerHit(), foundQuadruplets[quadId][2]->getOuterHit());
  }

}
