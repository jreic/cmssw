import FWCore.ParameterSet.Config as cms

diJetAnalyzer = cms.EDAnalyzer(
    'DiJetAnalyzer',
    pfJetCollName       = cms.string('ak4PFJetsCHS'),
    pfJetCorrName       = cms.string('ak4PFCHSL1FastL2L3'),
    hbheRecHitName      = cms.string('hbhereco'),
    hfRecHitName        = cms.string('hfreco'),
    hoRecHitName        = cms.string('horeco'),
    pvCollName          = cms.string('offlinePrimaryVertices'),
    rootHistFilename    = cms.string('dijettree.root'),
    maxDeltaEta         = cms.double(1.5),
    minTagJetEta        = cms.double(0.0),
    maxTagJetEta        = cms.double(5.0),
    minSumJetEt         = cms.double(50.),
    minJetEt            = cms.double(20.),
    maxThirdJetEt       = cms.double(75.),
    debug               = cms.untracked.bool(False)
    )
