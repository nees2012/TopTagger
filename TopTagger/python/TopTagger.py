import TopTaggerInterface as tti

class Top:
    def __init__(self, pt, eta, phi, mass, disc, type):
        self.pt = pt
        self.eta = eta
        self.phi = phi
        self.mass = mass
        self.disc = disc
        self.type = type

class TopTaggerResult:
    def __init__(self, results):
        self.floatVals = results[0]
        self.intVals = results[1]

    def __len__(self):
        return self.floatVals.shape[0]

    def __iter__(self):
        for variables in zip(self.ptCol(), self.etaCol(), self.phiCol(), self.massCol(), self.discCol(), self.typeCol()):
            yield Top(*variables)

    def ptCol(self):
        return self.floatVals[:, 0]

    def etaCol(self):
        return self.floatVals[:, 1]

    def phiCol(self):
        return self.floatVals[:, 2]

    def massCol(self):
        return self.floatVals[:, 3]

    def discCol(self):
        return self.floatVals[:, 4]

    def typeCol(self):
        return self.intVals[:, 0]

class TopTagger:

    def __init__(self, cfgFile, workingDir = ""):
        self.cfgFile = cfgFile
        self.workingDir = workingDir
        self.initialize()

    def __enter__(self):
        self.initialize()
        return self

    def __del__(self):
        self.close()

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def initialize(self):
        if not hasattr(self, "tt"):
            self.tt = tti.setup(self.cfgFile, self.workingDir)

    def close(self):
        try:
            del self.tt
        except AttributeError:
            pass

    def run(self, jet_pt, jet_eta, jet_phi, jet_mass, jet_btag, floatVars, intVars):
        tti.run(self.tt, jet_pt, jet_eta, jet_phi, jet_mass, jet_btag, floatVars, intVars)
        results = tti.getResults(self.tt)
        return TopTaggerResult(results)

    def runFromNanoAOD(self, event):
        supplementaryFloatVariables = {
            "qgPtD":                                event.Jet_qgptD,
            "qgAxis1":                              event.Jet_qgAxis1,
            "qgAxis2":                              event.Jet_qgAxis2,
            "recoJetschargedHadronEnergyFraction":  event.Jet_chHEF,
            "recoJetschargedEmEnergyFraction":      event.Jet_chEmEF,
            "recoJetsneutralEmEnergyFraction":      event.Jet_neEmEF,
            "recoJetsmuonEnergyFraction":           event.Jet_muEF,
            "recoJetsHFHadronEnergyFraction":       event.Jet_hfHadEF,
            "recoJetsHFEMEnergyFraction":           event.Jet_hfEMEF,
            "recoJetsneutralEnergyFraction":        event.Jet_neHEF,
            "PhotonEnergyFraction":                 event.Jet_phEF,
            "ElectronEnergyFraction":               event.Jet_elEF,
            "ChargedHadronMultiplicity":            event.Jet_chHadMult,
            "NeutralHadronMultiplicity":            event.Jet_neHadMult,
            "PhotonMultiplicity":                   event.Jet_phMult,
            "ElectronMultiplicity":                 event.Jet_elMult,
            "MuonMultiplicity":                     event.Jet_muMult,
            "DeepCSVb":                             event.Jet_deepCSVb,
            "DeepCSVc":                             event.Jet_deepCSVc,
            "DeepCSVl":                             event.Jet_deepCSVudsg,
            "DeepCSVbb":                            event.Jet_deepCSVbb,
        }
    
        supplementaryIntVariables = {
            "qgMult":                               event.Jet_qgMult,
        }
        
        return self.run(event.Jet_pt, event.Jet_eta, event.Jet_phi, event.Jet_mass, event.Jet_btagCSVV2, supplementaryFloatVariables, supplementaryIntVariables)



if __name__ == "__main__":
    import ROOT
    import optparse

    #Option parsing 
    parser = optparse.OptionParser()

    #Add command line options 
    parser.add_option ('-f', "--file",      dest='inputFile',  action='store',                          type=str, help="Input file")
    parser.add_option ('-b', "--tree",      dest='treeName',   action='store', default="Events",                  help="Name of TTree (Default: Events")
    parser.add_option ('-c', "--taggerCfg", dest='taggerCfg',  action='store', default="TopTagger.cfg",           help="Name of Tagger config file name (Default: TopTagger.cfg")
    parser.add_option ('-w', "--workDir",   dest='workDir',    action='store', default="",                        help="orking directory for top tagger config (usually where the cfg is located) (Default: .")
    parser.add_option ('-n', "--nEvts",     dest='nEvts',      action='store', default=-1,              type=int, help="Number of events to run over (Default: all events")

    options, args = parser.parse_args()

    f = ROOT.TFile.Open(options.inputFile)

    tree = f.Get(options.treeName)

    tt = TopTagger(options.taggerCfg, options.workDir)

    for iEvt, event in enumerate(tree):
        if (options.nEvts > 0) and (iEvt + 1 > options.nEvts): break
    
        print "Event #:", iEvt + 1
    
        tops = tt.runFromNanoAOD(event)
    
        print "\tN tops:", len(tops)
    
        for top in tops:
            print "\tTop properties: Type: %3d,   Pt: %6.1lf,   Eta: %7.3lf,   Phi: %7.3lf,   M: %7.3lf,   Disc: %7.3f"%(top.type, top.pt, top.eta, top.phi, top.mass, top.disc)
        print ""
    
