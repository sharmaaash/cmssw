#include "RecoEgamma/EgammaElectronAlgos/interface/ElectronHcalHelper.h"
#include "RecoEgamma/EgammaIsolationAlgos/interface/EgammaHadTower.h"
#include "Geometry/Records/interface/CaloGeometryRecord.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "Geometry/Records/interface/CaloGeometryRecord.h"
#include "CondFormats/DataRecord/interface/HcalChannelQualityRcd.h"

using namespace reco;

void ElectronHcalHelper::beginEvent(const edm::Event& evt, const edm::EventSetup& es) {
  if (cfg_.hOverEConeSize == 0) {
    return;
  }

  if (cfg_.useTowers) {
    towersFromCollection_ = &evt.get(cfg_.hcalTowers);
    towerIso1_ =
        std::make_unique<EgammaTowerIsolation>(cfg_.hOverEConeSize, 0., cfg_.hOverEPtMin, 1, towersFromCollection_);
    towerIso2_ =
        std::make_unique<EgammaTowerIsolation>(cfg_.hOverEConeSize, 0., cfg_.hOverEPtMin, 2, towersFromCollection_);

    edm::ESHandle<CaloTowerConstituentsMap> ctmaph;
    es.get<CaloGeometryRecord>().get(ctmaph);
    towerMap_ = ctmaph.product();

    edm::ESHandle<HcalChannelQuality> hQuality;
    es.get<HcalChannelQualityRcd>().get("withTopo", hQuality);
    hcalQuality_ = hQuality.product();
    edm::ESHandle<HcalTopology> hcalTopology;
    es.get<HcalRecNumberingRecord>().get(hcalTopology);
    hcalTopology_ = hcalTopology.product();
  } else {
    edm::Handle<HBHERecHitCollection> hbhe_;
    if (!evt.getByToken(cfg_.hcalRecHits, hbhe_)) {
      edm::LogError("ElectronHcalHelper::readEvent") << "failed to get the rechits";
    }

    hcalIso_ = std::make_unique<EgammaHcalIsolation>(
        cfg_.hOverEConeSize, 0., cfg_.hOverEHBMinE, cfg_.hOverEHFMinE, 0., 0., caloGeom_, *hbhe_);

    unsigned long long newCaloGeomCacheId_ = es.get<CaloGeometryRecord>().cacheIdentifier();
    if (caloGeomCacheId_ != newCaloGeomCacheId_) {
      caloGeomCacheId_ = newCaloGeomCacheId_;
      es.get<CaloGeometryRecord>().get(caloGeom_);
    }
  }
}

std::vector<CaloTowerDetId> ElectronHcalHelper::hcalTowersBehindClusters(const reco::SuperCluster& sc) const {
  return egammaHadTower::towersOf(sc, *towerMap_);
}

double ElectronHcalHelper::hcalESumDepth1BehindClusters(const std::vector<CaloTowerDetId>& towers) const {
  return egammaHadTower::getDepth1HcalESum(towers, *towersFromCollection_);
}

double ElectronHcalHelper::hcalESumDepth2BehindClusters(const std::vector<CaloTowerDetId>& towers) const {
  return egammaHadTower::getDepth2HcalESum(towers, *towersFromCollection_);
}

double ElectronHcalHelper::hcalESum(const SuperCluster& sc, const std::vector<CaloTowerDetId>* excludeTowers) const {
  if (cfg_.hOverEConeSize == 0) {
    return 0;
  }
  if (cfg_.useTowers) {
    return (hcalESumDepth1(sc, excludeTowers) + hcalESumDepth2(sc, excludeTowers));
  } else {
    return hcalIso_->getHcalESum(&sc);
  }
}

double ElectronHcalHelper::hcalESumDepth1(const SuperCluster& sc,
                                          const std::vector<CaloTowerDetId>* excludeTowers) const {
  if (cfg_.hOverEConeSize == 0) {
    return 0;
  }
  if (cfg_.useTowers) {
    return towerIso1_->getTowerESum(&sc, excludeTowers);
  } else {
    return hcalIso_->getHcalESumDepth1(&sc);
  }
}

double ElectronHcalHelper::hcalESumDepth2(const SuperCluster& sc,
                                          const std::vector<CaloTowerDetId>* excludeTowers) const {
  if (cfg_.hOverEConeSize == 0) {
    return 0;
  }
  if (cfg_.useTowers) {
    return towerIso2_->getTowerESum(&sc, excludeTowers);
  } else {
    return hcalIso_->getHcalESumDepth2(&sc);
  }
}

bool ElectronHcalHelper::hasActiveHcal(const reco::SuperCluster& sc) const {
  if (cfg_.checkHcalStatus && cfg_.hOverEConeSize != 0 && cfg_.useTowers) {
    return egammaHadTower::hasActiveHcal(
        egammaHadTower::towersOf(sc, *towerMap_), *towerMap_, *hcalQuality_, *hcalTopology_);
  } else {
    return true;
  }
}
