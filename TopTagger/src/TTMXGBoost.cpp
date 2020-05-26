#include "TopTagger/TopTagger/interface/TTMXGBoost.h"

#include "TopTagger/TopTagger/interface/TopTaggerUtilities.h"
#include "TopTagger/TopTagger/interface/TopObject.h"
#include "TopTagger/TopTagger/interface/TopTaggerResults.h"
#include "TopTagger/CfgParser/interface/Context.hh"
#include "TopTagger/CfgParser/interface/CfgDocument.hh"
#include "TopTagger/CfgParser/interface/TTException.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

void TTMXGBoost::getParameters(const cfg::CfgDocument* cfgDoc, const std::string& localContextName)
{
#ifdef DOXGBOOST
    //Construct contexts
    cfg::Context commonCxt("Common");
    cfg::Context localCxt(localContextName);

    discriminator_ = cfgDoc->get("discCut",       localCxt, -999.9);
    modelFile_     = cfgDoc->get("modelFile",     localCxt, "");
    NConstituents_ = cfgDoc->get("NConstituents", localCxt, 3);
    nCores_        = cfgDoc->get("NCores",        localCxt, 1);

    csvThreshold_  = cfgDoc->get("csvThreshold", localCxt, -999.9);
    bEtaCut_       = cfgDoc->get("bEtaCut",      localCxt, -999.9);
    maxNbInTop_    = cfgDoc->get("maxNbInTop",   localCxt, -1);

    std::string modelFileFullPath;
    if(workingDirectory_.size()) modelFileFullPath = workingDirectory_ + "/" + modelFile_;
    else                         modelFileFullPath = modelFile_;

    int iVar = 0;
    bool keepLooping;
    do
    {
        keepLooping = false;

        //Get variable name
        std::string varName = cfgDoc->get("mvaVar", iVar, localCxt, "");

        //if it is a non empty string save in vector
        if(varName.size() > 0)
        {
            keepLooping = true;

            vars_.push_back(varName);
        }
        ++iVar;
    }
    while(keepLooping);

    //Variable to hold xgboost status
    int status = 0;

    //get the booster from the file
    status =  XGBoosterCreate({}, 0, &h_booster);
    status |= XGBoosterLoadModel(h_booster, modelFileFullPath.c_str());
    status |= XGBoosterSetParam(h_booster, "nthread", std::to_string(nCores_).c_str());

    if(status) 
    {
        THROW_TTEXCEPTION("ERROR: Unable to import model from file: " + modelFileFullPath);
    }

    //resize data vector and map data vector to variable calclator 
    data_.resize(vars_.size());
    //load variables
    if(NConstituents_ == 1)
    {
        varCalculator_.reset(new ttUtility::BDTMonojetInputCalculator());
    }
    else if(NConstituents_ == 2)
    {
        varCalculator_.reset(new ttUtility::BDTDijetInputCalculator());
    }
    else if(NConstituents_ == 3)
    {
        varCalculator_.reset(new ttUtility::TrijetInputCalculator());
    }
    //map variables
    varCalculator_->mapVars(vars_);
    varCalculator_->setPtr(data_.data());

#else
    //Mark variables unused to suppress warnings
    (void)cfgDoc;
    (void)localContextName;
    THROW_TTEXCEPTION("ERROR: TopTagger not compiled with XGBoost support!!!");
#endif
}

void TTMXGBoost::run(TopTaggerResults& ttResults)
{
#ifdef DOXGBOOST
    //Get the list of top candidates as generated by the clustering algo
    std::vector<TopObject>& topCandidates = ttResults.getTopCandidates();
    //Get the list of final tops into which we will stick candidates
    std::vector<TopObject*>& tops = ttResults.getTops();

    //xgboost status variable
    int status = 0;

    for(auto& topCand : topCandidates)
    {
        //Prepare the data!
        if(varCalculator_->calculateVars(topCand, 0))
        {
            // convert to DMatrix (is this unnecessary deep copy necessary?)
            DMatrixHandle h_data;
            status = XGDMatrixCreateFromMat(data_.data(), 1, data_.size(), -1, &h_data);

            //predict value
            bst_ulong out_len;
            const float *output;
            status = XGBoosterPredict(h_booster, h_data, 0,0, &out_len, &output);

            if(status)
            {
                THROW_TTEXCEPTION("ERROR: Unable to run booster");
            }

            if(out_len < 1)
            {
                THROW_TTEXCEPTION("ERROR: Booster produced no output");
            }

            //clean up DMatrix
            XGDMatrixFree(h_data);

            //Get output discriminator 
            topCand.setDiscriminator(output[0]);
            
            //Check number of b-tagged jets in the top
            bool passBrequirements = maxNbInTop_ < 0 || topCand.getNBConstituents(csvThreshold_, bEtaCut_) <= maxNbInTop_;

            //place in final top list if it passes the threshold
            if(topCand.getDiscriminator() > discriminator_ && passBrequirements)
            {
                tops.push_back(&topCand);
            }
        }
    }
#else
    //Mark variables unused to suppress warnings
    (void)ttResults;
#endif
}

TTMXGBoost::~TTMXGBoost()
{    
#ifdef DOXGBOOST

    XGBoosterFree(h_booster);

#endif
}


