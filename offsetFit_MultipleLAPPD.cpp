// This is a script for offset fitting, use it on the output LAPPDTree.root
// This script require all LAPPD events appear in sequence, and all CTC events appear in sequence
/* procedure:
    1. load LAPPDTree.root (TODO: or use LAPPDTree_runNumber.root)
        2. load TimeStamp tree, get run number and part file number
            3. for each unique run number and part file number, do fit:
                4. in this file, for each LAPPD ID:
                    5. loop TimeStamp tree
                        load data timestamp
                        load data beamgate
                        load PPS for board 0 and board 1
                    6. loop Trig (or GTrig) tree
                        load CTC PPS (word = 32)
                        load target trigger word

                    7. Find the number of resets in LAPPD PPS:
                        There must be at least one event in each reset.
                        Based on the event index order, only fit before the reset which doesn't have data event.
                        Save the data event and PPS index order.

                    8. Use the target trigger word, fit the offset
                    9. save the offset for this LAPPD ID, run number, part file number, index, reset number.
               10.  Print info to txt file

Laser trigger word: 47
Undelayed beam trigger word: 14

root -l -q 'offsetFit_MultipleLAPPD.cpp("LAPPDTree.root", 14, 1, 10, 0)'
root -l -q 'offsetFit_MultipleLAPPD.cpp("LAPPDTree.root", 47, 0, 10, 0)'
*/
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <cmath>
#include <algorithm>
#include "TFile.h"
#include "TTree.h"
#include "TH2D.h"
#include "TH1D.h"
#include "TString.h"

vector<vector<ULong64_t>> fitInThisReset(
    const std::vector<ULong64_t> &LAPPDDataTimeStampUL,
    const std::vector<ULong64_t> &LAPPDDataBeamgateUL,
    const std::vector<ULong64_t> &LAPPD_PPS,
    const int fitTargetTriggerWord,
    const std::vector<ULong64_t> &CTCTrigger,
    const std::vector<ULong64_t> &CTCPPS,
    const ULong64_t PPSDeltaT)
{
    cout << "***************************************" << endl;
    cout << "Fitting in this reset with:" << endl;
    cout << "LAPPDDataTimeStampUL size: " << LAPPDDataTimeStampUL.size() << endl;
    cout << "LAPPDDataBeamgateUL size: " << LAPPDDataBeamgateUL.size() << endl;
    cout << "LAPPD_PPS size: " << LAPPD_PPS.size() << endl;
    cout << "CTCTrigger size: " << CTCTrigger.size() << endl;
    cout << "CTCPPS size: " << CTCPPS.size() << endl;
    cout << "PPSDeltaT: " << PPSDeltaT << endl; // in ps
    // for this input PPS, fit an offset
    // return the offset and other information in order
    // precedure:
    // 1. check drift
    // 2. shift the timestmap and beamgate based on drift
    // 3. fit the offset

    std::vector<double> PPSInterval_ACDC;

    for (int i = 1; i < LAPPD_PPS.size(); i++)
    {

        double diff = static_cast<double>(LAPPD_PPS[i] - LAPPD_PPS[i - 1]);
        cout<<fixed<<"PPSDiff = "<<diff<<endl;
        // cout << "LAPPD_PPS[" << i << "]: " << LAPPD_PPS[i] / 1E9 / 1000 << ", diff is " << diff / 1E9 / 1000 << endl;
        //  cout << "diff: " << diff/1E9 << ", 0.9*deltaT = " << 0.9 * PPSDeltaT/1E9 << ", 1.1*deltaT = " << 1.1 * PPSDeltaT/1E9 << endl;
        // only save the time interval that is close to the deltaT, to avoid the clock tick level miss and the whole PPS pulse was missed.
        if (diff > 0.9 * PPSDeltaT && diff < 1.1 * PPSDeltaT)
        {
            PPSInterval_ACDC.push_back((PPSDeltaT - diff) / 1000 / 1000);
            cout << "push back interval in us = " << (PPSDeltaT - diff) / 1000 / 1000 << ", 0.9*PPSdeltaT = " << 0.9 * PPSDeltaT / 1E9 << ", 1.1*PPSdeltaT = " << 1.1 * PPSDeltaT / 1E9 << endl;
        }
        else
        {
            cout << "0.9*PPSdeltaT = " << 0.9 * PPSDeltaT / 1E9 << ", 1.1*PPSdeltaT = " << 1.1 * PPSDeltaT / 1E9 << ", diff = " << (PPSDeltaT - diff) / 1000 / 1000 << endl;
        }
        //cout << "Diff of Interval to deltaT in second is " << (PPSDeltaT - diff) / 1000 / 1E9 << endl;
    }
    int totalPPS = LAPPD_PPS.size();
    int driftedPPS = 0;

    TH1D *h = new TH1D("h", "h", 1000, 0, 1E3);
    for (int i = 0; i < PPSInterval_ACDC.size(); i++)
    {
        // only fill the drift histogram if there is a drift > 2 microseconds
        cout<<"PPSInterval_ACDC["<<i<<"] = "<<PPSInterval_ACDC[i]<<endl;
        if (PPSInterval_ACDC[i] > 2)
        {
            h->Fill(PPSInterval_ACDC[i]); // fill in microseconds
            cout << "Fill histogram: PPSInterval_ACDC[" << i << "]: " << PPSInterval_ACDC[i] << endl;
        }
    }

    TF1 *gausf = new TF1("gausf", "gaus", 0, 1E3);
    h->Fit(gausf, "Q");                                                           // Q for quiet mode
    ULong64_t drift = static_cast<ULong64_t>(gausf->GetParameter(1) * 1E3 * 1E3); // drift in ps
    ULong64_t trueInterval = PPSDeltaT - drift;
    std::cout << "Gaussian Drift in ps is " << drift << std::endl;
    std::cout << "True PPS interval in ps is " << trueInterval << std::endl;
    delete gausf;
    delete h;
    // initialize variables need for fitting
    int orphanCount = 0; // count the number that how many data event timestamp doesn't matched to a target trigger within an interval.
    std::map<double, std::vector<vector<int>>> DerivationMap;
    cout << "LAPPD_PPS.size() " << LAPPD_PPS.size() << " CTCPPS.size() " << CTCPPS.size() << endl;
    for (int i = 0; i < LAPPD_PPS.size(); i++)
    {
        if (i > 5)
        {
            if (i % static_cast<int>(LAPPD_PPS.size() / 5) == 0)
                cout << "Fitting PPS " << i << " of " << LAPPD_PPS.size() << endl;
        }
        ULong64_t LAPPD_PPS_ns = LAPPD_PPS.at(i) / 1000;
        ULong64_t LAPPD_PPS_truncated_ps = LAPPD_PPS.at(i) % 1000;

        for (int j = 0; j < CTCPPS.size(); j++)
        {

            vector<double> diffSum;
            ULong64_t offsetNow_ns = 0;
            if (drift == 0)
            {
                offsetNow_ns = CTCPPS.at(j) - LAPPD_PPS_ns;
            }
            else
            {
                double LAPPD_PPS_ns_dd = static_cast<double>(LAPPD_PPS_ns);
                double driftScaling = LAPPD_PPS_ns_dd / (trueInterval / 1000);
                ULong64_t totalDriftedClock = static_cast<ULong64_t>(drift * driftScaling / 1000);
                offsetNow_ns = CTCPPS.at(j) - (LAPPD_PPS_ns + totalDriftedClock);
                if (i < 3 && j < 3)
                    cout<<"totalDriftedClock in ns = "<<driftScaling <<" number of interval * "<<drift/1000 <<" ns drift per interval = "<<totalDriftedClock<<" ns."<<endl;
            }

            if (i < 3 && j < 3)
                cout << "In ns: CTCPPS.at(" << j << "): " << CTCPPS.at(j) << ", LAPPD_PPS.at(" << i << "): " << LAPPD_PPS.at(i) << ", use LAPPD_PPS_ns: " << LAPPD_PPS_ns << ", offsetNow_ns: " << offsetNow_ns << endl;

            // now, we have offset in ns.
            // We do fit in ns level, then save the previse information in ps level.

            orphanCount = 0;
            vector<int> notOrphanIndex;
            vector<int> ctcPairedIndex;
            vector<int> ctcOrphanPairedIndex;

            for (int lappdb = 0; lappdb < LAPPDDataBeamgateUL.size(); lappdb++)
            {
                ULong64_t TS_ns = LAPPDDataTimeStampUL.at(lappdb) / 1000;
                ULong64_t TS_truncated_ps = LAPPDDataTimeStampUL.at(lappdb) % 1000;
                // if fit for the undelayed beam trigger, use BG
                if (fitTargetTriggerWord == 14)
                {
                    TS_ns = LAPPDDataBeamgateUL.at(lappdb) / 1000;
                    TS_truncated_ps = LAPPDDataBeamgateUL.at(lappdb) % 1000;
                }

                ULong64_t driftCorrectionForTS = 0;
                if (drift != 0)
                {
                    double TS_ns_dd = static_cast<double>(TS_ns);
                    double driftScaling = TS_ns_dd / (trueInterval / 1000);
                    driftCorrectionForTS = static_cast<ULong64_t>(drift * driftScaling / 1000);
                    if(lappdb<3 && i<3 && j<3)
                        cout<<i<<j<<lappdb<<": driftCorrectionForTS = "<<driftScaling <<" number of interval * "<<drift/1000 <<" ns drift per interval = "<<driftCorrectionForTS<<" ns."<<endl;
                }


                // this is the TS we use for matching
                ULong64_t DriftCorrectedTS_ns = TS_ns + offsetNow_ns + driftCorrectionForTS;

                // initialise other matching variables
                ULong64_t minMatchDiff = std::numeric_limits<ULong64_t>::max();
                bool useThisValue = true;
                int minPairIndex = 0;
                string reason = "none";
                Long64_t first_derivation = 0;
                int minIndex = 0;
                double useValue = true;

                // find the best match of target trigger to this TS, in ns level.
                for (int ctcb = 0; ctcb < CTCTrigger.size(); ctcb++)
                {
                    ULong64_t CTCTrigger_ns = CTCTrigger.at(ctcb);
                    Long64_t diff = CTCTrigger_ns - DriftCorrectedTS_ns;
                    if (diff < 0)
                        diff = -diff;

                    if (diff < minMatchDiff)
                    {
                        minMatchDiff = diff;
                        minPairIndex = ctcb;
                    }

                    Long64_t LastBound = diff - minMatchDiff;
                    if (LastBound < 0)
                        LastBound = -LastBound;
                    Long64_t FirstBound = diff - first_derivation;
                    if (FirstBound < 0)
                        FirstBound = -FirstBound;

                    // save the dt for the first matching to determin the matching position in range.
                    if (ctcb == 0)
                        first_derivation = diff;

                    if (ctcb == CTCTrigger.size() - 1 && LastBound / 1E9 < 0.01)
                    {
                        useValue = false;
                        reason = "When matching the last TS, this diff is too close to (or even is) the minMatchDiff at index " + std::to_string(minPairIndex) + " in " + std::to_string(CTCTrigger.size()) + ". All LAPPD TS is out of the end of CTC trigger range";
                    }
                    if (ctcb == CTCTrigger.size() - 1 && FirstBound / 1E9 < 0.01)
                    {
                        useValue = false;
                        reason = "When matching the last TS, this diff is too close to (or even is) the first_derivation at index " + std::to_string(minPairIndex) + " in " + std::to_string(CTCTrigger.size()) + ". All LAPPD TS is out of the beginning of CTC trigger range";
                    }
                }

                if (useValue)
                {
                    diffSum.push_back(minMatchDiff);
                    double minAllowedDiff = 0;
                    double maxAllowedDiff = 100E3;
                    if (fitTargetTriggerWord == 14)
                    {
                        minAllowedDiff = 322E3;
                        maxAllowedDiff = 326E3;
                    }
                    if (minMatchDiff > maxAllowedDiff || minMatchDiff < minAllowedDiff)
                    {
                        // TODO: adjust the limit for laser.
                        orphanCount += 1;
                        ctcOrphanPairedIndex.push_back(minPairIndex);
                    }
                    else
                    {
                        notOrphanIndex.push_back(lappdb);
                        ctcPairedIndex.push_back(minPairIndex);
                    }
                }
                else
                {
                    orphanCount += 1;
                }
            }

            double mean_dev = 0;
            for (int k = 0; k < diffSum.size(); k++)
            {
                mean_dev += diffSum[k];
            }
            if (diffSum.size() > 0)
                mean_dev = mean_dev / diffSum.size();

            double mean_dev_noOrphan = 0;
            for (int k = 0; k < notOrphanIndex.size(); k++)
            {
                if (fitTargetTriggerWord == 14)
                {
                    if (diffSum.at(k) > 322E3 && diffSum.at(k) < 326E3)
                        mean_dev_noOrphan += diffSum[notOrphanIndex[k]];
                }
                else
                {
                    mean_dev_noOrphan += diffSum[notOrphanIndex[k]];
                }
            }
            if ((diffSum.size() - orphanCount) != 0)
            {
                mean_dev_noOrphan = mean_dev_noOrphan / (diffSum.size() - orphanCount);
            }
            else
            {
                mean_dev_noOrphan = -1; // if all timestamps are out of the range, set it to -1
            }

            bool increMean_dev = false;
            int maxAttempts = 1000;
            int attemptCount = 0;

            if (mean_dev > 0)
            {
                int increament_dev = 0;
                while (true)
                {
                    // TODO
                    auto iter = DerivationMap.find(mean_dev);
                    if (iter == DerivationMap.end() || iter->second.empty())
                    {
                        vector<int> Info = {i, j, orphanCount, static_cast<int>(mean_dev_noOrphan * 1000), increament_dev};
                        DerivationMap[mean_dev].push_back(Info);
                        DerivationMap[mean_dev].push_back(notOrphanIndex);
                        DerivationMap[mean_dev].push_back(ctcPairedIndex);
                        DerivationMap[mean_dev].push_back(ctcOrphanPairedIndex);
                        break;
                    }
                    else
                    {
                        increament_dev += 1;
                        mean_dev += 0.001; // if the mean_dev is already in the map, increase it by 1ps
                        attemptCount += 1;
                        if (attemptCount > maxAttempts)
                            break;
                    }
                }
            }
        }
    }
    // finish matching, found the minimum mean_dev in the map, extract the matching information
    double min_mean_dev = std::numeric_limits<double>::max();
    int final_i = 0;
    int final_j = 0;
    int gotOrphanCount = 0;
    double gotMin_mean_dev_noOrphan = 0;
    double increament_times = 0;
    vector<int> final_notOrphanIndex;
    vector<int> final_ctcPairedIndex;
    vector<int> final_ctcOrphanIndex;
    for (const auto &minIter : DerivationMap)
    {
        if (minIter.first > 10 && minIter.first < min_mean_dev)
        {
            min_mean_dev = minIter.first;
            final_i = minIter.second[0][0];
            final_j = minIter.second[0][1];
            gotOrphanCount = minIter.second[0][2];
            gotMin_mean_dev_noOrphan = static_cast<int>(minIter.second[0][3] / 1000);
            increament_times = minIter.second[0][4];
            final_notOrphanIndex = minIter.second[1];
            final_ctcPairedIndex = minIter.second[2];
            final_ctcOrphanIndex = minIter.second[3];
        }
    }

    ULong64_t final_offset_ns = 0;
    ULong64_t final_offset_ps_negative = 0;
    if (drift == 0)
    {
        final_offset_ns = CTCPPS.at(final_j) - (LAPPD_PPS.at(final_i) / 1000);
        final_offset_ps_negative = LAPPD_PPS.at(final_i) % 1000;
    }
    else
    {
        ULong64_t LAPPD_PPS_ns = LAPPD_PPS.at(final_i) / 1000;
        ULong64_t LAPPD_PPS_truncated_ps = LAPPD_PPS.at(final_i) % 1000;
        double driftScaling = static_cast<double>(LAPPD_PPS_ns) / (trueInterval / 1000); // this is the same drift scaling as in the matching loop
        ULong64_t totalDriftedClock = static_cast<ULong64_t>(drift * driftScaling / 1000);
        cout<<fixed<<"Found CTCPPS as "<<CTCPPS.at(final_j)<<", LAPPD_PPS_ns = "<<LAPPD_PPS_ns;
        final_offset_ns = CTCPPS.at(final_j) - (LAPPD_PPS_ns + totalDriftedClock);
        final_offset_ps_negative = LAPPD_PPS_truncated_ps; // this is useless if there is a drift though.
        cout<<" final_offset_ns = "<<final_offset_ns<<", driftScaling = "<<driftScaling<<", totalDriftedClock_ns = "<<totalDriftedClock<<endl;
    }
    /*
    cout << "******* Fit Finished *******" << endl;
    cout << "*** Final offset in is " << final_offset_ns << " ns minus " << final_offset_ps_negative << " ps" << endl;
    cout << "*** Final orphan count is " << gotOrphanCount << endl;
    cout << "*** Final mean_dev_noOrphan is " << gotMin_mean_dev_noOrphan << " ps" << endl;
    cout << "*** Final increament times in this result is " << increament_times << endl;
    cout << "*** Final mean deviation is " << min_mean_dev << " ps" << endl;
    cout << "*** Final PPS index is " << final_i << ", in total of " << LAPPD_PPS.size() << endl;
    cout << "*** Final CTC PPS index is " << final_j << ", in total of " << CTCPPS.size() << endl;
    cout << "***************************" << endl;
    cout << "******* Saving *************" << endl;
    */

    cout << "\033[1;34m******* Fit Finished *******\033[0m" << endl;

    cout << "\033[1;34m*** Final offset in is \033[1;31m" << final_offset_ns << "\033[1;34m ns minus \033[1;31m" << final_offset_ps_negative << "\033[1;34m ps\033[0m" << endl;
    cout << "\033[1;34m*** Final orphan count is \033[1;31m" << gotOrphanCount << "\033[0m" << endl;
    cout << "\033[1;34m*** Final mean_dev_noOrphan is \033[1;31m" << gotMin_mean_dev_noOrphan << "\033[1;34m ns\033[0m" << endl;
    cout << "\033[1;34m*** Final increament times in this result is \033[1;31m" << increament_times << "\033[0m" << endl;
    cout << "\033[1;34m*** Final mean deviation is \033[1;31m" << min_mean_dev << "\033[1;34m ns\033[0m" << endl;
    cout << "\033[1;34m*** Final PPS index is \033[1;31m" << final_i << "\033[1;34m, in total of \033[1;31m" << LAPPD_PPS.size() << "\033[0m" << endl;
    cout << "\033[1;34m*** Final CTC PPS index is \033[1;31m" << final_j << "\033[1;34m, in total of \033[1;31m" << CTCPPS.size() << "\033[0m" << endl;
    cout << "\033[1;34m***************************\033[0m" << endl;
    cout << "\033[1;34m******* Saving *************\033[0m" << endl;

    // now, based on this offset, calculate the event time for each event, and save to result.
    vector<ULong64_t> TimeStampRaw;
    vector<ULong64_t> BeamGateRaw;
    vector<ULong64_t> TimeStamp_ns;
    vector<ULong64_t> BeamGate_ns;
    vector<ULong64_t> TimeStamp_ps;
    vector<ULong64_t> BeamGate_ps;
    vector<ULong64_t> EventIndex;
    vector<ULong64_t> EventDeviation_ns;
    vector<ULong64_t> CTCTriggerIndex;
    vector<ULong64_t> CTCTriggerTimeStamp_ns;
    vector<ULong64_t> BeamGate_correction_tick;
    vector<ULong64_t> TimeStamp_correction_tick;
    vector<long long> PPS_tick_correction; // if timestamp falls in between of PPS i and i+1, corrected timestamp = timestamp + PPS_tick_correction[i]
    vector<long long> LAPPD_PPS_missing_ticks;
    vector<ULong64_t> LAPPD_PPS_interval_ticks;

    vector<ULong64_t> BG_PPSBefore;
    vector<ULong64_t> BG_PPSAfter;
    vector<ULong64_t> BG_PPSDiff;
    vector<ULong64_t> BG_PPSMiss;
    vector<ULong64_t> TS_PPSBefore;
    vector<ULong64_t> TS_PPSAfter;
    vector<ULong64_t> TS_PPSDiff;
    vector<ULong64_t> TS_PPSMiss;

    vector<ULong64_t> TS_driftCorrection_ns;
    vector<ULong64_t> BG_driftCorrection_ns;

    // calculate the missing ticks for each LAPPD PPS
    PPS_tick_correction.push_back(0);
    LAPPD_PPS_missing_ticks.push_back(0);
    LAPPD_PPS_interval_ticks.push_back(0);
    ULong64_t intervalTicks = 320000000 * (PPSDeltaT / 1000 / 1E9);
    for (int i = 1; i < LAPPD_PPS.size(); i++)
    {
        ULong64_t thisPPS = LAPPD_PPS.at(i) / 3125;
        ULong64_t prevPPS = LAPPD_PPS.at(i - 1) / 3125;
        long long thisInterval = thisPPS - prevPPS;
        long long thisMissingTicks = intervalTicks - thisInterval;
        LAPPD_PPS_interval_ticks.push_back(thisInterval);

        // if the difference between this PPS and previous PPS is > intervalTicks-20 and < intervalTicks+5,
        // the missed value is thisInterval - intervalTicks
        // push this value plus the sum of previous missing ticks to the vector
        // else just push the sum of previous missing ticks
        long long sumOfPreviousMissingTicks = 0;
        for (int j = 0; j < LAPPD_PPS_missing_ticks.size(); j++)
        {
            sumOfPreviousMissingTicks += LAPPD_PPS_missing_ticks.at(j);
        }

        if (thisMissingTicks > -20 && thisMissingTicks < 20)
        {
            LAPPD_PPS_missing_ticks.push_back(thisMissingTicks);
            PPS_tick_correction.push_back(thisMissingTicks + sumOfPreviousMissingTicks);
        }
        else
        {
            // some time one pps might be wired, but the combination of two is ok.
            bool combined = false;
            // if there are missing PPS, interval is like 22399999990 % 3200000000 = 3199999990
            long long pInterval = thisInterval % intervalTicks;
            long long missingPTicks = 0;
            if (intervalTicks - pInterval < 30)
                missingPTicks = intervalTicks - pInterval;
            else if (pInterval < 30)
                missingPTicks = -pInterval;
            //////
            if (missingPTicks == 1 || missingPTicks == -1)
            {
                cout << "Found missing tick is " << missingPTicks << ",continue." << endl;
                LAPPD_PPS_missing_ticks.push_back(0);
                PPS_tick_correction.push_back(sumOfPreviousMissingTicks);
                continue;
            }

            if (missingPTicks != 0)
            {
                LAPPD_PPS_missing_ticks.push_back(missingPTicks);
                PPS_tick_correction.push_back(missingPTicks + sumOfPreviousMissingTicks);
                combined = true;
                cout << "Pushing PPS correction " << i << ", this PPS interval tick is " << thisInterval << ", missing ticks: " << thisMissingTicks << ", push missing " << LAPPD_PPS_missing_ticks.at(i) << ", push correction " << PPS_tick_correction.at(i) << endl;
                continue;
            }

            // if one PPS is not recoreded correctly, like one interval is 1574262436, followed by a 4825737559
            // then 1574262436 + 4825737559 = 6399999995 = 3200000000 * 2 - 5
            long long nextInterval = 0;
            if (i < LAPPD_PPS.size() - 1)
            {
                nextInterval = LAPPD_PPS.at(i + 1) / 3125 - thisPPS;
                long long combinedMissingTicks = intervalTicks - (nextInterval + thisInterval) % intervalTicks;
                if (combinedMissingTicks > -30 && combinedMissingTicks < 30)
                {
                    LAPPD_PPS_missing_ticks.push_back(combinedMissingTicks);
                    PPS_tick_correction.push_back(combinedMissingTicks + sumOfPreviousMissingTicks);
                    combined = true;
                    cout << "Pushing PPS correction " << i << ", this PPS interval tick is " << thisInterval << ", missing ticks: " << thisMissingTicks << ", push missing " << LAPPD_PPS_missing_ticks.at(i) << ", push correction " << PPS_tick_correction.at(i) << endl;
                    continue;
                }
            }

            if (!combined)
            {
                LAPPD_PPS_missing_ticks.push_back(0);
                PPS_tick_correction.push_back(sumOfPreviousMissingTicks);
            }
        }
        cout << "Pushing PPS correction " << i << ", this PPS interval tick is " << thisInterval << ", missing ticks: " << thisMissingTicks << ", push missing " << LAPPD_PPS_missing_ticks.at(i) << ", push correction " << PPS_tick_correction.at(i) << endl;
    }

    // loop all data events, plus the offset, save the event time and beamgate time
    // fing the closest CTC trigger, also save all information
    cout << "Start saving results. PPS size: " << LAPPD_PPS.size() << ", beamgate size: " << LAPPDDataBeamgateUL.size() << endl;
    cout << "First PPS: " << LAPPD_PPS.at(0) / 3125 << ", Last PPS: " << LAPPD_PPS.at(LAPPD_PPS.size() - 1) / 3125 << endl;
    cout << "First BG: " << LAPPDDataBeamgateUL.at(0) / 3125 << ", Last BG: " << LAPPDDataBeamgateUL.at(LAPPDDataBeamgateUL.size() - 1) / 3125 << endl;
    cout << "First TS: " << LAPPDDataTimeStampUL.at(0) / 3125 << ", Last TS: " << LAPPDDataTimeStampUL.at(LAPPDDataTimeStampUL.size() - 1) / 3125 << endl;

    for (int l = 0; l < LAPPDDataTimeStampUL.size(); l++)
    {
        ULong64_t TS_ns = LAPPDDataTimeStampUL.at(l) / 1000;
        ULong64_t TS_truncated_ps = LAPPDDataTimeStampUL.at(l) % 1000;
        ULong64_t BG_ns = LAPPDDataBeamgateUL.at(l) / 1000;
        ULong64_t BG_truncated_ps = LAPPDDataBeamgateUL.at(l) % 1000;
        ULong64_t driftCorrectionForTS = 0;
        ULong64_t driftCorrectionForBG = 0;
        if (drift != 0)
        {
            
            double driftScaling = static_cast<double>(TS_ns) / (trueInterval / 1000);
            driftCorrectionForTS = static_cast<ULong64_t>(drift/ 1000 * driftScaling );
            double driftScalingBG = static_cast<double>(BG_ns) / (trueInterval / 1000);
            driftCorrectionForBG = static_cast<ULong64_t>(drift/ 1000 * driftScalingBG );
            cout<<"drift = "<<drift<<", driftScaling = "<<driftScaling<<", driftCorrectionForTS = "<<driftCorrectionForTS<<", driftCorrectionForBG = "<<driftCorrectionForBG<<endl;
            cout<<"driftCorrectionForTS = "<<driftCorrectionForTS<<", driftCorrectionForBG = "<<driftCorrectionForBG<<endl;
        }
        ULong64_t DriftCorrectedTS_ns = TS_ns + final_offset_ns + driftCorrectionForTS;
        ULong64_t DriftCorrectedBG_ns = BG_ns + final_offset_ns + driftCorrectionForBG;
        cout<<"DriftCorrectedTS_ns = "<<DriftCorrectedTS_ns<<" =: final_offset_ns = "<<final_offset_ns<<" + driftCorrectionForTS = "<<driftCorrectionForTS<<" + TS_ns = "<<TS_ns<<endl;
        ULong64_t min_mean_dev_match = std::numeric_limits<ULong64_t>::max();
        int matchedIndex = 0;
        for (int c = 0; c < CTCTrigger.size(); c++)
        {
            ULong64_t CTCTrigger_ns = CTCTrigger.at(c);
            Long64_t diff = CTCTrigger_ns - DriftCorrectedTS_ns;
            if (diff < 0)
                diff = -diff;
            if (diff < min_mean_dev_match)
            {
                matchedIndex = c;
                min_mean_dev_match = diff;
            }
        }

        // check the position of beamgate and timestamp raw fall into which pps interval;
        bool TSFound = false;
        bool BGFound = false;
        for (int i = 0; i < LAPPD_PPS.size() - 1; i++)
        {
            if (LAPPD_PPS.at(i) / 3125 < LAPPDDataTimeStampUL.at(l) / 3125 && LAPPD_PPS.at(i + 1) / 3125 > LAPPDDataTimeStampUL.at(l) / 3125)
            {
                TimeStamp_correction_tick.push_back(PPS_tick_correction.at(i) + 1000);
                TSFound = true;
                break;
            }
        }
        if (!TSFound && LAPPDDataTimeStampUL.at(l) / 3125 > LAPPD_PPS.at(LAPPD_PPS.size() - 1) / 3125)
        {
            TimeStamp_correction_tick.push_back(PPS_tick_correction.at(LAPPD_PPS.size() - 1) + 1000);
            TSFound = true;
        }
        if (!TSFound && LAPPDDataTimeStampUL.at(l) / 3125 < LAPPD_PPS.at(0) / 3125)
        {
            TimeStamp_correction_tick.push_back(0 + 1000);
            TSFound = true;
        }

        for (int i = 0; i < LAPPD_PPS.size() - 1; i++)
        {
            if (LAPPD_PPS.at(i) / 3125 < LAPPDDataBeamgateUL.at(l) / 3125 && LAPPD_PPS.at(i + 1) / 3125 > LAPPDDataBeamgateUL.at(l) / 3125)
            {
                BeamGate_correction_tick.push_back(PPS_tick_correction.at(i) + 1000);
                BGFound = true;
                cout << "Normal push: BGraw = " << LAPPDDataBeamgateUL.at(l) / 3125 << ", pps = " << LAPPD_PPS.at(i) << ", pps/3125 = " << LAPPD_PPS.at(i) / 3125 << endl;
                if (LAPPD_PPS_interval_ticks.at(i) != intervalTicks)
                {
                    cout << "Warning: PPS interval is not " << intervalTicks << " at index " << i << ", it is " << LAPPD_PPS_interval_ticks.at(i) << endl;
                }
                break;
            }
        }
        if (!BGFound && LAPPDDataBeamgateUL.at(l) / 3125 > LAPPD_PPS.at(LAPPD_PPS.size() - 1) / 3125)
        {
            BeamGate_correction_tick.push_back(PPS_tick_correction.at(LAPPD_PPS.size() - 1) + 1000);
            cout << "BGraw = " << LAPPDDataBeamgateUL.at(l) / 3125 << ", pps = " << LAPPD_PPS.at(LAPPD_PPS.size() - 1) << ", pps/3125 = " << LAPPD_PPS.at(LAPPD_PPS.size() - 1) / 3125 << endl;
            BGFound = true;
        }
        if (!BGFound && LAPPDDataBeamgateUL.at(l) / 3125 < LAPPD_PPS.at(0) / 3125)
        {
            BeamGate_correction_tick.push_back(0 + 1000);
            cout << "BGraw less than pps0" << endl;
            BGFound = true;
        }
        if (!TSFound || !BGFound)
        {
            cout << "Error: PPS not found for event " << l << ", TSFound: " << TSFound << ", BGFound: " << BGFound << endl;
        }
        /*
        cout << "******Found result:" << endl;
        cout << "LAPPDDataTimeStampUL.at(" << l << "): " << LAPPDDataTimeStampUL.at(l)/3125 << endl;
        cout << "LAPPDDataBeamgateUL.at(" << l << "): " << LAPPDDataBeamgateUL.at(l)/3125 << endl;
        cout << "TS_ns: " << TS_ns << endl;
        cout << "BG_ns: " << BG_ns << endl;
        cout << "final_offset_ns: " << final_offset_ns << endl;
        cout << "drift: " << drift << endl;
        cout << "TS driftscaling: " << TS_ns / trueInterval / 1000 << endl;
        cout << "DriftCorrectedTS_ns: " << DriftCorrectedTS_ns << endl;
        cout << "TS_truncated_ps: " << TS_truncated_ps << endl;
        cout << "DriftCorrectedBG_ns: " << DriftCorrectedBG_ns << endl;
        cout << "BG_truncated_ps: " << BG_truncated_ps << endl;
        cout << "Found min_mean_dev_match: " << min_mean_dev_match << endl;
        cout << "MatchedIndex: " << matchedIndex << endl;
        cout << "CTCTrigger.at(" << matchedIndex << "): " << CTCTrigger.at(matchedIndex) << endl;
        cout << "BeamGate_correction_tick at " << l << ": " << BeamGate_correction_tick.at(l) << endl;
        cout << "TimeStamp_correction_tick at " << l << ": " << TimeStamp_correction_tick.at(l) << endl;
        */

        // for this LAPPDDataBeamgateUL.at(l), in the LAPPD_PPS vector, find it's closest PPS before and after, and also calculate the time difference between them, and the missing tick between them.
        // save as BG_PPSBefore_tick, BG_PPSAfter_tick, BG_PPSDiff_tick, BG_PPSMissing_tick
        // Do the same thing for LAPPDDataTimeStampUL.at(l), save as TS_PPSBefore_tick, TS_PPSAfter_tick, TS_PPSDiff_tick, TS_PPSMissing_tick

        ULong64_t BG_PPSBefore_tick = 0;
        ULong64_t BG_PPSAfter_tick = 0;
        ULong64_t BG_PPSDiff_tick = 0;
        ULong64_t BG_PPSMissing_tick = 0;

        ULong64_t TS_PPSBefore_tick = 0;
        ULong64_t TS_PPSAfter_tick = 0;
        ULong64_t TS_PPSDiff_tick = 0;
        ULong64_t TS_PPSMissing_tick = 0;
        // if the first PPS is later than beamgate, then set before = 0, after is the first, diff is the first, missing is the first

        cout << "Start finding PPS before and after the beamgate" << endl;

        if ((LAPPD_PPS.at(0) / 3125 > LAPPDDataBeamgateUL.at(l) / 3125) || (LAPPD_PPS.at(LAPPD_PPS.size() - 1) / 3125 < LAPPDDataBeamgateUL.at(l) / 3125))
        {
            if (LAPPD_PPS.at(0) / 3125 > LAPPDDataBeamgateUL.at(l) / 3125)
            {
                BG_PPSBefore_tick = 0;
                BG_PPSAfter_tick = LAPPD_PPS.at(0) / 3125;
                BG_PPSDiff_tick = LAPPD_PPS.at(0) / 3125;
                BG_PPSMissing_tick = LAPPD_PPS.at(0) / 3125;
                cout << "First PPS is later than beamgate, before is 0, after is the first, diff is the first, missing is the first" << endl;
            }
            // if the last PPS is earlier than beamgate, then set before is the last, after = 0, diff is the last, missing is the last
            if (LAPPD_PPS.at(LAPPD_PPS.size() - 1) / 3125 < LAPPDDataBeamgateUL.at(l) / 3125)
            {
                BG_PPSBefore_tick = LAPPD_PPS.at(LAPPD_PPS.size() - 1) / 3125;
                BG_PPSAfter_tick = 0;
                BG_PPSDiff_tick = LAPPD_PPS.at(LAPPD_PPS.size() - 1) / 3125;
                BG_PPSMissing_tick = LAPPD_PPS.at(LAPPD_PPS.size() - 1) / 3125;
                cout << "Last PPS is earlier than beamgate, before is the last, after is 0, diff is the last, missing is the last" << endl;
            }
        }
        else
        {
            // if the first PPS is earlier than beamgate, and the last PPS is later than beamgate, then find the closest PPS before and after
            for (int i = 0; i < LAPPD_PPS.size() - 1; i++)
            {
                if (LAPPD_PPS.at(i) / 3125 < LAPPDDataBeamgateUL.at(l) / 3125 && LAPPD_PPS.at(i + 1) / 3125 > LAPPDDataBeamgateUL.at(l) / 3125)
                {
                    BG_PPSBefore_tick = LAPPD_PPS.at(i) / 3125;
                    BG_PPSAfter_tick = LAPPD_PPS.at(i + 1) / 3125;
                    BG_PPSDiff_tick = LAPPD_PPS.at(i + 1) / 3125 - LAPPD_PPS.at(i) / 3125;
                    ULong64_t DiffTick = (LAPPD_PPS.at(i + 1) - LAPPD_PPS.at(i)) / 3125 - 1000;
                    BG_PPSMissing_tick = 3200000000 - DiffTick;
                    cout << "Found PPS before and after the beamgate, before: " << BG_PPSBefore_tick << ", after: " << BG_PPSAfter_tick << ", diff: " << BG_PPSDiff_tick << ", missing: " << BG_PPSMissing_tick << endl;
                    break;
                }
            }
        }

        // do the samething for timestamp
        cout << "Start finding PPS before and after the timestamp" << endl;

        if ((LAPPD_PPS.at(0) / 3125 > LAPPDDataTimeStampUL.at(l) / 3125) || (LAPPD_PPS.at(LAPPD_PPS.size() - 1) / 3125 < LAPPDDataTimeStampUL.at(l) / 3125))
        {
            if (LAPPD_PPS.at(0) / 3125 > LAPPDDataTimeStampUL.at(l) / 3125)
            {
                TS_PPSBefore_tick = 0;
                TS_PPSAfter_tick = LAPPD_PPS.at(0) / 3125;
                TS_PPSDiff_tick = LAPPD_PPS.at(0) / 3125;
                TS_PPSMissing_tick = LAPPD_PPS.at(0) / 3125;
                cout << "First PPS is later than timestamp, before is 0, after is the first, diff is the first, missing is the first" << endl;
            }
            if (LAPPD_PPS.at(LAPPD_PPS.size() - 1) / 3125 < LAPPDDataTimeStampUL.at(l) / 3125)
            {
                TS_PPSBefore_tick = LAPPD_PPS.at(LAPPD_PPS.size() - 1) / 3125;
                TS_PPSAfter_tick = 0;
                TS_PPSDiff_tick = LAPPD_PPS.at(LAPPD_PPS.size() - 1) / 3125;
                TS_PPSMissing_tick = LAPPD_PPS.at(LAPPD_PPS.size() - 1) / 3125;
                cout << "Last PPS is earlier than timestamp, before is the last, after is 0, diff is the last, missing is the last" << endl;
            }
        }
        else
        {
            for (int i = 0; i < LAPPD_PPS.size() - 1; i++)
            {
                if (LAPPD_PPS.at(i) / 3125 < LAPPDDataTimeStampUL.at(l) / 3125 && LAPPD_PPS.at(i + 1) / 3125 > LAPPDDataTimeStampUL.at(l) / 3125)
                {
                    TS_PPSBefore_tick = LAPPD_PPS.at(i) / 3125;
                    TS_PPSAfter_tick = LAPPD_PPS.at(i + 1) / 3125;
                    TS_PPSDiff_tick = LAPPD_PPS.at(i + 1) / 3125 - LAPPD_PPS.at(i) / 3125;
                    ULong64_t DiffTick = (LAPPD_PPS.at(i + 1) - LAPPD_PPS.at(i)) / 3125 - 1000;
                    TS_PPSMissing_tick = 3200000000 - DiffTick;
                    cout << "Found PPS before and after the timestamp, before: " << TS_PPSBefore_tick << ", after: " << TS_PPSAfter_tick << ", diff: " << TS_PPSDiff_tick << ", missing: " << TS_PPSMissing_tick << endl;
                    break;
                }
            }
        }

        TimeStampRaw.push_back(LAPPDDataTimeStampUL.at(l) / 3125);
        BeamGateRaw.push_back(LAPPDDataBeamgateUL.at(l) / 3125);
        TimeStamp_ns.push_back(DriftCorrectedTS_ns);
        BeamGate_ns.push_back(DriftCorrectedBG_ns);
        TimeStamp_ps.push_back(TS_truncated_ps);
        BeamGate_ps.push_back(BG_truncated_ps);
        EventIndex.push_back(l);
        EventDeviation_ns.push_back(min_mean_dev_match);
        // to get ps, should minus the TS_truncated_ps
        CTCTriggerIndex.push_back(matchedIndex);
        CTCTriggerTimeStamp_ns.push_back(CTCTrigger.at(matchedIndex));

        BG_PPSBefore.push_back(BG_PPSBefore_tick);
        BG_PPSAfter.push_back(BG_PPSAfter_tick);
        BG_PPSDiff.push_back(BG_PPSDiff_tick);
        BG_PPSMiss.push_back(BG_PPSMissing_tick);
        TS_PPSBefore.push_back(TS_PPSBefore_tick);
        TS_PPSAfter.push_back(TS_PPSAfter_tick);
        TS_PPSDiff.push_back(TS_PPSDiff_tick);
        TS_PPSMiss.push_back(TS_PPSMissing_tick);

        TS_driftCorrection_ns.push_back(driftCorrectionForTS);
        BG_driftCorrection_ns.push_back(driftCorrectionForBG);
    }
    ULong64_t totalEventNumber = LAPPDDataTimeStampUL.size();
    ULong64_t gotOrphanCount_out = gotOrphanCount;
    ULong64_t gotMin_mean_dev_noOrphan_out = gotMin_mean_dev_noOrphan;
    ULong64_t increament_times_out = increament_times;
    ULong64_t min_mean_dev_out = min_mean_dev;
    ULong64_t final_i_out = final_i;
    ULong64_t final_j_out = final_j;
    ULong64_t drift_out = drift;

    vector<ULong64_t> FitInfo = {final_offset_ns, final_offset_ps_negative, gotOrphanCount_out, gotMin_mean_dev_noOrphan_out, increament_times_out, min_mean_dev_out, final_i_out, final_j_out, totalEventNumber, drift_out};

    vector<vector<ULong64_t>> Result = {FitInfo, TimeStampRaw, BeamGateRaw, TimeStamp_ns, BeamGate_ns, TimeStamp_ps, BeamGate_ps, EventIndex, EventDeviation_ns, CTCTriggerIndex, CTCTriggerTimeStamp_ns, BeamGate_correction_tick, TimeStamp_correction_tick, LAPPD_PPS_interval_ticks, BG_PPSBefore, BG_PPSAfter, BG_PPSDiff, BG_PPSMiss, TS_PPSBefore, TS_PPSAfter, TS_PPSDiff, TS_PPSMiss, TS_driftCorrection_ns, BG_driftCorrection_ns};
    return Result;
}

vector<vector<ULong64_t>> fitInPartFile(TTree *lappdTree, TTree *triggerTree, int runNumber, int subRunNumber, int partFileNumber, int LAPPD_ID, int fitTargetTriggerWord, bool triggerGrouped, int intervalInSecond)
{
    cout << "***************************************" << endl;
    cout << "Fitting in run " << runNumber << ", sub run " << subRunNumber << ", part file " << partFileNumber << " for LAPPD ID " << LAPPD_ID << endl;

    vector<ULong64_t> LAPPD_PPS0;
    vector<ULong64_t> LAPPD_PPS1;
    vector<ULong64_t> LAPPDDataTimeStampUL;
    vector<ULong64_t> LAPPDDataBeamgateUL;

    vector<ULong64_t> CTCTargetTimeStamp;
    vector<ULong64_t> CTCPPSTimeStamp;

    int LAPPD_ID_inTree;
    int runNumber_inTree;
    int subRunNumber_inTree;
    int partFileNumber_inTree;

    ULong64_t ppsTime0;
    ULong64_t ppsTime1;
    ULong64_t LAPPDTimeStampUL;
    ULong64_t LAPPDBeamgateUL;
    ULong64_t LAPPDDataTimestamp;
    ULong64_t LAPPDDataBeamgate;
    double LAPPDDataTimestampFloat;
    double LAPPDDataBeamgateFloat;

    vector<uint32_t> *CTCTriggerWord = nullptr;
    ULong64_t CTCTimeStamp;
    vector<uint32_t> *groupedTriggerWords = nullptr;
    vector<ULong64_t> *groupedTriggerTimestamps = nullptr;

    lappdTree->SetBranchAddress("RunNumber", &runNumber_inTree);
    lappdTree->SetBranchAddress("SubRunNumber", &subRunNumber_inTree);
    lappdTree->SetBranchAddress("PartFileNumber", &partFileNumber_inTree);
    lappdTree->SetBranchAddress("LAPPD_ID", &LAPPD_ID_inTree);
    lappdTree->SetBranchAddress("LAPPDDataTimeStampUL", &LAPPDTimeStampUL);
    lappdTree->SetBranchAddress("LAPPDDataBeamgateUL", &LAPPDBeamgateUL);
    lappdTree->SetBranchAddress("LAPPDDataTimestamp", &LAPPDDataTimestamp);
    lappdTree->SetBranchAddress("LAPPDDataBeamgate", &LAPPDDataBeamgate);
    lappdTree->SetBranchAddress("LAPPDDataTimestampFloat", &LAPPDDataTimestampFloat);
    lappdTree->SetBranchAddress("LAPPDDataBeamgateFloat", &LAPPDDataBeamgateFloat);
    lappdTree->SetBranchAddress("ppsTime0", &ppsTime0);
    lappdTree->SetBranchAddress("ppsTime1", &ppsTime1);

    // triggerTree->Print();
    triggerTree->SetBranchAddress("RunNumber", &runNumber_inTree);
    triggerTree->SetBranchAddress("SubRunNumber", &subRunNumber_inTree);
    triggerTree->SetBranchAddress("PartFileNumber", &partFileNumber_inTree);
    if (triggerGrouped)
    {
        triggerTree->SetBranchAddress("gTrigWord", &groupedTriggerWords);
        triggerTree->SetBranchAddress("gTrigTime", &groupedTriggerTimestamps);
    }
    else
    {
        triggerTree->SetBranchAddress("CTCTriggerWord", &CTCTriggerWord);
        triggerTree->SetBranchAddress("CTCTimeStamp", &CTCTimeStamp);
    }

    int l_nEntries = lappdTree->GetEntries();
    int t_nEntries = triggerTree->GetEntries();
    int repeatedPPSNumber0 = -1;
    int repeatedPPSNumber1 = -1;

    // 5. loop TimeStamp tree
    for (int i = 0; i < l_nEntries; i++)
    {
        lappdTree->GetEntry(i);
        // if(i<100)
        //     cout<<"LAPPD_ID_inTree: "<<LAPPD_ID_inTree<<", runNumber_inTree: "<<runNumber_inTree<<", subRunNumber_inTree: "<<subRunNumber_inTree<<", partFileNumber_inTree: "<<partFileNumber_inTree<<endl;
        //     cout<<"LAPPD_ID: "<<LAPPD_ID<<", runNumber: "<<runNumber<<", subRunNumber: "<<subRunNumber<<", partFileNumber: "<<partFileNumber<<endl;
        if (LAPPD_ID_inTree == LAPPD_ID && runNumber_inTree == runNumber && subRunNumber_inTree == subRunNumber && partFileNumber_inTree == partFileNumber)
        {
            if (LAPPDTimeStampUL != 0)
            {
                cout << "In unit of 1ps, after conversion and saving, LAPPDTimeStampUL: " << LAPPDTimeStampUL * 1000 / 8 * 25 << ", LAPPDBeamgateUL: " << LAPPDBeamgateUL * 1000 / 8 * 25 << endl;
                cout << "In second, use double, Timestamp: " << static_cast<double>(LAPPDTimeStampUL * 1000 / 8 * 25) / 1E9 / 1000 << ", Beamgate: " << static_cast<double>(LAPPDBeamgateUL * 1000 / 8 * 25) / 1E9 / 1000 << endl;
                LAPPDDataTimeStampUL.push_back(LAPPDTimeStampUL * 1000 / 8 * 25);
                LAPPDDataBeamgateUL.push_back(LAPPDBeamgateUL * 1000 / 8 * 25);
                /*
                cout << "LAPPDDataTimestamp: " << LAPPDDataTimestamp << ", LAPPDDataBeamgate: " << LAPPDDataBeamgate << endl;
                cout << "LAPPDDataTimestampFloat: " << LAPPDDataTimestampFloat << ", LAPPDDataBeamgateFloat: " << LAPPDDataBeamgateFloat << endl;
                ULong64_t ULTS = LAPPDTimeStampUL*1000/8*25;
                double DTS = static_cast<double>(LAPPDDataTimestamp);
                double plusDTS = DTS + LAPPDDataTimestampFloat;
                ULong64_t ULBG = LAPPDBeamgateUL*1000/8*25;
                double DBG = static_cast<double>(LAPPDDataBeamgate);
                double plusDBG = DBG + LAPPDDataBeamgateFloat;
                cout << std::fixed << " ULTS: " << ULTS << ", DTS: " << DTS << ", plusDTS: " << plusDTS << endl;
                cout << std::fixed << " ULBG: " << ULBG << ", DBG: " << DBG << ", plusDBG: " << plusDBG << endl;
                cout << endl;
                */
            }
            else if (LAPPDTimeStampUL == 0)
            {
                // cout << "ppsTime0: " << ppsTime0 << ", ppsTime1: " << ppsTime1 << endl;
                if (LAPPD_PPS0.size() == 0)
                    LAPPD_PPS0.push_back(ppsTime0 * 1000 / 8 * 25);
                if (LAPPD_PPS1.size() == 0)
                    LAPPD_PPS1.push_back(ppsTime1 * 1000 / 8 * 25);
                if (LAPPD_PPS0.size() > 0 && ppsTime0 * 1000 / 8 * 25 != LAPPD_PPS0.at(LAPPD_PPS0.size() - 1))
                    LAPPD_PPS0.push_back(ppsTime0 * 1000 / 8 * 25);
                else
                    repeatedPPSNumber0 += 1;
                if (LAPPD_PPS1.size() > 0 && ppsTime1 * 1000 / 8 * 25 != LAPPD_PPS1.at(LAPPD_PPS1.size() - 1))
                    LAPPD_PPS1.push_back(ppsTime1 * 1000 / 8 * 25);
                else
                    repeatedPPSNumber1 += 1;
            }
        }
    }
    cout << "repeatedPPSNumber0: " << repeatedPPSNumber0 << ", loaded PPS0 size: " << LAPPD_PPS0.size() << endl;
    cout << "repeatedPPSNumber1: " << repeatedPPSNumber1 << ", loaded PPS1 size: " << LAPPD_PPS1.size() << endl;

    // 6. loop Trig (or GTrig) tree
    for (int i = 0; i < t_nEntries; i++)
    {
        triggerTree->GetEntry(i);
        if (runNumber_inTree == runNumber && subRunNumber_inTree == subRunNumber && partFileNumber_inTree == partFileNumber)
        {
            // cout<<"triggerGrouped: "<<triggerGrouped<<", fitTargetTriggerWord: "<<fitTargetTriggerWord<<endl;
            if (triggerGrouped)
            {
                for (int j = 0; j < groupedTriggerWords->size(); j++)
                {
                    // cout<<"At j = "<<j<<",finding groupedTriggerWords: "<<groupedTriggerWords->at(j)<<", fitTargetTriggerWord: "<<fitTargetTriggerWord<<endl;
                    if (groupedTriggerWords->at(j) == fitTargetTriggerWord)
                        CTCTargetTimeStamp.push_back(groupedTriggerTimestamps->at(j));
                    if (groupedTriggerWords->at(j) == 32)
                        CTCPPSTimeStamp.push_back(groupedTriggerTimestamps->at(j));
                }
            }
            else
            {
                for (int j = 0; j < CTCTriggerWord->size(); j++)
                {
                    if (CTCTriggerWord->at(j) == fitTargetTriggerWord)
                        CTCTargetTimeStamp.push_back(CTCTimeStamp);
                    if (CTCTriggerWord->at(j) == 32)
                        CTCPPSTimeStamp.push_back(CTCTimeStamp);
                }
            }
        }
    }
    cout << "Vector for partfile " << partFileNumber << " for LAPPD ID " << LAPPD_ID << " loaded." << endl;
    cout << "LAPPDDataTimeStampUL in ps size: " << LAPPDDataTimeStampUL.size() << endl;
    cout << "LAPPDDataBeamgateUL in ps size: " << LAPPDDataBeamgateUL.size() << endl;
    cout << "LAPPD_PPS0 size: " << LAPPD_PPS0.size() << endl;
    cout << "LAPPD_PPS1 size: " << LAPPD_PPS1.size() << endl;
    cout << "CTCTargetTimeStamp size: " << CTCTargetTimeStamp.size() << endl;
    cout << "CTCPPSTimeStamp size: " << CTCPPSTimeStamp.size() << endl;
    // 7. Find the number of resets in LAPPD PPS:
    int LAPPDDataFitStopIndex = 0;
    int resetNumber = 0;
    // first check is there a reset, if no, set the LAPPDDataFitStopIndex to the size of LAPPDDataTimeStampUL
    // if all PPS in this part file was increamented, then there is no reset
    for (int i = 1; i < LAPPD_PPS0.size(); i++)
    {
        if (LAPPD_PPS0[i] < LAPPD_PPS0[i - 1])
        {
            resetNumber += 1;
            cout << "For LAPPD ID " << LAPPD_ID << ", run number " << runNumber << ", sub run number " << subRunNumber << ", part file number " << partFileNumber << ", reset "
                 << " found at PPS_ACDC0 index " << i << endl;
            break;
        }
    }
    for (int i = 1; i < LAPPD_PPS1.size(); i++)
    {
        if (LAPPD_PPS1[i] < LAPPD_PPS1[i - 1])
        {
            resetNumber += 1;
            cout << "For LAPPD ID " << LAPPD_ID << ", run number " << runNumber << ", sub run number " << subRunNumber << ", part file number " << partFileNumber << ", reset "
                 << " found at PPS_ACDC1 index " << i << endl;
            break;
        }
    }
    if (resetNumber == 0)
        cout << "For LAPPD ID " << LAPPD_ID << ", run number " << runNumber << ", sub run number " << subRunNumber << ", part file number " << partFileNumber << ", no reset found." << endl;

    // if reset found, loop the timestamp in order to find the LAPPDDataFitStopIndex
    //
    if (resetNumber != 0)
    {
        for (int i = 1; i < LAPPDDataTimeStampUL.size(); i++)
        {
            if (LAPPDDataTimeStampUL[i] < LAPPDDataTimeStampUL[i - 1])
            {
                LAPPDDataFitStopIndex = i - 1;
                break;
            }
        }
        // TODO: extend this to later offsets
    }
    // 8. Use the target trigger word, fit the offset
    // TODO: fit for each reset
    vector<vector<ULong64_t>> ResultTotal;
    if (LAPPDDataTimeStampUL.size() == LAPPDDataBeamgateUL.size())
    {
        if (LAPPD_PPS0.size() == 0 || LAPPD_PPS1.size() == 0)
        {
            cout << "Error: PPS0 or PPS1 is empty, return empty result." << endl;
            return ResultTotal;
        }
        vector<vector<ULong64_t>> ResultACDC0 = fitInThisReset(LAPPDDataTimeStampUL, LAPPDDataBeamgateUL, LAPPD_PPS0, fitTargetTriggerWord, CTCTargetTimeStamp, CTCPPSTimeStamp, intervalInSecond * 1E9 * 1000);
        vector<vector<ULong64_t>> ResultACDC1 = fitInThisReset(LAPPDDataTimeStampUL, LAPPDDataBeamgateUL, LAPPD_PPS1, fitTargetTriggerWord, CTCTargetTimeStamp, CTCPPSTimeStamp, intervalInSecond * 1E9 * 1000);

        // 9. save the offset for this LAPPD ID, run number, part file number, index, reset number.

        cout << "Fitting in part file " << partFileNumber << " for LAPPD ID " << LAPPD_ID << " done." << endl;

        // Combine ResultACDC0 and ResultACDC1 to ResultTotal
        for (int i = 0; i < ResultACDC0.size(); i++)
        {
            ResultTotal.push_back(ResultACDC0[i]);
        }
        for (int i = 0; i < ResultACDC1.size(); i++)
        {
            ResultTotal.push_back(ResultACDC1[i]);
        }
    }
    return ResultTotal;
}

void offsetFit_MultipleLAPPD(string fileName, int fitTargetTriggerWord, bool triggerGrouped, int intervalInSecond, int processPFNumber)
{
    // 1. load LAPPDTree.root
    const string file = fileName;
    TFile *f = new TFile(file.c_str(), "READ");
    if (!f->IsOpen())
    {
        std::cerr << "Error: cannot open file " << file << std::endl;
        return;
    }

    std::cout << "Opened file " << file << std::endl;

    std::ofstream outputOffset("offset.txt");

    outputOffset << "runNumber"
                 << "\t"
                 << "subRunNumber"
                 << "\t"
                 << "partFileNumber"
                 << "\t"
                 << "resetNumber"
                 << "\t"
                 << "LAPPD_ID"
                 << "\t"
                 << "offset_ACDC0_ns"
                 << "\t"
                 << "offset_ACDC1_ns"
                 << "\t"
                 << "offset_ACDC0_ps_negative"
                 << "\t"
                 << "offset_ACDC1_ps_negative"
                 << "\t"
                 << "gotOrphanCount_ACDC0"
                 << "\t"
                 << "gotOrphanCount_ACDC1"
                 << "\t"
                 << "EventNumInThisPartFile"
                 << "\t"
                 << "min_mean_dev_noOrphan_ACDC0"
                 << "\t"
                 << "min_mean_dev_noOrphan_ACDC1"
                 << "\t"
                 << "increament_times_ACDC0"
                 << "\t"
                 << "increament_times_ACDC1"
                 << "\t"
                 << "min_mean_dev_ACDC0"
                 << "\t"
                 << "min_mean_dev_ACDC1"
                 << std::endl;

    // 2. load TimeStamp tree, get run number and part file number
    TTree *TSTree = (TTree *)f->Get("TimeStamp");
    TTree *CTCTree = (TTree *)f->Get("Trig");
    TTree *GCTCTree = (TTree *)f->Get("GTrig");
    int runNumber;
    int subRunNumber;
    int partFileNumber;
    int LAPPD_ID;
    ULong64_t ppsCount0;
    TSTree->SetBranchAddress("RunNumber", &runNumber);
    TSTree->SetBranchAddress("SubRunNumber", &subRunNumber);
    TSTree->SetBranchAddress("PartFileNumber", &partFileNumber);
    TSTree->SetBranchAddress("LAPPD_ID", &LAPPD_ID);
    TSTree->SetBranchAddress("ppsCount0", &ppsCount0);
    std::vector<vector<int>> loopInfo;
    int nEntries = TSTree->GetEntries();
    for (int i = 0; i < nEntries; i++)
    {
        TSTree->GetEntry(i);
        // if this entry is PPS event, continue.
        if (ppsCount0 != 0)
            continue;
        vector<int> info = {runNumber, subRunNumber, partFileNumber, LAPPD_ID};
        // find if this info is already in loopInfo
        bool found = std::find_if(loopInfo.begin(), loopInfo.end(), [&info](const std::vector<int> &vec)
                                  {
                                      return vec == info; // compare vec and info
                                  }) != loopInfo.end();   // if find_if doesn't return end，found

        if (!found)
            loopInfo.push_back(info);
    }

    // 3. for each unique run number and part file number
    // 4. in this file, for each LAPPD ID
    // do fit:
    std::map<string, vector<vector<ULong64_t>>> ResultMap;
    std::vector<vector<int>>::iterator it;
    int pfNumber = 0;
    for (it = loopInfo.begin(); it != loopInfo.end(); it++)
    {
        int runNumber = (*it)[0];
        int subRunNumber = (*it)[1];
        int partFileNumber = (*it)[2];
        int LAPPD_ID = (*it)[3];
        if (processPFNumber != 0)
        {
            if (pfNumber >= processPFNumber)
                break;
        }

        vector<vector<ULong64_t>> Result;
        if (!triggerGrouped)
        { // cout<<"trigger not grouped"<<endl;
            Result = fitInPartFile(TSTree, CTCTree, runNumber, subRunNumber, partFileNumber, LAPPD_ID, fitTargetTriggerWord, triggerGrouped, intervalInSecond);
        }
        else

        { // cout<<"trigger grouped"<<endl;
            Result = fitInPartFile(TSTree, GCTCTree, runNumber, subRunNumber, partFileNumber, LAPPD_ID, fitTargetTriggerWord, triggerGrouped, intervalInSecond);
        }
        // Combine the *it to a string, and save the result to ResultMap
        string key = std::to_string(runNumber) + "_" + std::to_string(subRunNumber) + "_" + std::to_string(partFileNumber) + "_" + std::to_string(LAPPD_ID);
        ResultMap[key] = Result;
        pfNumber++;
    }

    /*
        vector<vector<ULong64_t>> Result = {FitInfo, TimeStampRaw, BeamGateRaw, TimeStamp_ns, BeamGate_ns, TimeStamp_ps, BeamGate_ps, EventIndex, EventDeviation_ns, CTCTriggerIndex, CTCTriggerTimeStamp_ns,BeamGate_correction_tick,TimeStamp_correction_tick};
        vector<ULong64_t> FitInfo = {final_offset_ns, final_offset_ps_negative, gotOrphanCount, gotMin_mean_dev_noOrphan, increament_times, min_mean_dev, final_i, final_j, totalEventNumber};
        vector<ULong64_t> TimeStampRaw;
        vector<ULong64_t> BeamGateRaw;
        vector<ULong64_t> TimeStamp_ns;
        vector<ULong64_t> BeamGate_ns;
        vector<ULong64_t> TimeStamp_ps;
        vector<ULong64_t> BeamGate_ps;
        vector<ULong64_t> EventIndex;
        vector<ULong64_t> EventDeviation_ns;
        vector<ULong64_t> CTCTriggerIndex;
        vector<ULong64_t> CTCTriggerTimeStamp_ns;
        vector<ULong64_t> BeamGate_correction_tick;
        vector<ULong64_t> TimeStamp_correction_tick;
    */
    // Loop the ResultMap, save the result to a root tree in a root file
    cout << "Start saving the result to root file and txt file..." << endl;
    TFile *fOut = new TFile("offsetFitResult.root", "RECREATE");
    TTree *tOut = new TTree("Events", "Events");
    int runNumber_out;
    int subRunNumber_out;
    int partFileNumber_out;
    int LAPPD_ID_out;
    ULong64_t EventIndex;
    ULong64_t EventNumberInThisPartFile;
    ULong64_t final_offset_ns_0;
    ULong64_t final_offset_ns_1;
    ULong64_t final_offset_ps_negative_0;
    ULong64_t final_offset_ps_negative_1;
    double drift0;
    double drift1;
    ULong64_t gotOrphanCount_0;
    ULong64_t gotOrphanCount_1;
    ULong64_t gotMin_mean_dev_noOrphan_0;
    ULong64_t gotMin_mean_dev_noOrphan_1;
    ULong64_t increament_times_0;
    ULong64_t increament_times_1;
    ULong64_t min_mean_dev_0;
    ULong64_t min_mean_dev_1;
    ULong64_t TimeStampRaw;
    ULong64_t BeamGateRaw;
    ULong64_t TimeStamp_ns;
    ULong64_t BeamGate_ns;
    ULong64_t TimeStamp_ps;
    ULong64_t BeamGate_ps;
    ULong64_t EventDeviation_ns_0;
    ULong64_t EventDeviation_ns_1;
    ULong64_t CTCTriggerIndex;
    ULong64_t CTCTriggerTimeStamp_ns;
    long long BGMinusTrigger_ns;
    long long BGCorrection_tick;
    long long TSCorrection_tick;
    ULong64_t LAPPD_PPS_interval_ticks;
    ULong64_t BG_PPSBefore_tick;
    ULong64_t BG_PPSAfter_tick;
    ULong64_t BG_PPSDiff_tick;
    ULong64_t BG_PPSMissing_tick;
    ULong64_t TS_PPSBefore_tick;
    ULong64_t TS_PPSAfter_tick;
    ULong64_t TS_PPSDiff_tick;
    ULong64_t TS_PPSMissing_tick;

    ULong64_t TS_driftCorrection_ns;
    ULong64_t BG_driftCorrection_ns;

    tOut->Branch("runNumber", &runNumber_out, "runNumber/I");
    tOut->Branch("subRunNumber", &subRunNumber_out, "subRunNumber/I");
    tOut->Branch("partFileNumber", &partFileNumber_out, "partFileNumber/I");
    tOut->Branch("LAPPD_ID", &LAPPD_ID_out, "LAPPD_ID/I");
    tOut->Branch("EventIndex", &EventIndex, "EventIndex/l");
    tOut->Branch("EventNumInThisPF", &EventNumberInThisPartFile, "EventNumInThisPF/l");
    tOut->Branch("final_offset_ns_0", &final_offset_ns_0, "final_offset_ns_0/l");
    tOut->Branch("final_offset_ns_1", &final_offset_ns_1, "final_offset_ns_1/l");
    tOut->Branch("final_offset_ps_negative_0", &final_offset_ps_negative_0, "final_offset_ps_negative_0/l");
    tOut->Branch("final_offset_ps_negative_1", &final_offset_ps_negative_1, "final_offset_ps_negative_1/l");
    tOut->Branch("drift0", &drift0, "drift0/D");
    tOut->Branch("drift1", &drift1, "drift1/D");
    tOut->Branch("gotOrphanCount_0", &gotOrphanCount_0, "gotOrphanCount_0/l");
    tOut->Branch("gotOrphanCount_1", &gotOrphanCount_1, "gotOrphanCount_1/l");
    tOut->Branch("gotMin_mean_dev_noOrphan_0", &gotMin_mean_dev_noOrphan_0, "gotMin_mean_dev_noOrphan_0/l");
    tOut->Branch("gotMin_mean_dev_noOrphan_1", &gotMin_mean_dev_noOrphan_1, "gotMin_mean_dev_noOrphan_1/l");
    tOut->Branch("increament_times_0", &increament_times_0, "increament_times_0/l");
    tOut->Branch("increament_times_1", &increament_times_1, "increament_times_1/l");
    tOut->Branch("min_mean_dev_0", &min_mean_dev_0, "min_mean_dev_0/l");
    tOut->Branch("min_mean_dev_1", &min_mean_dev_1, "min_mean_dev_1/l");
    tOut->Branch("TimeStampRaw", &TimeStampRaw, "TimeStampRaw/l");
    tOut->Branch("BeamGateRaw", &BeamGateRaw, "BeamGateRaw/l");
    tOut->Branch("TimeStamp_ns", &TimeStamp_ns, "TimeStamp_ns/l");
    tOut->Branch("BeamGate_ns", &BeamGate_ns, "BeamGate_ns/l");
    tOut->Branch("TimeStamp_ps", &TimeStamp_ps, "TimeStamp_ps/l");
    tOut->Branch("BeamGate_ps", &BeamGate_ps, "BeamGate_ps/l");
    tOut->Branch("EventDeviation_ns_0", &EventDeviation_ns_0, "EventDeviation_ns_0/l");
    tOut->Branch("EventDeviation_ns_1", &EventDeviation_ns_1, "EventDeviation_ns_1/l");
    tOut->Branch("CTCTriggerIndex", &CTCTriggerIndex, "CTCTriggerIndex/l");
    tOut->Branch("CTCTriggerTimeStamp_ns", &CTCTriggerTimeStamp_ns, "CTCTriggerTimeStamp_ns/l");
    tOut->Branch("BGMinusTrigger_ns", &BGMinusTrigger_ns, "BGMinusTrigger_ns/L");
    tOut->Branch("BGCorrection_tick", &BGCorrection_tick, "BGCorrection_tick/l");
    tOut->Branch("TSCorrection_tick", &TSCorrection_tick, "TSCorrection_tick/l");
    tOut->Branch("LAPPD_PPS_interval_ticks", &LAPPD_PPS_interval_ticks, "LAPPD_PPS_interval_ticks/l");
    tOut->Branch("BG_PPSBefore_tick", &BG_PPSBefore_tick, "BG_PPSBefore_tick/l");
    tOut->Branch("BG_PPSAfter_tick", &BG_PPSAfter_tick, "BG_PPSAfter_tick/l");
    tOut->Branch("BG_PPSDiff_tick", &BG_PPSDiff_tick, "BG_PPSDiff_tick/l");
    tOut->Branch("BG_PPSMissing_tick", &BG_PPSMissing_tick, "BG_PPSMissing_tick/l");
    tOut->Branch("TS_PPSBefore_tick", &TS_PPSBefore_tick, "TS_PPSBefore_tick/l");
    tOut->Branch("TS_PPSAfter_tick", &TS_PPSAfter_tick, "TS_PPSAfter_tick/l");
    tOut->Branch("TS_PPSDiff_tick", &TS_PPSDiff_tick, "TS_PPSDiff_tick/l");
    tOut->Branch("TS_PPSMissing_tick", &TS_PPSMissing_tick, "TS_PPSMissing_tick/l");
    tOut->Branch("TS_driftCorrection_ns", &TS_driftCorrection_ns, "TS_driftCorrection_ns/l");
    tOut->Branch("BG_driftCorrection_ns", &BG_driftCorrection_ns, "BG_driftCorrection_ns/l");
    
    std::ofstream outputEvents("outputEvents.txt");
    for (auto it = ResultMap.begin(); it != ResultMap.end(); it++)
    {
        string key = it->first;
        vector<vector<ULong64_t>> Result = it->second;

        if (Result.size() == 0)
            continue;

        runNumber_out = std::stoi(key.substr(0, key.find("_")));
        subRunNumber_out = std::stoi(key.substr(key.find("_") + 1, key.find("_", key.find("_") + 1) - key.find("_") - 1));
        partFileNumber_out = std::stoi(key.substr(key.find("_", key.find("_") + 1) + 1, key.find("_", key.find("_", key.find("_") + 1) + 1) - key.find("_", key.find("_") + 1) - 1));
        LAPPD_ID_out = std::stoi(key.substr(key.find("_", key.find("_", key.find("_") + 1) + 1) + 1, key.size() - key.find("_", key.find("_", key.find("_") + 1) + 1) - 1));
        final_offset_ns_0 = Result[0][0];
        final_offset_ns_1 = Result[24][0];
        final_offset_ps_negative_0 = Result[0][1];
        final_offset_ps_negative_1 = Result[24][1];
        gotOrphanCount_0 = Result[0][2];
        gotOrphanCount_1 = Result[24][2];
        gotMin_mean_dev_noOrphan_0 = Result[0][3];
        gotMin_mean_dev_noOrphan_1 = Result[24][3];
        increament_times_0 = Result[0][4];
        increament_times_1 = Result[24][4];
        min_mean_dev_0 = Result[0][5];
        min_mean_dev_1 = Result[24][5];
        EventNumberInThisPartFile = Result[0][8];
        drift0 = Result[0][9];
        drift1 = Result[24][9];

        //                                      0           1           2           3           4               5               6           7           8                    9           10                     11                          12                          13                      14              15          16          17          18          19              20          21
        // vector<vector<ULong64_t>> Result = {FitInfo, TimeStampRaw, BeamGateRaw, TimeStamp_ns, BeamGate_ns, TimeStamp_ps, BeamGate_ps, EventIndex, EventDeviation_ns, CTCTriggerIndex, CTCTriggerTimeStamp_ns, BeamGate_correction_tick, TimeStamp_correction_tick, LAPPD_PPS_interval_ticks, BG_PPSBefore, BG_PPSAfter, BG_PPSDiff, BG_PPSMiss, TS_PPSBefore, TS_PPSAfter, TS_PPSDiff, TS_PPSMiss};
        // any Result[x] , if x>13, x = x + 8
        for (int j = 0; j < Result[1].size(); j++)
        {
            long long BGTdiff = Result[4][j] - Result[10][j] - 325250;
            // cout<<"BGTDiff: "<<BGTdiff<<endl;
            // cout<<"Saving BeamGate_ns = "<<Result[4][j]<<", CTCTriggerTimeStamp_ns = "<<Result[10][j]<<", with BG-T-325250= "<<BGTdiff<<", at partFileNumber "<<partFileNumber_out<<", EventIndex = "<<Result[7][j]<<", j = "<<j<<endl;
            outputEvents << fixed << Result[3][j] << " " << Result[5][j] << " " << Result[4][j] << " " << Result[6][j] << " " << Result[10][j] << " " << BGTdiff << " " << partFileNumber_out << " " << Result[7][j] << " " << Result[11][j] << " " << Result[12][j] << " " << Result[13][j] << " " << Result[22][j] << " " << Result[23][j] << endl;
            EventIndex = Result[7][j];
            TimeStampRaw = Result[1][j];
            BeamGateRaw = Result[2][j];
            TimeStamp_ns = Result[3][j];
            BeamGate_ns = Result[4][j];
            TimeStamp_ps = Result[5][j];
            BeamGate_ps = Result[6][j];
            CTCTriggerIndex = Result[9][j];
            CTCTriggerTimeStamp_ns = Result[10][j];
            EventDeviation_ns_0 = Result[8][j];
            EventDeviation_ns_1 = Result[28][j];
            BGMinusTrigger_ns = BGTdiff;
            BGCorrection_tick = Result[11][j];
            TSCorrection_tick = Result[12][j];
            LAPPD_PPS_interval_ticks = Result[13][j];
            BG_PPSBefore_tick = Result[14][j];
            BG_PPSAfter_tick = Result[15][j];
            BG_PPSDiff_tick = Result[16][j];
            BG_PPSMissing_tick = Result[17][j];
            TS_PPSBefore_tick = Result[18][j];
            TS_PPSAfter_tick = Result[19][j];
            TS_PPSDiff_tick = Result[20][j];
            TS_PPSMissing_tick = Result[21][j];
            TS_driftCorrection_ns = Result[22][j];
            BG_driftCorrection_ns = Result[23][j];
            tOut->Fill();
        }
        outputOffset << runNumber_out << "\t" << subRunNumber_out << "\t" << partFileNumber_out << "\t" << 0 << "\t" << LAPPD_ID_out << "\t" << final_offset_ns_0 << "\t" << final_offset_ns_1 << "\t" << final_offset_ps_negative_0 << "\t" << final_offset_ps_negative_1 << "\t" << gotOrphanCount_0 << "\t" << gotOrphanCount_1 << "\t" << EventNumberInThisPartFile << "\t" << gotMin_mean_dev_noOrphan_0 << "\t" << gotMin_mean_dev_noOrphan_1 << "\t" << increament_times_0 << "\t" << increament_times_1 << "\t" << min_mean_dev_0 << "\t" << min_mean_dev_1 << std::endl;
    }
    outputOffset.close();
    fOut->cd();
    tOut->Write();
    fOut->Close();
    cout << "Result saved." << endl;
}
