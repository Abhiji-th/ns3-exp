#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

int main(int argc, char *argv[])
{
    // Enable ECN
    Config::SetDefault("ns3::TcpSocketBase::EcnMode", EnumValue(TcpSocketState::EcnMode_t::ClassicEcn));
    Config::SetDefault("ns3::TcpSocketBase::UseEcn", EnumValue(TcpSocketState::UseEcn_t::On));

    // Create nodes
    NodeContainer nodes;
    nodes.Create(2);

    // Set up point-to-point link
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);

    // Install internet stack
    InternetStackHelper stack;
    stack.Install(nodes);

    // Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // Create TCP sink application
    uint16_t port = 8080;
    PacketSinkHelper sink("ns3::TcpSocketFactory",
                         InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApps = sink.Install(nodes.Get(1));
    sinkApps.Start(Seconds(1.0));
    sinkApps.Stop(Seconds(10.0));

    // Create TCP sender application
    OnOffHelper onoff("ns3::TcpSocketFactory",
                     InetSocketAddress(interfaces.GetAddress(1), port));
    onoff.SetAttribute("DataRate", StringValue("1Mbps"));
    onoff.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps = onoff.Install(nodes.Get(0));
    clientApps.Start(Seconds(1.0));
    clientApps.Stop(Seconds(10.0));

    // Enable PCAP tracing
    pointToPoint.EnablePcapAll("tcp-ecn");  // Creates .pcap files for all devices

    // Enable ASCII tracing
    AsciiTraceHelper ascii;
    pointToPoint.EnableAsciiAll(ascii.CreateFileStream("tcp-ecn.tr"));  // Creates ASCII trace file

    // Set simulation time
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}