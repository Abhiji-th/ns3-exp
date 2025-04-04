#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/pcap-file.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("EcnPlusPcapTest");

int
main(int argc, char* argv[])
{
    // Enable logging for debugging
    LogComponentEnable("EcnPlusPcapTest", LOG_LEVEL_INFO);
    LogComponentEnable("TcpSocketBase", LOG_LEVEL_INFO);

    Config::SetDefault("ns3::TcpSocketBase::UseEcn", EnumValue(TcpSocketState::On));


    // Create three nodes: sender, router, and receiver
    NodeContainer nodes;
    nodes.Create(3); // Node 0: Sender, Node 1: Router, Node 2: Receiver

    // Set up point-to-point links
    PointToPointHelper pointToPointSender;
    pointToPointSender.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPointSender.SetChannelAttribute("Delay", StringValue("2ms"));

    PointToPointHelper pointToPointRouter;
    pointToPointRouter.SetDeviceAttribute("DataRate", StringValue("500kbps")); // Bottleneck
    pointToPointRouter.SetChannelAttribute("Delay", StringValue("2ms"));
    pointToPointRouter.SetQueue("ns3::DropTailQueue<Packet>",
                               "MaxSize", StringValue("50p")); // Smaller queue to trigger congestion

    // Connect sender to router (5Mbps)
    NodeContainer senderToRouter(nodes.Get(0), nodes.Get(1));
    NetDeviceContainer senderToRouterDevices = pointToPointSender.Install(senderToRouter);

    // Connect router to receiver (500kbps - bottleneck)
    NodeContainer routerToReceiver(nodes.Get(1), nodes.Get(2));
    NetDeviceContainer routerToReceiverDevices = pointToPointRouter.Install(routerToReceiver);

    // Install Internet stack on all nodes
    InternetStackHelper stack;
    stack.Install(nodes);

    // Assign IP addresses
    Ipv4AddressHelper address;

    // Sender to router link
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer senderToRouterInterfaces = address.Assign(senderToRouterDevices);

    // Router to receiver link
    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer routerToReceiverInterfaces = address.Assign(routerToReceiverDevices);

    // Create a TCP sink (receiver)
    uint16_t port = 8080;
    Address sinkAddress(InetSocketAddress(routerToReceiverInterfaces.GetAddress(1), port));
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", sinkAddress);
    ApplicationContainer sinkApp = sinkHelper.Install(nodes.Get(2));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(10.0));

    // Create a TCP sender (1Mbps, exceeding the bottleneck)
    OnOffHelper senderHelper("ns3::TcpSocketFactory", sinkAddress);
    senderHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"));
    senderHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.0]"));
    senderHelper.SetAttribute("DataRate", DataRateValue(DataRate("1Mbps")));
    senderHelper.SetAttribute("PacketSize", UintegerValue(512));

    ApplicationContainer senderApp = senderHelper.Install(nodes.Get(0));
    senderApp.Start(Seconds(1.0));
    senderApp.Stop(Seconds(10.0));

    // Enable IP routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Enable PCAP tracing on both links
    pointToPointSender.EnablePcap("ecn-sender-to-router", senderToRouterDevices, false);
    pointToPointRouter.EnablePcap("ecn-router-to-receiver", routerToReceiverDevices, false);

    // Run the simulation
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    Simulator::Destroy();

    NS_LOG_INFO("Simulation completed with congestion and ECN enabled. Check pcap files for ECN behavior.");

    return 0;
}