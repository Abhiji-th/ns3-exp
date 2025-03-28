/*
 * Copyright (c) 2023 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/boolean.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/data-rate.h"
#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/error-model.h"
#include "ns3/inet-socket-address.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/names.h"
#include "ns3/node.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/trace-helper.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/udp-socket.h"
#include "ns3/uinteger.h"
#include "ns3/wifi-mac.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-psdu.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"

#include <limits>
#include <sstream>
#include <string>

// This is an example to verify performance of 802.11aa groupcast with retries (GCR) in comparison
// with the usual NoAck/NoRetry retransmission policy. In the latter, a groupcast frame is
// transmitted only once, whereas GCR allows to retransmit groupcast frames to improve reliability.
//
// The simulation considers a single 802.11ax AP and a configurable number of GCR-capable STAs in an
// infrastructure network. Multicast traffic is generated from the AP to all the non-AP STAs and
// artificial errors can be introduced to mimic interference on the wireless channel.
//
// There are a number of command-line options available to control the scenario under test. The list
// of available command-line options can be listed with the following command:
// ./ns3 run "wifi-multicast --help"
//
// The main command-line options are:
//    --gcrRetransmissionPolicy: control the retransmission policy by selecting "NoAckNoRetry" for
//    no retransmission, "GcrUr" for GCR with unsolicited retries or "GcrBlockAck" for GCR Block Ack
//    --nStations: control the number of GCR-capable STAs associated to the AP
//    --accessCategory: control the access category to use for the multicast traffic
//    --multicastFrameErrorRate: set the artificial frame error rate for the groupcast traffic
//    --nRetriesGcrUr: if GCR-UR is selected, this parameter controls the maximum number of retries
//    --gcrProtection: select the protection mechanism for groupcast frames if GCR-UR or GCR-BA is
//    used, either "Rts-Cts" or "Cts-To-Self" can be selected
//
// Example usage for NoAckNoRetry and a frame error rate of 20%:
// ./ns3 run "wifi-multicast --gcrRetransmissionPolicy=NoAckNoRetry --multicastFrameErrorRate=0.2"
// which outputs:
// Node         TX packets  TX bytes    RX packets  RX bytes    Throughput (Mbit/s)
// AP           10          10000       0           0           11.1111
// STA1         0           0           10          10000       10.992
//
// Example usage for GCR-UR with up to 2 retries and the same frame error rate:
// ./ns3 run "wifi-multicast --gcrRetransmissionPolicy=GcrUr --nRetriesGcrUr=2
// --multicastFrameErrorRate=0.2"
// which outputs:
// Node         TX packets  TX bytes    RX packets  RX bytes    Throughput (Mbit/s)
// AP           10          10000       0           0           11.1111
// STA1         0           0           10          10000       10.992
//
// Example usage for GCR-BA with 4 STAs and the same frame error rate:
// ./ns3 run "wifi-multicast --gcrRetransmissionPolicy=GcrBlockAck --nStations=4"
// which outputs:
// Node         TX packets  TX bytes    RX packets  RX bytes    Throughput (Mbit/s)
// AP           10          10000       0           0           11.1111
// STA1         0           0           10          10000       8.26959
// STA2         0           0           10          10000       8.26959
// STA3         0           0           10          10000       8.26959
// STA4         0           0           10          10000       8.26959

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiMulticast");

uint64_t g_txBytes; //!< Number of generated bytes
Time g_firstTx;     //!< Time at which first TX packet is generated
Time g_lastTx;      //!< Time at which last TX packet is generated
Time g_lastRx;      //!< Time at which last RX packet is received

/**
 * Parse context strings of the form "/NodeList/x/DeviceList/x/..." to extract the NodeId integer
 *
 * @param context The context to parse.
 * @return the NodeId
 */
uint32_t
ContextToNodeId(const std::string& context)
{
    std::string sub = context.substr(10);
    uint32_t pos = sub.find("/Device");
    return std::stoi(sub.substr(0, pos));
}

/**
 * Socket sent packet.
 *
 * @param context The context.
 * @param p The packet.
 */
void
SocketTxPacket(std::string context, Ptr<const Packet> p)
{
    if (g_txBytes == 0)
    {
        g_firstTx = Simulator::Now();
    }
    g_txBytes += p->GetSize();
    g_lastTx = Simulator::Now();
}

/**
 *
 * @param context The context.
 * @param p The packet.
 * @param from sender address.
 */
void
SocketRxPacket(std::string context, Ptr<const Packet> p, const Address& from)
{
    g_lastRx = Simulator::Now();
}

/**
 * Callback when a frame is transmitted.
 * @param rxErrorModel the post reception error model on the receiver
 * @param ranVar the random variable to determine whether the packet shall be corrupted
 * @param errorRate the configured corruption error rate for multicast frames
 * @param context the context
 * @param psduMap the PSDU map
 * @param txVector the TX vector
 * @param txPowerW the tx power in Watts
 */
void
TxCallback(Ptr<ListErrorModel> rxErrorModel,
           Ptr<RandomVariableStream> ranVar,
           double errorRate,
           std::string context,
           WifiConstPsduMap psduMap,
           WifiTxVector txVector,
           double txPowerW)
{
    auto psdu = psduMap.begin()->second;
    if (psdu->GetHeader(0).GetAddr1().IsGroup() && !psdu->GetHeader(0).GetAddr1().IsBroadcast() &&
        psdu->GetHeader(0).IsQosData())
    {
        NS_LOG_INFO("AP tx multicast: PSDU=" << *psdu << " TXVECTOR=" << txVector);
        if (ranVar->GetValue() < errorRate)
        {
            auto uid = psdu->GetPayload(0)->GetUid();
            NS_LOG_INFO("Corrupt multicast frame with UID=" << uid);
            rxErrorModel->SetList({uid});
        }
        else
        {
            rxErrorModel->SetList({});
        }
    }
}

/**
 * Callback when a frame is successfully received.
 *
 * @param context the context
 * @param p the packet
 * @param snr the SNR (in linear scale)
 * @param mode the mode used to transmit the packet
 * @param preamble the preamble
 */
void
RxCallback(std::string context,
           Ptr<const Packet> p,
           double snr,
           WifiMode mode,
           WifiPreamble preamble)
{
    Ptr<Packet> packet = p->Copy();
    WifiMacHeader hdr;
    packet->RemoveHeader(hdr);
    if (hdr.GetAddr1().IsGroup() && !hdr.GetAddr1().IsBroadcast() && hdr.IsQosData())
    {
        std::stringstream ss;
        hdr.Print(ss);
        NS_LOG_INFO("STA" << ContextToNodeId(context) << " rx multicast: " << ss.str());
    }
}

int
main(int argc, char* argv[])
{
    bool logging{false};
    bool verbose{false};
    bool pcap{false};
    std::size_t nStations{1};
    Time simulationTime{"10s"};
    uint32_t payloadSize{1000}; // bytes
    DataRate dataRate{"10Mb/s"};
    uint32_t maxPackets{10};
    uint32_t rtsThreshold{std::numeric_limits<uint16_t>::max()};
    std::string targetAddr{"239.192.100.1"};
    std::string accessCategory{"AC_BE"};
    std::string gcrRetransmissionPolicy{"NoAckNoRetry"};
    std::string rateManager{"Constant"};
    uint16_t constantRateMcs{11};
    uint16_t nRetriesGcrUr{7};
    std::string gcrProtection{"Rts-Cts"};
    double multicastFrameErrorRate{0.0};
    uint16_t maxAmpduLength{0};
    double minExpectedPackets{0};
    double maxExpectedPackets{0};
    double minExpectedThroughput{0};
    double maxExpectedThroughput{0};
    double tolerance{0.01};

    CommandLine cmd(__FILE__);
    cmd.AddValue("logging", "turn on example log components", logging);
    cmd.AddValue("verbose", "turn on all wifi log components", verbose);
    cmd.AddValue("pcap", "turn on pcap file output", pcap);
    cmd.AddValue("nStations", "number of non-AP stations", nStations);
    cmd.AddValue("simulationTime", "Simulation time", simulationTime);
    cmd.AddValue("payloadSize", "The application payload size in bytes", payloadSize);
    cmd.AddValue(
        "maxPackets",
        "The maximum number of packets to be generated by the application (0 for no limit)",
        maxPackets);
    cmd.AddValue("dataRate", "The application data rate", dataRate);
    cmd.AddValue("rtsThreshold", "RTS threshold", rtsThreshold);
    cmd.AddValue("rateManager",
                 "The rate adaptation manager to use (Constant, Ideal, MinstrelHt)",
                 rateManager);
    cmd.AddValue("mcs",
                 "The MCS to use if Constant rate adaptation manager is used",
                 constantRateMcs);
    cmd.AddValue("targetAddress", "multicast target address", targetAddr);
    cmd.AddValue("accessCategory",
                 "select the multicast traffic access category (AC_BE, AC_BK, AC_VI, AC_VO)",
                 accessCategory);
    cmd.AddValue(
        "gcrRetransmissionPolicy",
        "GCR retransmission policy for groupcast frames (NoAckNoRetry, GcrUr, GcrBlockAck)",
        gcrRetransmissionPolicy);
    cmd.AddValue("nRetriesGcrUr",
                 "number of retries per groupcast frame when GCR-UR retransmission policy is used",
                 nRetriesGcrUr);
    cmd.AddValue("gcrProtection",
                 "protection to use for GCR (Rts-Cts or Cts-To-Self)",
                 gcrProtection);
    cmd.AddValue("multicastFrameErrorRate",
                 "artificial error rate for multicast frame",
                 multicastFrameErrorRate);
    cmd.AddValue("maxAmpduLength", "maximum length in bytes of an A-MPDU", maxAmpduLength);
    cmd.AddValue("minExpectedPackets",
                 "if set, simulation fails if the lowest amount of received packets is below this "
                 "value (in Mbit/s)",
                 minExpectedPackets);
    cmd.AddValue("maxExpectedPackets",
                 "if set, simulation fails if the highest amount of received packets is above this "
                 "value (in Mbit/s)",
                 maxExpectedPackets);
    cmd.AddValue("minExpectedThroughput",
                 "if set, simulation fails if the throughput is below this value",
                 minExpectedThroughput);
    cmd.AddValue("maxExpectedThroughput",
                 "if set, simulation fails if the throughput is above this value",
                 maxExpectedThroughput);
    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::WifiMac::RobustAVStreamingSupported", BooleanValue(true));

    // create nodes
    NodeContainer wifiApNode;
    wifiApNode.Create(1);
    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(nStations);

    // configure PHY and MAC
    WifiHelper wifi;
    if (verbose)
    {
        WifiHelper::EnableLogComponents();
    }
    if (logging)
    {
        LogComponentEnable("WifiMulticast", LOG_LEVEL_ALL);
    }
    wifi.SetStandard(WIFI_STANDARD_80211ax);

    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);

    auto wifiChannel = YansWifiChannelHelper::Default();
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiPhy.SetChannel(wifiChannel.Create());

    WifiMacHelper wifiMac;
    if (rateManager == "Constant")
    {
        wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                     "DataMode",
                                     StringValue("HeMcs" + std::to_string(constantRateMcs)),
                                     "RtsCtsThreshold",
                                     UintegerValue(rtsThreshold));
    }
    else
    {
        wifi.SetRemoteStationManager("ns3::" + rateManager + "RateWifiManager",
                                     "RtsCtsThreshold",
                                     UintegerValue(rtsThreshold));
    }

    if (gcrRetransmissionPolicy != "NoAckNoRetry")
    {
        std::string retransmissionPolicyStr;
        if (gcrRetransmissionPolicy == "GcrUr")
        {
            retransmissionPolicyStr = "GCR_UR";
        }
        else if (gcrRetransmissionPolicy == "GcrBlockAck")
        {
            retransmissionPolicyStr = "GCR_BA";
        }
        else
        {
            std::cout << "Wrong retransmission policy!" << std::endl;
            return 0;
        }
        wifiMac.SetGcrManager("ns3::WifiDefaultGcrManager",
                              "RetransmissionPolicy",
                              StringValue(retransmissionPolicyStr),
                              "UnsolicitedRetryLimit",
                              UintegerValue(nRetriesGcrUr),
                              "GcrProtectionMode",
                              StringValue(gcrProtection));
    }

    Ssid ssid = Ssid("wifi-multicast");

    // setup AP
    wifiMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    auto apDevice = wifi.Install(wifiPhy, wifiMac, wifiApNode);

    // setup STAs
    wifiMac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    auto staDevices = wifi.Install(wifiPhy, wifiMac, wifiStaNodes);

    auto rxErrorModel = CreateObject<ListErrorModel>();
    auto ranVar = CreateObject<UniformRandomVariable>();
    ranVar->SetStream(1);
    Config::Connect("/NodeList/0/DeviceList/*/$ns3::WifiNetDevice/Phys/0/PhyTxPsduBegin",
                    MakeCallback(TxCallback).Bind(rxErrorModel, ranVar, multicastFrameErrorRate));
    for (std::size_t i = 0; i < nStations; ++i)
    {
        auto wifiMac = DynamicCast<WifiNetDevice>(staDevices.Get(i))->GetMac();
        wifiMac->GetWifiPhy(0)->SetPostReceptionErrorModel(rxErrorModel);
    }
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/State/RxOk",
                    MakeCallback(RxCallback));

    // mobility
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    for (std::size_t i = 0; i < nStations; ++i)
    {
        positionAlloc->Add(Vector(i, 0.0, 0.0));
    }
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNodes);

    // setup static routes to facilitate multicast flood
    Ipv4ListRoutingHelper listRouting;
    Ipv4StaticRoutingHelper staticRouting;
    listRouting.Add(staticRouting, 0);

    // configure IP stack
    InternetStackHelper internet;
    internet.SetIpv6StackInstall(false);
    internet.SetIpv4ArpJitter(true);
    internet.SetRoutingHelper(listRouting);
    internet.Install(wifiApNode);
    internet.Install(wifiStaNodes);

    Ipv4AddressHelper ipv4address;
    ipv4address.SetBase("10.0.0.0", "255.255.255.0");
    auto apNodeInterface = ipv4address.Assign(apDevice);
    auto staNodeInterfaces = ipv4address.Assign(staDevices);

    // add static route in AP
    auto ipv4 = wifiApNode.Get(0)->GetObject<Ipv4>();
    auto routing = staticRouting.GetStaticRouting(ipv4);
    routing->AddHostRouteTo(targetAddr.c_str(),
                            ipv4->GetInterfaceForDevice(wifiApNode.Get(0)->GetDevice(0)),
                            0);

    uint8_t tosValue;
    std::string maxAmpduSizeAttribute;
    if (accessCategory == "AC_BE")
    {
        tosValue = 0x70;
        maxAmpduSizeAttribute = "BE_MaxAmpduSize";
    }
    else if (accessCategory == "AC_BK")
    {
        tosValue = 0x28;
        maxAmpduSizeAttribute = "BK_MaxAmpduSize";
    }
    else if (accessCategory == "AC_VI")
    {
        tosValue = 0xb8;
        maxAmpduSizeAttribute = "VI_MaxAmpduSize";
    }
    else if (accessCategory == "AC_VO")
    {
        tosValue = 0xc0;
        maxAmpduSizeAttribute = "VO_MaxAmpduSize";
    }
    else
    {
        NS_ABORT_MSG("Invalid access category");
    }
    auto apWifiMac = DynamicCast<WifiNetDevice>(apDevice.Get(0))->GetMac();
    apWifiMac->SetAttribute(maxAmpduSizeAttribute, UintegerValue(maxAmpduLength));

    // sinks
    InetSocketAddress sinkSocket(Ipv4Address::GetAny(), 90);
    PacketSinkHelper sinkHelper("ns3::UdpSocketFactory", sinkSocket);
    auto sinks = sinkHelper.Install(wifiStaNodes);
    sinks.Start(Seconds(0));
    sinks.Stop(simulationTime + Seconds(2.0));

    // source
    InetSocketAddress sourceSocket(targetAddr.c_str(), 90);
    OnOffHelper onoffHelper("ns3::UdpSocketFactory", sourceSocket);
    onoffHelper.SetAttribute("DataRate", DataRateValue(dataRate));
    onoffHelper.SetAttribute("PacketSize", UintegerValue(payloadSize));
    onoffHelper.SetAttribute("MaxBytes", UintegerValue(maxPackets * payloadSize));
    onoffHelper.SetAttribute("Tos", UintegerValue(tosValue));
    onoffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onoffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    auto source = onoffHelper.Install(wifiApNode);
    source.Start(Seconds(1));
    source.Stop(simulationTime + Seconds(1.0));

    // pcap
    if (pcap)
    {
        wifiPhy.EnablePcap("wifi-multicast-AP", apDevice.Get(0));
        for (std::size_t i = 0; i < nStations; ++i)
        {
            wifiPhy.EnablePcap("wifi-multicast-STA" + std::to_string(i + 1), staDevices.Get(i));
        }
    }

    g_txBytes = 0;

    Config::Connect("/NodeList/*/$ns3::Node/ApplicationList/*/$ns3::OnOffApplication/Tx",
                    MakeCallback(&SocketTxPacket));

    Config::Connect("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx",
                    MakeCallback(&SocketRxPacket));

    // run simulation
    Simulator::Stop(simulationTime + Seconds(2.0));
    Simulator::Run();

    // check results
    std::cout << "Node\t\t\tTX packets\t\tTX bytes\t\tRX packets\t\tRX bytes\t\tThroughput (Mbit/s)"
              << std::endl;
    const auto txRate =
        (g_lastTx - g_firstTx).IsStrictlyPositive()
            ? static_cast<double>(g_txBytes * 8) / ((g_lastTx - g_firstTx).GetMicroSeconds())
            : 0.0; // Mbit/s
    const auto txPackets = g_txBytes / payloadSize;
    std::cout << "AP"
              << "\t\t\t" << txPackets << "\t\t\t" << g_txBytes << "\t\t\t0\t\t\t0\t\t\t" << txRate
              << "" << std::endl;
    for (std::size_t i = 0; i < nStations; ++i)
    {
        const auto rxBytes = sinks.Get(0)->GetObject<PacketSink>()->GetTotalRx();
        const auto rxPackets = rxBytes / payloadSize;
        const auto throughput =
            (g_lastRx - g_firstTx).IsStrictlyPositive()
                ? static_cast<double>(rxBytes * 8) / ((g_lastRx - g_firstTx).GetMicroSeconds())
                : 0.0; // Mbit/s
        std::cout << "STA" << i + 1 << "\t\t\t0\t\t\t0\t\t\t" << rxPackets << "\t\t\t" << rxBytes
                  << "\t\t\t" << throughput << "" << std::endl;
        if (rxPackets < minExpectedPackets)
        {
            NS_LOG_ERROR("Obtained RX packets " << rxPackets << " is not expected!");
            exit(1);
        }
        if (maxExpectedPackets > 0 && rxPackets > maxExpectedPackets)
        {
            NS_LOG_ERROR("Obtained RX packets " << rxPackets << " is not expected!");
            exit(1);
        }
        if ((throughput * (1 + tolerance)) < minExpectedThroughput)
        {
            NS_LOG_ERROR("Obtained throughput " << throughput << " is not expected!");
            exit(1);
        }
        if (maxExpectedThroughput > 0 && (throughput > (maxExpectedThroughput * (1 + tolerance))))
        {
            NS_LOG_ERROR("Obtained throughput " << throughput << " is not expected!");
            exit(1);
        }
    }

    Simulator::Destroy();
    return 0;
}
