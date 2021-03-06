import FWCore.ParameterSet.Config as cms

def customizePixelTracksSoAonCPU(process):
  
  process.CUDAService = cms.Service('CUDAService',
    enabled = cms.untracked.bool(False)
  )

  # ensure the same results when running on GPU (which supports only the 'HLT' payload) and CPU
  process.siPixelClustersPreSplitting.cpu.payloadType = cms.string('HLT')

  from RecoLocalTracker.SiPixelRecHits.siPixelRecHitSoAFromLegacy_cfi import siPixelRecHitSoAFromLegacy
  process.siPixelRecHitsPreSplitting = siPixelRecHitSoAFromLegacy.clone(
    convertToLegacy = True
  )

  from RecoPixelVertexing.PixelTriplets.caHitNtupletCUDA_cfi import caHitNtupletCUDA
  process.pixelTrackSoA = caHitNtupletCUDA.clone(
    onGPU = False,
    pixelRecHitSrc = 'siPixelRecHitsPreSplitting'
  )

  from RecoPixelVertexing.PixelTrackFitting.pixelTrackProducerFromSoA_cfi import pixelTrackProducerFromSoA
  process.pixelTracks = pixelTrackProducerFromSoA.clone(
    pixelRecHitLegacySrc = 'siPixelRecHitsPreSplitting'
  )

  process.reconstruction_step += process.siPixelRecHitsPreSplitting + process.pixelTrackSoA

  return process


def customizePixelTracksForTriplets(process):

  from HLTrigger.Configuration.common import producers_by_type
  for producer in producers_by_type(process, 'CAHitNtupletCUDA'):
        producer.includeJumpingForwardDoublets = True
        producer.minHitsPerNtuplet = 3
 
  return process
 

def customizePixelTracksSoAonCPUForProfiling(process):

  process = customizePixelTracksSoAonCPU(process)

  process.siPixelRecHitSoAFromLegacy.convertToLegacy = False
  
  process.TkSoA = cms.Path(process.offlineBeamSpot + process.siPixelDigis + process.siPixelClustersPreSplitting + process.siPixelRecHitSoAFromLegacy + process.pixelTrackSoA)

  process.schedule = cms.Schedule(process.TkSoA)

  return process
