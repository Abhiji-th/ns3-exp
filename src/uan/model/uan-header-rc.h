/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Leonard Tracy <lentracy@gmail.com>
 */

#ifndef UAN_HEADER_RC_H
#define UAN_HEADER_RC_H

#include "ns3/header.h"
#include "ns3/mac8-address.h"
#include "ns3/nstime.h"

#include <set>

namespace ns3
{

/**
 * @ingroup uan
 *
 * Extra data header information.
 *
 * Adds propagation delay measure, and frame number info to
 * transmitted data packet.
 */
class UanHeaderRcData : public Header
{
  public:
    /** Default constructor */
    UanHeaderRcData();
    /**
     * Constructor.
     *
     * @param frameNum Data frame # of reservation being transmitted.
     * @param propDelay  Measured propagation delay found in handshaking.
     * @note Prop. delay is transmitted with 16 bits and ms accuracy.
     */
    UanHeaderRcData(uint8_t frameNum, Time propDelay);
    /** Destructor */
    ~UanHeaderRcData() override;

    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Set the frame number of the reservation being transmitted.
     *
     * @param frameNum The data frame number.
     */
    void SetFrameNo(uint8_t frameNum);
    /**
     * Set the propagation delay as found in handshaking.
     *
     * @param propDelay The measured propagation delay.
     * @note Prop. delay is transmitted with 16 bits and ms accuracy.
     */
    void SetPropDelay(Time propDelay);
    /**
     * Get the frame number of the reservation being transmitted.
     *
     * @return The data frame number.
     */
    uint8_t GetFrameNo() const;
    /**
     * Get the propagation delay found in handshaking.
     *
     * @return The measured propagation delay.
     * @note Prop. delay is transmitted with 16 bits and ms accuracy
     */
    Time GetPropDelay() const;
    /**
     * Specialized Print with Time::Unit declared.
     *
     * @param os ostream.
     * @param unit Time unit.
     */
    void Print(std::ostream& os, Time::Unit unit) const;

    // Inherited methods
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;
    TypeId GetInstanceTypeId() const override;

  private:
    uint8_t m_frameNo; //!< Data frame number.
    Time m_propDelay;  //!< Propagation delay.

    // end of class UanHeaderRcData
};

/**
 * @ingroup uan
 *
 * RTS header.
 *
 * Contains frame number, retry number, number of frames, length, and timestamp.
 */
class UanHeaderRcRts : public Header
{
  public:
    /** Default constructor */
    UanHeaderRcRts();
    /**
     * Constructor.
     *
     * @param frameNo Reservation frame number.
     * @param retryNo Retry number of RTS packet.
     * @param noFrames Number of data frames in reservation.
     * @param length Number of bytes (including headers) in data.
     * @param ts RTS TX timestamp.
     * @note Timestamp is serialized into 32 bits with ms accuracy.
     */
    UanHeaderRcRts(uint8_t frameNo, uint8_t retryNo, uint8_t noFrames, uint16_t length, Time ts);
    /** Destructor */
    ~UanHeaderRcRts() override;

    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Set the frame number.
     *
     * @param fno TX frame number
     */
    void SetFrameNo(uint8_t fno);
    /**
     * Set the number of data frames included in this reservation request.
     *
     * @param no Number of frames.
     */
    void SetNoFrames(uint8_t no);
    /**
     * Set RTS transmission time.
     *
     * @param timeStamp The RTS transmission time.
     */
    void SetTimeStamp(Time timeStamp);
    /**
     * Set the number of data bytes in the reservation.
     *
     * @param length Total number of data bytes in reservation (including headers).
     * @note Timestamp is serialized with 32 bits in ms precision.
     */
    void SetLength(uint16_t length);
    /**
     * Set the retry number of this RTS packet.
     *
     * This is used to match timestamp to correctly received RTS.
     *
     * @param no Retry number.
     */
    void SetRetryNo(uint8_t no);

    /**
     * Get the frame number.
     *
     * @return The frame number.
     */
    uint8_t GetFrameNo() const;
    /**
     * Get the number of data frames in the reservation.
     *
     * @return The number of data frames.
     */
    uint8_t GetNoFrames() const;
    /**
     * Get the transmit timestamp of this RTS packet.
     *
     * @return The TX time.
     * @note Timestamp is serialized with 32 bits in ms precision.
     */
    Time GetTimeStamp() const;
    /**
     * Get the total number of bytes in the reservation, including headers.
     *
     * @return Total number of bytes in data packets for reservation.
     */
    uint16_t GetLength() const;
    /**
     * Get the retry number of this RTS packet.
     *
     * @return The retry number.
     */
    uint8_t GetRetryNo() const;
    /**
     * Specialized Print with Time::Unit declared.
     *
     * @param os ostream.
     * @param unit Time unit.
     */
    void Print(std::ostream& os, Time::Unit unit) const;

    // Inherited methods
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;
    TypeId GetInstanceTypeId() const override;

  private:
    uint8_t m_frameNo;  //!< Reservation frame number.
    uint8_t m_noFrames; //!< Number of data frames in reservation.
    uint16_t m_length;  //!< Number of bytes (including headers) in data.
    Time m_timeStamp;   //!< RTS TX timestamp.
    uint8_t m_retryNo;  //!< Retry number of RTS packet.

    // end of class UanHeaderRcRts
};

/**
 * @ingroup uan
 *
 * Cycle broadcast information.
 *
 * This includes the rate number, retry rate and window time.
 */
class UanHeaderRcCtsGlobal : public Header
{
  public:
    /** Default constructor */
    UanHeaderRcCtsGlobal();
    /**
     * Constructor
     *
     * @param wt Window time.
     * @param ts Timestamp.
     * @param rate Rate number.
     * @param retryRate  Retry rate value.
     */
    UanHeaderRcCtsGlobal(Time wt, Time ts, uint16_t rate, uint16_t retryRate);
    /** Destructor */
    ~UanHeaderRcCtsGlobal() override;

    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Set the rate number corresponding to data rate of current cycle.
     * @param rate The rate number.
     */
    void SetRateNum(uint16_t rate);
    /**
     * Set the retry rate number for the current cycle.
     * @param rate The retry rate number
     */
    void SetRetryRate(uint16_t rate);
    /**
     * Set the window time (time duration following blocking time
     * to allow RTS transmissions).
     *
     * @param t The window time.
     */
    void SetWindowTime(Time t);

    /**
     * Set the CTS timestamp.
     *
     * @param timeStamp The time of CTS transmission.
     */
    void SetTxTimeStamp(Time timeStamp);

    /**
     * Get the data rate number.
     *
     * @return The rate number.
     */
    uint16_t GetRateNum() const;
    /**
     * Get the retry rate number.
     *
     * @return The retry rate number.
     */
    uint16_t GetRetryRate() const;
    /**
     * Get the window time (time duration following blocking time
     * to allow RTS transmissions).
     *
     * @return The window time.
     */
    Time GetWindowTime() const;
    /**
     * Get the CTS transmit timestamp.
     *
     * @return The timestamp.
     */
    Time GetTxTimeStamp() const;
    /**
     * Specialized Print with Time::Unit declared.
     *
     * @param os ostream.
     * @param unit Time unit.
     */
    void Print(std::ostream& os, Time::Unit unit) const;

    // Inherited methods
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;
    TypeId GetInstanceTypeId() const override;

  private:
    Time m_timeStampTx;   //!< Timestamp.
    Time m_winTime;       //!< Window time.
    uint16_t m_retryRate; //!< Retry rate.
    uint16_t m_rateNum;   //!< Rate number.

    // end of class UanHeaderRcCtsGlobal
};

/**
 * @ingroup uan
 *
 * CTS header
 *
 * Includes RTS RX time, CTS TX time, delay until TX, RTS blocking period,
 * RTS tx period, rate #, and retry rate #
 */
class UanHeaderRcCts : public Header
{
  public:
    /** Default constructor */
    UanHeaderRcCts();
    /**
     * Constructor
     *
     * @param frameNo Reservation frame # being cleared.
     * @param retryNo Retry # of received RTS packet.
     * @param rtsTs RX time of RTS packet at gateway.
     * @param delay Delay until transmission.
     * @param addr Destination of CTS packet.
     * @note Times are serialized, with ms precision, into 32 bit fields.
     */
    UanHeaderRcCts(uint8_t frameNo, uint8_t retryNo, Time rtsTs, Time delay, Mac8Address addr);
    /** Destructor */
    ~UanHeaderRcCts() override;

    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Set the RTS frame number being cleared.
     *
     * @param frameNo The frame number.
     */
    void SetFrameNo(uint8_t frameNo);
    /**
     * Set the timestamp for RTS reception.
     *
     * @param timeStamp The timestamp.
     */
    void SetRtsTimeStamp(Time timeStamp);
    /**
     * Set the time delay from CTS transmission to first data frame arrival.
     *
     * @param delay The delay time.
     */
    void SetDelayToTx(Time delay);
    /**
     * Set the retry number of the RTS frame being cleared.
     *
     * @param no The retry number.
     */
    void SetRetryNo(uint8_t no);
    /**
     * Set the destination address, for scheduling info.
     *
     * @param addr The destination address.
     */
    void SetAddress(Mac8Address addr);

    /**
     * Get the frame number of the RTS being cleared.
     *
     * @return The frame number.
     */
    uint8_t GetFrameNo() const;
    /**
     * Get the receive time of the RTS being cleared.
     *
     * @return The RX time.
     */
    Time GetRtsTimeStamp() const;
    /**
     * Get the time delay from TX time of CTS packet until
     * arrival of first data frame.
     *
     * @return The delay time.
     */
    Time GetDelayToTx() const;
    /**
     * Get the retry number of the RTS packet being cleared.
     *
     * @return The retry number
     */
    uint8_t GetRetryNo() const;
    /**
     * Get the destination address, for scheduling info.
     *
     * @return The destination address.
     */
    Mac8Address GetAddress() const;
    /**
     * Specialized Print with Time::Unit declared.
     *
     * @param os ostream.
     * @param unit Time unit.
     */
    void Print(std::ostream& os, Time::Unit unit) const;

    // Inherited methods
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;
    TypeId GetInstanceTypeId() const override;

  private:
    uint8_t m_frameNo;     //!< Reservation frame number being cleared.
    Time m_timeStampRts;   //!< RX time of RTS packet at gateway.
    uint8_t m_retryNo;     //!< Retry number of received RTS packet.
    Time m_delay;          //!< Delay until transmission.
    Mac8Address m_address; //!< Destination of CTS packet.

    // end of class UanHeaderRcCts
};

/**
 * @ingroup uan
 *
 * Header used for ACK packets by protocol UanMacRc
 */
class UanHeaderRcAck : public Header
{
  public:
    /** Default constructor */
    UanHeaderRcAck();
    /** Destructor */
    ~UanHeaderRcAck() override;

    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Set the frame number of the reservation being acknowledged.
     *
     * @param frameNo The frame number.
     */
    void SetFrameNo(uint8_t frameNo);
    /**
     * NACK a frame.
     *
     * @param frame The data frame number being NACKed.
     */
    void AddNackedFrame(uint8_t frame);

    /**
     * Get the set of NACK'ed frames.
     *
     * @return The set of NACK'ed frames.
     */
    const std::set<uint8_t>& GetNackedFrames() const;
    /**
     * Get the reservation frame number being ACKed.
     *
     * @return The frame number.
     */
    uint8_t GetFrameNo() const;
    /**
     * Get the number of data frames being NACKed.
     *
     * @return The number of NACKed frames.
     */
    uint8_t GetNoNacks() const;

    // Inherited methods
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;
    TypeId GetInstanceTypeId() const override;

  private:
    uint8_t m_frameNo;                //!< Next frame number.
    std::set<uint8_t> m_nackedFrames; //!< Marker for nacked frames.

    // end of class UanHeaderRcAck
};

} // namespace ns3

#endif /* UAN_HEADER_RC_H */
