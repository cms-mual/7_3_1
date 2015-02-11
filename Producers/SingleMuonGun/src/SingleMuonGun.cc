// -*- C++ -*-
//
// Package:    SingleMuonGun
// Class:      SingleMuonGun
// 
/**\class SingleMuonGun SingleMuonGun.cc SingleMuonGun/src/SingleMuonGun.cc

 Description: [one line class summary]

 Implementation:
     [Notes on implementation]
*/
//
// Original Author:  Yuriy Pakhotin,,,
//         Created:  Wed Sep  4 13:36:33 CDT 2013
// $Id: SingleMuonGun.cc,v 1.3 2013/09/08 14:46:47 pakhotin Exp $
//
//

// system include files
#include <memory>
#include <string>
#include <ostream>
#include <iostream>
#include "boost/shared_ptr.hpp"

// user include files
#include "HepPDT/defs.h"
#include "HepPDT/TableBuilder.hh"
#include "HepPDT/ParticleDataTable.hh"

#include "HepMC/GenEvent.h"

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/EDProducer.h"
#include "FWCore/Framework/interface/Run.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "FWCore/Utilities/interface/RandomNumberGenerator.h"

#include "CLHEP/Random/JamesRandom.h"
#include "CLHEP/Random/RandFlat.h"

#include "SimDataFormats/GeneratorProducts/interface/HepMCProduct.h"
#include "SimDataFormats/GeneratorProducts/interface/GenEventInfoProduct.h"
#include "SimDataFormats/GeneratorProducts/interface/GenRunInfoProduct.h"

#include "SimGeneral/HepPDTRecord/interface/ParticleDataTable.h"

#include "FWCore/Framework/interface/LuminosityBlock.h"

using namespace edm;
using namespace std;
using namespace CLHEP;

namespace {
  void getEngineReference()
  {

    Service<RandomNumberGenerator> rng;
    if(!rng.isAvailable()) {
      throw cms::Exception("Configuration")
       << "The RandomNumberProducer module requires the RandomNumberGeneratorService\n"
          "which appears to be absent.  Please add that service to your configuration\n"
          "or remove the modules that require it.";
    }
    // The Service has already instantiated an engine.  Make contact with it.
  }
}



//
// class declaration
//

class SingleMuonGun : public edm::EDProducer {
  public:
  explicit SingleMuonGun(const edm::ParameterSet&);
    ~SingleMuonGun();

  private:
    virtual void beginJob();
    virtual void produce(edm::Event&, const edm::EventSetup&);
    virtual void endJob();
    
    virtual void beginRun(edm::Run&, edm::EventSetup const&);
    virtual void endRun(edm::Run&, edm::EventSetup const&);
      
  // ----------member data ---------------------------
  
  // the event format itself
  HepMC::GenEvent* m_Evt;
  
  ESHandle<HepPDT::ParticleDataTable> m_PDGTable;
    	    	
  int    m_Verbosity;
  int    m_partID;
  double m_minPt;
  double m_maxPt;
  double m_minEta;
  double m_maxEta;
  double m_minPhi;
  double m_maxPhi;
  
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
SingleMuonGun::SingleMuonGun(const edm::ParameterSet& iConfig)
  : m_Evt(0)
  // , m_RandomEngine( getEngineReference() )
  // , m_RandomGenerator(0)
  , m_Verbosity( iConfig.getUntrackedParameter<int>( "Verbosity",0 ) )
  , m_minPt(     iConfig.getParameter<double>("MinPt") )
  , m_maxPt(     iConfig.getParameter<double>("MaxPt") )
  , m_minEta(    iConfig.getParameter<double>("MinEta") )
  , m_maxEta(    iConfig.getParameter<double>("MaxEta") )
  , m_minPhi(    iConfig.getParameter<double>("MinPhi") )
  , m_maxPhi(    iConfig.getParameter<double>("MaxPhi") )
{
  
  // m_RandomGenerator = new CLHEP::RandFlat(m_RandomEngine);
  getEngineReference();
  produces<HepMCProduct>();
  produces<GenEventInfoProduct>();
  produces<GenRunInfoProduct, InRun>();
  
}


SingleMuonGun::~SingleMuonGun()
{
 
   // do anything here that needs to be done at desctruction time
   // (e.g. close files, deallocate resources etc.)

}


//
// member functions
//

// ------------ method called on each new Event  ------------
void SingleMuonGun::produce(edm::Event& iEvent, const edm::EventSetup& iSetup)
{
  
  if ( m_Verbosity > 0 ) cout << " SingleMuonGunProducer : Begin New Event Generation" << endl;
  

  edm::Service<edm::RandomNumberGenerator> rng;
  CLHEP::HepRandomEngine& m_RandomEngine = rng->getEngine(iEvent.streamID());
  CLHEP::RandFlat* m_RandomGenerator = new CLHEP::RandFlat(m_RandomEngine);
  m_Evt = new HepMC::GenEvent();
  
  HepMC::GenVertex* Vtx = new HepMC::GenVertex( HepMC::FourVector(0.,0.,0.) );

  // double muon_sign = m_RandomGenerator->fire(-1.0,  1.0);
  double muon_sign = m_RandomGenerator->fire(-1.0,  1.0);
  if ( muon_sign < 0 ) {
    m_partID = -13;
  } else {
    m_partID = 13;
  }
  
 m_minPt = 30.0;
 m_maxPt = 200.0;
 // double pt    = m_minPt;
 double pt = 0.0;


 double y_rnd = 0.0;
 while(1) {
   pt    = m_RandomGenerator->fire(m_minPt,  m_maxPt);
   y_rnd = m_RandomGenerator->fire(0.0, 0.46684);



   if (pt >= 30.0  && pt < 40.0  && y_rnd < 0.46684     ) break;
   if (pt >= 40.0  && pt < 50.0  && y_rnd < 0.3498604   ) break;
   if (pt >= 50.0  && pt < 60.0  && y_rnd < 0.09976921  ) break;
   if (pt >= 60.0  && pt < 70.0  && y_rnd < 0.03986568  ) break;
   if (pt >= 70.0  && pt < 80.0  && y_rnd < 0.0186057   ) break;
   if (pt >= 80.0  && pt < 90.0  && y_rnd < 0.009773275 ) break;
   if (pt >= 90.0  && pt < 100.0 && y_rnd < 0.0055456   ) break;
   if (pt >= 100.0 && pt < 110.0 && y_rnd < 0.003377719 ) break;
   if (pt >= 110.0 && pt < 120.0 && y_rnd < 0.002205957 ) break;
   if (pt >= 120.0 && pt < 130.0 && y_rnd < 0.001352316 ) break;
   if (pt >= 130.0 && pt < 140.0 && y_rnd < 0.0008818916) break;
   if (pt >= 140.0 && pt < 150.0 && y_rnd < 0.0006362394) break;
   if (pt >= 150.0 && pt < 160.0 && y_rnd < 0.0004544567) break;
   if (pt >= 160.0 && pt < 170.0 && y_rnd < 0.0002898697) break;
   if (pt >= 170.0 && pt < 180.0 && y_rnd < 0.0002481088) break;
   if (pt >= 180.0 && pt < 190.0 && y_rnd < 0.0001658153) break;
   if (pt >= 190.0 && pt < 200.0 && y_rnd < 0.0001277392) break;
 }
  

  double eta = m_RandomGenerator->fire(m_minEta, m_maxEta);
  double phi = m_RandomGenerator->fire(m_minPhi, m_maxPhi);

  
  iSetup.getData( m_PDGTable ) ;

  const HepPDT::ParticleData* PData = m_PDGTable->particle( HepPDT::ParticleID(abs(m_partID)) );


  double mass    = PData->mass().value();
  double theta   = 2.*atan(exp(-eta));
  double mom     = pt/sin(theta);
  double px      = pt*cos(phi);
  double py      = pt*sin(phi);
  double pz      = mom*cos(theta);
  double energy2 = mom*mom + mass*mass;
  double energy  = sqrt(energy2); 
  HepMC::FourVector p(px,py,pz,energy);
  HepMC::GenParticle* Part = new HepMC::GenParticle(p,m_partID,1);

  Part->suggest_barcode( 1 ) ;

  Vtx->add_particle_out(Part);

  m_Evt->add_vertex(Vtx) ;
  m_Evt->set_event_number(iEvent.id().event()) ;
  m_Evt->set_signal_process_id(20) ; 
  
  if ( m_Verbosity > 0 ) m_Evt->print() ;  
  
  auto_ptr<HepMCProduct> BProduct(new HepMCProduct()) ;
  BProduct->addHepMCData( m_Evt );
  iEvent.put(BProduct);

  auto_ptr<GenEventInfoProduct> genEventInfo(new GenEventInfoProduct(m_Evt));
  iEvent.put(genEventInfo);
  
  if ( m_Verbosity > 0 ) cout << " SingleMuonGunProducer : Event Generation Done" << endl;

}

// ------------ method called once each job just before starting event loop  ------------
void 
SingleMuonGun::beginJob()
{
}

// ------------ method called once each job just after ending the event loop  ------------
void 
SingleMuonGun::endJob()
{
}

// ------------ method called when starting to processes a run  ------------
void 
SingleMuonGun::beginRun(edm::Run& iRun, edm::EventSetup const& iSetup)
{

  iSetup.getData( m_PDGTable ) ;
  return;

}

// ------------ method called when ending the processing of a run  ------------
void 
SingleMuonGun::endRun(edm::Run& iRun, edm::EventSetup const& iSetup)
{

  auto_ptr<GenRunInfoProduct> genRunInfo( new GenRunInfoProduct() );
  iRun.put( genRunInfo );

}

//define this as a plug-in
DEFINE_FWK_MODULE(SingleMuonGun);
