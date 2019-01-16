#include "TopTagger/TopTagger/interface/TTMHEPRequirements.h"

#include "TopTagger/TopTagger/interface/TopTaggerResults.h"
#include "TopTagger/CfgParser/include/Context.hh"
#include "TopTagger/CfgParser/include/CfgDocument.hh"

#include <cmath>

void TTMHEPRequirements::getParameters(const cfg::CfgDocument* cfgDoc, const std::string& localContextName)
{
    //Construct contexts
    cfg::Context commonCxt("Common");
    cfg::Context localCxt(localContextName);

    mW_ = cfgDoc->get("mW", commonCxt, -999.9);
    mt_ = cfgDoc->get("mt", commonCxt, -999.9);

    Rmin_ = cfgDoc->get("Rmin", localCxt, -999.9) * mW_/mt_;
    Rmax_ = cfgDoc->get("Rmax", localCxt, -999.9) * mW_/mt_;

    csvThresh_  = cfgDoc->get("csvThreshold", localCxt, -999.9);
    bEtaCut_    = cfgDoc->get("bEtaCut",      localCxt, -999.9);
    maxNbInTop_ = cfgDoc->get("maxNbInTop",   localCxt, -1);

    doMonojet_  = cfgDoc->get("doMonojet", localCxt, false);
    doDijet_    = cfgDoc->get("doDijet",   localCxt, false);
    doTrijet_   = cfgDoc->get("doTrijet",  localCxt, false);
}

void TTMHEPRequirements::run(TopTaggerResults& ttResults)
{
    //Get the list of top candidates as generated by the clustering algo
    std::vector<TopObject>& topCandidates = ttResults.getTopCandidates();
    //Get the list of final tops into which we will stick candidates
    std::vector<TopObject*>& tops = ttResults.getTops();

    for(auto& topCand : topCandidates)
    {
        //Grab the list of constituents which make up this top candidate 
        const std::vector<Constituent const *>& jets = topCand.getConstituents();

        //HEP tagger requirements
        bool passHEPRequirments = false;

        //Get the total candidate mass
        double m123 = topCand.p().M();

        if(doTrijet_ && jets.size() == 3) //trijets
        {
            double m12  = (jets[0]->p() + jets[1]->p()).M();
            double m23  = (jets[1]->p() + jets[2]->p()).M();
            double m13  = (jets[0]->p() + jets[2]->p()).M();

            //Implement HEP mass ratio requirements here
            bool criterionA = 0.2 < atan(m13/m12) &&
                atan(m13/m12) < 1.3 &&
                Rmin_ < m23/m123 &&
                m23/m123 < Rmax_;

            bool criterionB = (pow(Rmin_, 2)*(1+pow(m13/m12, 2)) < (1 - pow(m23/m123, 2))) &&
                ((1 - pow(m23/m123, 2)) < pow(Rmax_, 2)*(1 + pow(m13/m12, 2)));// &&
            //(m23/m123 > 0.35);

            bool criterionC = (pow(Rmin_, 2)*(1+pow(m12/m13, 2)) < (1 - pow(m23/m123, 2))) &&
                ((1 - pow(m23/m123, 2)) < pow(Rmax_, 2)*(1 + pow(m12/m13, 2)));// &&
            //(m23/m123 > 0.35);

            //Requirements on b-quarks
            int Nb = 0;
            for(const auto& jet : jets) if(jet->getBTagDisc() > csvThresh_ && fabs(jet->p().Eta()) < bEtaCut_) ++Nb;
            bool passBrequirements = (Nb <= maxNbInTop_);

            passHEPRequirments = (criterionA || criterionB || criterionC) && passBrequirements;
        }
        else if(doDijet_ && jets.size() == 2) //dijets
        {
            double m23  = (jets[0]->getType() == Constituent::AK8JET)?(jets[0]->getSoftDropMass() * jets[0]->getWMassCorr()):(jets[1]->getSoftDropMass() * jets[1]->getWMassCorr());
            //small hack for legacy tagger
            if(jets[0]->getType() == Constituent::AK4JET && jets[1]->getType() == Constituent::AK4JET) m23 = jets[0]->p().M();

            double m123 = topCand.p().M();
            if(jets[0]->getType() == Constituent::AK8JET)
            {
                TLorentzVector psudoVec;
                psudoVec.SetPtEtaPhiM(jets[0]->p().Pt(), jets[0]->p().Eta(), jets[0]->p().Phi(), jets[0]->getSoftDropMass() * jets[0]->getWMassCorr());
                m123 = (psudoVec + jets[1]->p()).M();
            }
            else if(jets[1]->getType() == Constituent::AK8JET)
            {
                TLorentzVector psudoVec;
                psudoVec.SetPtEtaPhiM(jets[1]->p().Pt(), jets[1]->p().Eta(), jets[1]->p().Phi(), jets[1]->getSoftDropMass() * jets[1]->getWMassCorr());
                m123 = (psudoVec + jets[0]->p()).M();
            }

            //Implement simplified HEP mass ratio requirements for di-jets here
            passHEPRequirments = Rmin_ < m23/m123 && m23/m123 < Rmax_;
        }
        else if(doMonojet_ && jets.size() == 1) //monojets
        {
            //Monojets just get passed if they are active and AK4
            if(jets[0]->getType() == Constituent::AK4JET)
            {
                passHEPRequirments = true;
            }
        }

        //If we pass the HEP requirements add top to final candidates list
        if(passHEPRequirments)
        {
            tops.push_back(&topCand);
        }
    }
}
