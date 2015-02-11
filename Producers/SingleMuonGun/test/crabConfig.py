from WMCore.Configuration import Configuration
config = Configuration()

config.section_("General")
config.General.requestName = 'singleMuonGun_MC_731_5M_v7_DESRUN2_73_V3_TAMU'
config.General.workArea = 'crab_projects'

config.section_("JobType")
config.JobType.pluginName = 'PrivateMC'
config.JobType.psetName = 'singleMuonGun_RECO_731_v3.py'


config.section_("Data")
config.Data.splitting = 'EventBased'
config.Data.unitsPerJob = 5000
NJOBS = 1000
config.Data.totalUnits = config.Data.unitsPerJob * NJOBS
config.Data.publication = False
config.Data.publishDBS = 'phys03'
config.Data.publishDataName = 'CRAB3_singleMuonGun_MC_731_5M_v7_DESRUN2_73_V3_TAMU'
config.Data.outLFN = '/store/user/ar273/'

config.section_("Site")
config.Site.storageSite = 'T3_US_TAMU'
