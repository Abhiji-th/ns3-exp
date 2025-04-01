#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/ipv4-header.h"
#include "ns3/tcp-socket-state.h"
#include "ns3/tcp-socket-base.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("EcnRouter");

// Callback to inspect packets for ECN markings
void PacketSinkRx(Ptr<const Packet> packet, const Address& from)
{
    Ipv4Header ipHeader;
    if (packet->PeekHeader(ipHeader))
    {
        uint8_t tos = ipHeader.GetTos();
        if ((tos & 0x03) == 0x03) // CE
            NS_LOG_INFO("Received packet with ECN CE at " << Simulator::Now().GetSeconds());
        else if ((tos & 0x03) == 0x02) // ECT(0)
            NS_LOG_INFO("Received ECT(0) packet at " << Simulator::Now().GetSeconds());
        else if ((tos & 0x03) == 0x01) // ECT(1)
            NS_LOG_INFO("Received ECT(1) packet at " << Simulator::Now().GetSeconds());
    }
}

// Callback to trace ECN state changes
void EcnStateTrace(TcpSocketState::EcnState_t oldValue, TcpSocketState::EcnState_t newValue)
{
    NS_LOG_INFO("ECN State Transition at " << Simulator::Now().GetSeconds() 
                << "s: " << TcpSocketState::EcnStateName[oldValue] 
                << " -> " << TcpSocketState::EcnStateName[newValue]);
}

// Callback to trace congestion window changes
void CwndTrace(uint32_t oldValue, uint32_t newValue)
{
    NS_LOG_INFO("Congestion Window at " << Simulator::Now().GetSeconds() 
                << "s: " << oldValue << " -> " << newValue);
}

// Callback to trace ECE sequence numbers
void EcnEchoSeqTrace(SequenceNumber32 oldValue, SequenceNumber32 newValue)
{
    NS_LOG_INFO("ECE Sequence at " << Simulator::Now().GetSeconds() 
                << "s: " << oldValue << " -> " << newValue);
}

// Callback to trace CE sequence numbers
void EcnCeSeqTrace(SequenceNumber32 oldValue, SequenceNumber32 newValue)
{
    NS_LOG_INFO("CE Sequence at " << Simulator::Now().GetSeconds() 
                << "s: " << oldValue << " -> " << newValue);
}

// Callback to trace CWR sequence numbers
void EcnCwrSeqTrace(SequenceNumber32 oldValue, SequenceNumber32 newValue)
{
    NS_LOG_INFO("CWR Sequence at " << Simulator::Now().GetSeconds() 
                << "s: " << oldValue << " -> " << newValue);
}

int main(int argc, char *argv[])
{
    // Enable logging
    LogComponentEnable("EcnRouter", LOG_LEVEL_INFO);
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    LogComponentEnable("TcpSocketBase", LOG_LEVEL_INFO);

    // Configure RED queue with ECN disabled for simplicity (single packet won't trigger it)
    Config::SetDefault("ns3::RedQueueDisc::UseEcn", BooleanValue(false));
    Config::SetDefault("ns3::RedQueueDisc::MaxSize", QueueSizeValue(QueueSize("25p")));
    Config::SetDefault("ns3::RedQueueDisc::MinTh", DoubleValue(5));
    Config::SetDefault("ns3::RedQueueDisc::MaxTh", DoubleValue(15));
    Config::SetDefault("ns3::RedQueueDisc::MeanPktSize", UintegerValue(1024));

    // Enable ECN in TCP
    Config::SetDefault("ns3::TcpSocketBase::UseEcn", EnumValue(TcpSocketState::On));

    // Create nodes: sender, router, receiver
    NodeContainer nodes;
    nodes.Create(3);

    // Set up point-to-point links
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer senderToRouter = p2p.Install(nodes.Get(0), nodes.Get(1));
    NetDeviceContainer routerToReceiver = p2p.Install(nodes.Get(1), nodes.Get(2));

    // Install Internet stack
    InternetStackHelper stack;
    stack.Install(nodes);

    // Install RED queue discipline
    TrafficControlHelper tch;
    tch.SetRootQueueDisc("ns3::RedQueueDisc");
    tch.Install(senderToRouter.Get(1));
    tch.Install(routerToReceiver.Get(0));

    // Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer senderRouterIfaces = address.Assign(senderToRouter);
    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer routerReceiverIfaces = address.Assign(routerToReceiver);

    // Enable routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // TCP sink (receiver)
    uint16_t port = 5000;
    PacketSinkHelper sink("ns3::TcpSocketFactory",
                          InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApp = sink.Install(nodes.Get(2));
    sinkApp.Start(Seconds(0.0));  // Start immediately
    sinkApp.Stop(Seconds(2.0));   // Short duration for single packet transfer

    // Connect the sink to monitor ECN markings
    Ptr<PacketSink> packetSink = sinkApp.Get(0)->GetObject<PacketSink>();
    packetSink->TraceConnectWithoutContext("PacketReceived", MakeCallback(&PacketSinkRx));

    // TCP sender (OnOffApplication) - Send only one packet
    OnOffHelper onoff("ns3::TcpSocketFactory",
                      InetSocketAddress(routerReceiverIfaces.GetAddress(1), port));
    onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=0.001]"));  // Very short on-time
    onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=10]"));   // Long off-time
    onoff.SetAttribute("DataRate", DataRateValue(DataRate("1Mbps")));  // Slow rate to ensure one packet
    onoff.SetAttribute("PacketSize", UintegerValue(1024));             // Single packet size
    onoff.SetAttribute("MaxBytes", UintegerValue(1024));               // Limit to one packet
    ApplicationContainer onoffApp = onoff.Install(nodes.Get(0));
    onoffApp.Start(Seconds(0.1));  // Start after sink is ready
    onoffApp.Stop(Seconds(2.0));   // Short simulation duration

    // Trace ECN-related metrics on the sender's TCP socket
    Ptr<Node> senderNode = nodes.Get(0);
    Simulator::Schedule(Seconds(0.2), [&]() {
        Ptr<Socket> socket = onoffApp.Get(0)->GetObject<OnOffApplication>()->GetSocket();
        if (socket)
        {
            Ptr<TcpSocketBase> tcpSocket = DynamicCast<TcpSocketBase>(socket);
            if (tcpSocket)
            {
                tcpSocket->TraceConnectWithoutContext("EcnState", MakeCallback(&EcnStateTrace));
                tcpSocket->TraceConnectWithoutContext("CongestionWindow", MakeCallback(&CwndTrace));
                tcpSocket->TraceConnectWithoutContext("EcnEchoSeq", MakeCallback(&EcnEchoSeqTrace));
                tcpSocket->TraceConnectWithoutContext("EcnCeSeq", MakeCallback(&EcnCeSeqTrace));
                tcpSocket->TraceConnectWithoutContext("EcnCwrSeq", MakeCallback(&EcnCwrSeqTrace));
            }
        }
    });

    // Enable packet tracing individually for each device
    p2p.EnablePcap("ecn-sender-to-router", senderToRouter.Get(0));    // Sender's device
    p2p.EnablePcap("ecn-router-to-sender", senderToRouter.Get(1));    // Router's device (Sender side)
    p2p.EnablePcap("ecn-router-to-receiver", routerToReceiver.Get(0)); // Router's device (Receiver side)
    p2p.EnablePcap("ecn-receiver-to-router", routerToReceiver.Get(1)); // Receiver's device

    // Run simulation for a short duration
    Simulator::Stop(Seconds(2.0));
    Simulator::Run();
    Simulator::Destroy();

    NS_LOG_INFO("Simulation completed.");
    return 0;
}