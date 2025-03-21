/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manuel Requena <manuel.requena@cttc.es>
 *         Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef LTE_TEST_INTERFERENCE_H
#define LTE_TEST_INTERFERENCE_H

#include "ns3/lte-common.h"
#include "ns3/test.h"

using namespace ns3;

/**
 * @ingroup lte-test
 *
 * @brief Test suite for interference test.
 */
class LteInterferenceTestSuite : public TestSuite
{
  public:
    LteInterferenceTestSuite();
};

/**
 * @ingroup lte-test
 *
 * @brief Test that SINR calculation and MCS selection works fine in a
 * multi-cell interference scenario.
 */
class LteInterferenceTestCase : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param name the reference name
     * @param d1 distance between ENB and UE
     * @param d2 distance between ENB and other UE
     * @param dlSinr the DL SINR
     * @param ulSinr the UL SINR
     * @param dlSe the DL se
     * @param ulSe the UL se
     * @param dlMcs the DL MCS
     * @param ulMcs the UL MCS
     */
    LteInterferenceTestCase(std::string name,
                            double d1,
                            double d2,
                            double dlSinr,
                            double ulSinr,
                            double dlSe,
                            double ulSe,
                            uint16_t dlMcs,
                            uint16_t ulMcs);
    ~LteInterferenceTestCase() override;

    /**
     * DL scheduling function
     * @param dlInfo the DL info
     */
    void DlScheduling(DlSchedulingCallbackInfo dlInfo);

    /**
     * UL scheduling function
     * @param frameNo the frame number
     * @param subframeNo the subframe number
     * @param rnti the RNTI
     * @param mcs the MCS
     * @param sizeTb
     */
    void UlScheduling(uint32_t frameNo,
                      uint32_t subframeNo,
                      uint16_t rnti,
                      uint8_t mcs,
                      uint16_t sizeTb);

  private:
    void DoRun() override;

    double m_d1;               ///< distance between UE and ENB
    double m_d2;               ///< distance between UE and other ENB
    double m_expectedDlSinrDb; ///< expected DL SINR in dB
    double m_expectedUlSinrDb; ///< expected UL SINR in dB
    uint16_t m_dlMcs;          ///< the DL MCS
    uint16_t m_ulMcs;          ///< the UL MCS
};

#endif /* LTE_TEST_INTERFERENCE_H */
