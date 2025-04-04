#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("EcnTest");

int main(int argc, char* argv[])
{
    // Enable logging
    LogComponentEnable("EcnTest", LOG_LEVEL_INFO);

    // Command-line arguments for customization
    CommandLine cmd;
    cmd.Parse(argc, argv);

    // Create nodes: Sender (0), Router (1), Receiver (2)
    NodeContainer nodes;
    nodes.Create(3);

    // Define point-to-point links (no bottleneck)
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps")); // Sender to Router
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    PointToPointHelper routerToReceiver;
    routerToReceiver.SetDeviceAttribute("DataRate", StringValue("5Mbps")); // Same as input
    routerToReceiver.SetChannelAttribute("Delay", StringValue("2ms"));

    // Install devices and channels
    NetDeviceContainer senderToRouter = p2p.Install(nodes.Get(0), nodes.Get(1));
    NetDeviceContainer routerToReceiver = routerToReceiver.Install(nodes.Get(1), nodes.Get(2));

    // Install Internet stack with ECN support
    InternetStackHelper stack;
    stack.Install(nodes);

    // Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer senderRouterIfaces = address.Assign(senderToRouter);

    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer routerReceiverIfaces = address.Assign(routerToReceiver);

    // Enable global routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Configure TCP sender (BulkSendApplication)
    BulkSendHelper sender("ns3::TcpSocketFactory",
                          InetSocketAddress(routerReceiverIfaces.GetAddress(1), 5000));
    sender.SetAttribute("MaxBytes", UintegerValue(0)); // Unlimited data
    sender.SetAttribute("SendSize", UintegerValue(1000)); // Packet size

    // Install sender on Node 0
    ApplicationContainer senderApp = sender.Install(nodes.Get(0));
    senderApp.Start(Seconds(1.0));
    senderApp.Stop(Seconds(10.0));

    // Configure TCP receiver (PacketSink)
    PacketSinkHelper receiver("ns3::TcpSocketFactory",
                              InetSocketAddress(Ipv4Address::GetAny(), 5000));
    ApplicationContainer receiverApp = receiver.Install(nodes.Get(2));
    receiverApp.Start(Seconds(0.0));
    receiverApp.Stop(Seconds(10.0));

    // Enable ECN on TCP sockets
    Config::SetDefault("ns3::TcpSocket::EcnMode", StringValue("ClassicEcn"));
    Config::SetDefault("ns3::TcpSocketBase::UseEcn", EnumValue(TcpSocketState::On));

    // Enable PCAP tracing on all interfaces
    p2p.EnablePcap("ecn-test-sender", senderToRouter.Get(0));    // Sender interface
    routerToReceiver.EnablePcap("ecn-test-router", routerToReceiver.Get(0)); // Router outgoing
    routerToReceiver.EnablePcap("ecn-test-receiver", routerToReceiver.Get(1)); // Receiver interface

    // Run simulation
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}