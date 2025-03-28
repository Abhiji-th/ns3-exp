/*
 * Copyright (c) 2018 Natale Patriciello <natale.patriciello@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */
#include "tcp-tx-item.h"

namespace ns3
{

void
TcpTxItem::Print(std::ostream& os, Time::Unit unit /* = Time::S */) const
{
    bool comma = false;
    os << "[" << m_startSeq << ";" << m_startSeq + GetSeqSize() << "|" << GetSeqSize() << "]";

    if (m_lost)
    {
        os << "[lost]";
        comma = true;
    }
    if (m_retrans)
    {
        if (comma)
        {
            os << ",";
        }

        os << "[retrans]";
        comma = true;
    }
    if (m_sacked)
    {
        if (comma)
        {
            os << ",";
        }
        os << "[sacked]";
        comma = true;
    }
    if (comma)
    {
        os << ",";
    }
    os << "[" << m_lastSent.As(unit) << "]";
}

uint32_t
TcpTxItem::GetSeqSize() const
{
    return m_packet && m_packet->GetSize() > 0 ? m_packet->GetSize() : 1;
}

bool
TcpTxItem::IsSacked() const
{
    return m_sacked;
}

bool
TcpTxItem::IsRetrans() const
{
    return m_retrans;
}

Ptr<Packet>
TcpTxItem::GetPacketCopy() const
{
    return m_packet->Copy();
}

Ptr<const Packet>
TcpTxItem::GetPacket() const
{
    return m_packet;
}

const Time&
TcpTxItem::GetLastSent() const
{
    return m_lastSent;
}

TcpTxItem::RateInformation&
TcpTxItem::GetRateInformation()
{
    return m_rateInfo;
}

} // namespace ns3
