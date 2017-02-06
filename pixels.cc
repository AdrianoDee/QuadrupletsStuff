// -*- C++ -*-
//
// Package:    HitEmbeddingAnalyzer
// Class:      HitEmbeddingAnalyzer
//
/**\class HitEmbeddingAnalyzer HitEmbeddingAnalyzer.cc HitEmbedding/HitEmbeddingAnalyzer/src/HitEmbeddingAnalyzer.cc

 Description: <one line class summary>

 Implementation:
     <Notes on implementation>
*/
//
// Original Author:  "David Silvers"
//         Created:  Mon May 18 12:08:07 CDT 2009
// $Id$
//
//


// *************************************************************
// The Analyzer follows the example of Validation/TrackerDigis/plugins/
// SiStripDigiValid.cc
// Goals: Loop over TIB, then loop over each strip and count the number of
// hits on that strip. Write a histogram that stores the number of hits.Then
// repeat for other portions of the tracker.
// *************************************************************


// system include files
#include <memory>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "FWCore/ServiceRegistry/interface/Service.h"
#include "PhysicsTools/UtilAlgos/interface/TFileService.h"
#include "TH1.h"

#include "TNtuple.h"
#include "TTree.h"
#include "TFile.h"


// End of 'standard' libraries. The following libraries are needed for digis

#include "DataFormats/SiStripDigi/interface/SiStripDigi.h"
#include "DataFormats/SiStripDetId/interface/StripSubdetector.h"
#include "DataFormats/SiStripDetId/interface/TIBDetId.h"
#include "DataFormats/Common/interface/DetSetVector.h"

#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"

#include "Geometry/CommonDetUnit/interface/GeomDet.h"
#include "Geometry/TrackerGeometryBuilder/interface/StripGeomDetUnit.h"
#include "Geometry/CommonTopologies/interface/StripTopology.h"
#include "DataFormats/GeometryVector/interface/LocalPoint.h"

#include "DataFormats/GeometryVector/interface/GlobalPoint.h"

#include "Geometry/TrackerGeometryBuilder/interface/PixelGeomDetUnit.h"
#include "Geometry/TrackerGeometryBuilder/interface/RectangularPixelTopology.h"
#include "DataFormats/SiPixelDigi/interface/PixelDigi.h"
#include "DataFormats/SiPixelDetId/interface/PixelSubdetector.h"
#include "DataFormats/SiPixelDetId/interface/PXBDetId.h"
//
// class decleration
//

using namespace std;
using namespace edm;

class HitEmbeddingAnalyzer : public edm::EDAnalyzer {
   public:
      explicit HitEmbeddingAnalyzer(const edm::ParameterSet&);
      ~HitEmbeddingAnalyzer();


   private:
      virtual void beginJob(const edm::EventSetup&) ;
      virtual void analyze(const edm::Event&, const edm::EventSetup&);
      virtual void endJob() ;

      // ----------member data ---------------------------


  TNtuple *n_xyz;
  TH1F *h_histogram;


};

//
// constants, enums and typedefs
//

//
// static data member definitions
//

//
// constructors and destructor
//
HitEmbeddingAnalyzer::HitEmbeddingAnalyzer(const edm::ParameterSet& iConfig)

{
//now do what ever initialization is needed

  cout << "HitEmbeddingAnalyzer: Constructor called. "  << endl;

  edm::Service<TFileService> fs;
  h_histogram = fs->make<TH1F>("histogram","histogram", 2, 0 , 1);

  n_xyz = new TNtuple("xyz", "xyz coordinates of Silicon Pixel", "sub:layer:id:adc:localx:localy:localz:globalx:globaly:globalz");


}


HitEmbeddingAnalyzer::~HitEmbeddingAnalyzer()
{

  // do anything here that needs to be done at desctruction time
  // (e.g. close files, deallocate resources etc.)

  cout << "HitEmbeddingAnalyzer: destructor called. "  << endl;
}


//
// member functions
//

// ------------ method called to for each event  ------------
void
HitEmbeddingAnalyzer::analyze(const edm::Event& iEvent,
                              const edm::EventSetup& iSetup) {

// This handle provides a pointer to the tracker geometry manager classes
  ESHandle<TrackerGeometry> theTrackerGeometry;
  iSetup.get<TrackerDigiGeometryRecord>().get(theTrackerGeometry);


  Handle<DetSetVector<SiStripDigi> > stripDigiHits;
  iEvent.getByLabel("siStripDigis", "ZeroSuppressed", stripDigiHits);

  for(DetSetVector<SiStripDigi>::const_iterator SSDiter = stripDigiHits->begin(); SSDiter != stripDigiHits->end(); SSDiter++) {

    unsigned int id = SSDiter->id;
    DetId detId(id);

    if ( (unsigned int)detId.subdetId() == StripSubdetector::TIB) {
// NOTE: we could also do detId.subdetId() == 3, since the TIB is assigned the numeric value of 3

      TIBDetId tibid(id);

      int layer = (int)tibid.layer();

      DetSet<SiStripDigi>::const_iterator begin=SSDiter->data.begin();
      DetSet<SiStripDigi>::const_iterator end=SSDiter->data.end();
      DetSet<SiStripDigi>::const_iterator itStrip;

      for (itStrip = begin; itStrip !=end; ++itStrip) {

        const SiStripDigi *trial = &*itStrip;
        unsigned short adc = trial->adc();
        float strip = (float)trial->channel();


        const StripGeomDetUnit* theStripDet = dynamic_cast<const StripGeomDetUnit*>((theTrackerGeometry->idToDet(id)));
        const StripTopology *topology = dynamic_cast<const StripTopology*>( &(theStripDet->specificTopology()));

        LocalPoint local = topology->localPosition(strip);

        GlobalPoint global = theStripDet->surface().toGlobal(local);



        n_xyz->Fill(detId.subdetId(), layer, id, adc, local.x(), local.y(), local.z(), global.x(), global.y(), global.z() );



      }
    }
  }




  Handle<DetSetVector<PixelDigi> > pixelDigiHits;
  iEvent.getByLabel("siPixelDigis", pixelDigiHits);

  for(DetSetVector<PixelDigi>::const_iterator SSDiter = pixelDigiHits->begin(); SSDiter != pixelDigiHits->end(); SSDiter++) {

    unsigned int id = SSDiter->id;
    DetId detId(id);

    if ( (unsigned int)detId.subdetId() == PixelSubdetector::PixelBarrel) {
// NOTE: we could also do detId.subdetId() == 3, since the TIB is assigned the numeric value of 3

      PXBDetId pxbid(id);

      int layer = (int)pxbid.layer();

      DetSet<PixelDigi>::const_iterator begin=SSDiter->data.begin();
      DetSet<PixelDigi>::const_iterator end=SSDiter->data.end();
      DetSet<PixelDigi>::const_iterator itPixel;

      for (itPixel = begin; itPixel !=end; ++itPixel) {

        const PixelDigi *trial = &*itPixel;
        unsigned short adc = trial->adc();
      int row = itPixel->row();
      int column = itPixel->column();



        const PixelGeomDetUnit* thePixelDet = dynamic_cast<const PixelGeomDetUnit*>((theTrackerGeometry->idToDet(id)));
        const RectangularPixelTopology *topology = dynamic_cast<const RectangularPixelTopology*>( &(thePixelDet->specificTopology()));

	//        LocalPoint local = topology->localPosition(strip);
        LocalPoint local = topology->localPosition(MeasurementPoint(row,column));
        GlobalPoint global = thePixelDet->surface().toGlobal(local);



        n_xyz->Fill(detId.subdetId(), layer, id, adc, local.x(), local.y(), local.z(), global.x(), global.y(), global.z() );



      }
    }
  }




#ifdef THIS_IS_AN_EVENT_EXAMPLE
   Handle<ExampleData> pIn;
   iEvent.getByLabel("example",pIn);
#endif

#ifdef THIS_IS_AN_EVENTSETUP_EXAMPLE
   ESHandle<SetupData> pSetup;
   iSetup.get<SetupRecord>().get(pSetup);
#endif

}


// ------------ method called once each job just before starting event loop  ------------
void
HitEmbeddingAnalyzer::beginJob(const edm::EventSetup& c)
{
  cout << "HitEmbeddingAnalyzer::beginJob " << endl;

}

// ------------ method called once each job just after ending the event loop  ------------
void
HitEmbeddingAnalyzer::endJob() {5+

  cout << "HitEmbeddingAnalyzer::endJob "  << endl;

}

//define this as a plug-in
DEFINE_FWK_MODULE(HitEmbeddingAnalyzer);
