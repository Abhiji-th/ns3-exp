#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpRedExample");

int main(int argc, char* argv[])
{
    // Enable logging
    LogComponentEnable("TcpRedExample", LOG_LEVEL_INFO);

    Config::SetDefault("ns3::TcpSocket::EcnMode", StringValue("ClassicEcn"));

    // Create nodes
    NodeContainer nodes;
    nodes.Create(3); // 0 = sender, 1 = router, 2 = receiver
    
    // Create point-to-point links
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    
    // Connect sender to router
    NetDeviceContainer senderToRouter = p2p.Install(nodes.Get(0), nodes.Get(1));
    
    // Connect router to receiver
    p2p.SetDeviceAttribute("DataRate", StringValue("1Mbps")); // Bottleneck link
    NetDeviceContainer routerToReceiver = p2p.Install(nodes.Get(1), nodes.Get(2));

    // Install internet stack
    InternetStackHelper stack;
    stack.Install(nodes);

    // Configure RED queue disc
    TrafficControlHelper tch;
    tch.SetRootQueueDisc("ns3::RedQueueDisc",
                         "MeanPktSize", UintegerValue(1000),
                         "MinTh", DoubleValue(5),
                         "MaxTh", DoubleValue(15),
                         "MaxSize", QueueSizeValue(QueueSize("25p")));
    
    tch.Install(senderToRouter.Get(1));  // Install on router's sender interface
    tch.Install(routerToReceiver.Get(0)); // Install on router's receiver interface

    // Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces1 = address.Assign(senderToRouter);
    
    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces2 = address.Assign(routerToReceiver);

    // Create TCP application - sender
    uint16_t port = 8080;
    BulkSendHelper source("ns3::TcpSocketFactory",
                         InetSocketAddress(interfaces2.GetAddress(1), port));
    source.SetAttribute("MaxBytes", UintegerValue(20000)); // 2 packets of 1000 bytes each
    source.SetAttribute("SendSize", UintegerValue(1000));
    
    ApplicationContainer sourceApps = source.Install(nodes.Get(0));
    sourceApps.Start(Seconds(1.0));
    sourceApps.Stop(Seconds(10.0));

    // Create TCP sink - receiver
    PacketSinkHelper sink("ns3::TcpSocketFactory",
                         InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApps = sink.Install(nodes.Get(2));
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(10.0));

    // Enable routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Enable pcap tracing
    p2p.EnablePcapAll("tcp-red-example");

    // Run simulation
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}