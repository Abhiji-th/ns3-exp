/*
 * Copyright (c) 2017 Kungliga Tekniska Högskolan
 *               2017 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * TBF, The Token Bucket Filter Queueing discipline
 *
 * This implementation is based on linux kernel code by
 * Authors:     Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru>
 *              Dmitry Torokhov <dtor@mail.ru> - allow attaching inner qdiscs -
 *                                               original idea by Martin Devera
 *
 * Implemented in ns-3 by: Surya Seetharaman <suryaseetharaman.9@gmail.com>
 *                         Stefano Avallone <stavallo@unina.it>
 */
#ifndef TBF_QUEUE_DISC_H
#define TBF_QUEUE_DISC_H

#include "queue-disc.h"

#include "ns3/boolean.h"
#include "ns3/data-rate.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "ns3/random-variable-stream.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/traced-value.h"

namespace ns3
{

/**
 * @ingroup traffic-control
 *
 * @brief A TBF packet queue disc
 */
class TbfQueueDisc : public QueueDisc
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief TbfQueueDisc Constructor
     *
     * Create a TBF queue disc
     */
    TbfQueueDisc();

    /**
     * @brief Destructor
     *
     * Destructor
     */
    ~TbfQueueDisc() override;

    /**
     * @brief Set the size of the first bucket in bytes.
     *
     * @param burst The size of first bucket in bytes.
     */
    void SetBurst(uint32_t burst);

    /**
     * @brief Get the size of the first bucket in bytes.
     *
     * @returns The size of the first bucket in bytes.
     */
    uint32_t GetBurst() const;

    /**
     * @brief Set the size of the second bucket in bytes.
     *
     * @param mtu The size of second bucket in bytes.
     */
    void SetMtu(uint32_t mtu);

    /**
     * @brief Get the size of the second bucket in bytes.
     *
     * @returns The size of the second bucket in bytes.
     */
    uint32_t GetMtu() const;

    /**
     * @brief Set the rate of the tokens entering the first bucket.
     *
     * @param rate The rate of first bucket tokens.
     */
    void SetRate(DataRate rate);

    /**
     * @brief Get the rate of the tokens entering the first bucket.
     *
     * @returns The rate of first bucket tokens.
     */
    DataRate GetRate() const;

    /**
     * @brief Set the rate of the tokens entering the second bucket.
     *
     * @param peakRate The rate of second bucket tokens.
     */
    void SetPeakRate(DataRate peakRate);

    /**
     * @brief Get the rate of the tokens entering the second bucket.
     *
     * @returns The rate of second bucket tokens.
     */
    DataRate GetPeakRate() const;

    /**
     * @brief Get the current number of tokens inside the first bucket in bytes.
     *
     * @returns The number of first bucket tokens in bytes.
     */
    uint32_t GetFirstBucketTokens() const;

    /**
     * @brief Get the current number of tokens inside the second bucket in bytes.
     *
     * @returns The number of second bucket tokens in bytes.
     */
    uint32_t GetSecondBucketTokens() const;

  protected:
    /**
     * @brief Dispose of the object
     */
    void DoDispose() override;

  private:
    bool DoEnqueue(Ptr<QueueDiscItem> item) override;
    Ptr<QueueDiscItem> DoDequeue() override;
    bool CheckConfig() override;
    void InitializeParams() override;

    /* parameters for the TBF Queue Disc */
    uint32_t m_burst;    //!< Size of first bucket in bytes
    uint32_t m_mtu;      //!< Size of second bucket in bytes
    DataRate m_rate;     //!< Rate at which tokens enter the first bucket
    DataRate m_peakRate; //!< Rate at which tokens enter the second bucket

    /* variables stored by TBF Queue Disc */
    TracedValue<uint32_t> m_btokens; //!< Current number of tokens in first bucket
    TracedValue<uint32_t> m_ptokens; //!< Current number of tokens in second bucket
    Time m_timeCheckPoint;           //!< Time check-point
    EventId m_id; //!< EventId of the scheduled queue waking event when enough tokens are available
};

} // namespace ns3

#endif /* TBF_QUEUE_DISC_H */
