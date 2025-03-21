/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Leonard Tracy <lentracy@gmail.com>
 */

#ifndef UAN_TRANSDUCER_H
#define UAN_TRANSDUCER_H

#include "uan-prop-model.h"
#include "uan-tx-mode.h"

#include "ns3/object.h"
#include "ns3/packet.h"

#include <list>

namespace ns3
{

class UanPhy;
class UanChannel;

/**
 * @ingroup uan
 *
 * Class consisting of packet arrival information (Time, RxPower, mode, PDP).
 */
class UanPacketArrival
{
  public:
    /** Default constructor. */
    UanPacketArrival()
    {
    }

    /**
     * Constructor.
     *
     * @param packet Packet arriving.
     * @param rxPowerDb RX signal power in dB of arriving packet.
     * @param txMode TX mode of arriving packet.
     * @param pdp Power delay profile of arriving packet.
     * @param arrTime Arrival time of packet.
     */
    UanPacketArrival(Ptr<Packet> packet,
                     double rxPowerDb,
                     UanTxMode txMode,
                     UanPdp pdp,
                     Time arrTime)
        : m_packet(packet),
          m_rxPowerDb(rxPowerDb),
          m_txMode(txMode),
          m_pdp(pdp),
          m_arrTime(arrTime)
    {
    }

    /** Destructor */
    ~UanPacketArrival()
    {
        m_packet = nullptr;
    }

    /**
     * Get the arriving packet.
     *
     * @return Pointer to packet.
     */
    inline Ptr<Packet> GetPacket() const
    {
        return m_packet;
    }

    /**
     * Get the received signal strength.
     *
     * @return Received signal strength in dB re 1uPa
     */
    inline double GetRxPowerDb() const
    {
        return m_rxPowerDb;
    }

    /**
     * Get the transmission mode of the packet.
     *
     * @return UanTxMode.
     */
    inline const UanTxMode& GetTxMode() const
    {
        return m_txMode;
    }

    /**
     * Get the packet arrival time.
     *
     * @return Arrival time.
     */
    inline Time GetArrivalTime() const
    {
        return m_arrTime;
    }

    /**
     * Get the propagation delay profile.
     *
     * @return PDP of arriving signal.
     */
    inline UanPdp GetPdp() const
    {
        return m_pdp;
    }

  private:
    Ptr<Packet> m_packet; //!< The arrived packet.
    double m_rxPowerDb;   //!< The received power, in dB.
    UanTxMode m_txMode;   //!< The transmission mode.
    UanPdp m_pdp;         //!< The propagation delay profile.
    Time m_arrTime;       //!< The arrival time.

    // end of class UanPacketArrival
};

/**
 * @ingroup uan
 *
 * Virtual base for Transducer objects.
 *
 * The Transducer was added to support classes such as UanPhyDual.
 * In a generic Phy setting, this class functions to hold information about all
 * possibly interfering packets.
 */
class UanTransducer : public Object
{
  public:
    /**
     * Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    /** Transducer state. */
    enum State
    {
        TX, //!< Transmitting.
        RX  //!< Receiving.
    };

    /** List of arriving packets overlapping in time. */
    typedef std::list<UanPacketArrival> ArrivalList;
    /** List of UanPhy objects. */
    typedef std::list<Ptr<UanPhy>> UanPhyList;

    /**
     * Get the transducer state.
     *
     * @return State (TX or RX) of this transducer.
     */
    virtual State GetState() const = 0;

    /**
     * Is the state receiving (or available for reception)?
     *
     * @return True if this transducer is available for receiving
     *   an incoming packet.
     */
    virtual bool IsRx() const = 0;
    /**
     * Is the state transmitting?
     *
     * @return True if there is a packet being transmitted from this transducer.
     */
    virtual bool IsTx() const = 0;
    /**
     * Get the list of overlapped (in time) packets at this transducer.
     *
     * @return List of all packets currently crossing this node in the water.
     */
    virtual const ArrivalList& GetArrivalList() const = 0;
    /**
     * Set the receiver gain.
     *
     * @param gainDb Gain added at receiver, in dB.
     */
    virtual void SetRxGainDb(double gainDb) = 0;
    /**
     * Get the receiver gain added to signal at receiver in dB.
     *
     * @return The gain (in dB).
     */
    virtual double GetRxGainDb() = 0;
    /**
     * Apply receiver gain in dB to the received power.
     * @param rxPowerDb Signal power in dB of arriving packet.
     * @param mode Mode arriving packet is using.
     *
     * @return Updated receive power (in dB) with gain applied.
     */
    virtual double ApplyRxGainDb(double rxPowerDb, UanTxMode mode) = 0;
    /**
     * Notify this object that a new packet has arrived at this nodes location
     *
     * @param packet Packet arriving.
     * @param rxPowerDb Signal power in dB of arriving packet.
     * @param txMode Mode arriving packet is using.
     * @param pdp PDP of arriving signal.
     */
    virtual void Receive(Ptr<Packet> packet, double rxPowerDb, UanTxMode txMode, UanPdp pdp) = 0;
    /**
     * Transmit a packet from this transducer.
     *
     * @param src Source PHY.
     * @param packet Packet to transmit.
     * @param txPowerDb Outgoing Tx power of packet.
     * @param txMode Mode to transmit packet with.
     */
    virtual void Transmit(Ptr<UanPhy> src,
                          Ptr<Packet> packet,
                          double txPowerDb,
                          UanTxMode txMode) = 0;
    /**
     * Attach this transducer to a channel.
     *
     * @param chan The channel
     */
    virtual void SetChannel(Ptr<UanChannel> chan) = 0;
    /**
     * Get the attached channel.
     *
     * @return The channel.
     */
    virtual Ptr<UanChannel> GetChannel() const = 0;
    /**
     * Attach a physical network layer above this transducer.
     *
     * More than one physical layer may be attached.
     *
     * @param phy The physical layer.
     */
    virtual void AddPhy(Ptr<UanPhy> phy) = 0;
    /**
     * Get the list of physical layer above this transducer.
     *
     * @return List of attached physical layers.
     */
    virtual const UanPhyList& GetPhyList() const = 0;
    /**
     * Clears all pointer references.
     */
    virtual void Clear() = 0;

    // end of class UanTransducer
};

} // namespace ns3

#endif /* UAN_TRANSDUCER_H */
