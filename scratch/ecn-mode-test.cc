#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/log.h"
#include "ns3/tcp-dctcp.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("EcnModeVerification");

// Callback to log EcnMode from a socket
void
LogEcnMode(Ptr<TcpSocketBase> socket)
{
    NS_LOG_INFO("Socket EcnMode: " << static_cast<int>(socket->GetEcnMode()));
    std::cout << Simulator::Now().GetSeconds() << "s: Socket EcnMode = "
              << static_cast<int>(socket->GetEcnMode()) << std::endl;
}

int
main(int argc, char* argv[])
{
    // Enable logging
    LogComponentEnable("EcnModeVerification", LOG_LEVEL_INFO);
    LogComponentEnable("TcpSocketBase", LOG_LEVEL_INFO); // For SetEcnMode logging

    // Command-line argument to test different ECN modes
    std::string ecnMode = "ClassicEcn"; // Default mode
    CommandLine cmd;
    cmd.AddValue("ecnMode", "ECN mode to test (ClassicEcn, DctcpEcn, EcnPlus)", ecnMode);
    cmd.Parse(argc, argv);

    // Set global ECN mode configuration
    Config::SetDefault("ns3::TcpSocketBase::EcnMode", StringValue(ecnMode));
    // Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpDctcp::GetTypeId()));

    // Create nodes
    NodeContainer nodes;
    nodes.Create(2); // Sender (0), Receiver (1)

    // Create point-to-point link
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer devices = p2p.Install(nodes.Get(0), nodes.Get(1));

    // Install internet stack
    InternetStackHelper stack;
    stack.Install(nodes);

    // Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // Create TCP sender
    uint16_t port = 8080;
    BulkSendHelper source("ns3::TcpSocketFactory",
                          InetSocketAddress(interfaces.GetAddress(1), port));
    source.SetAttribute("MaxBytes", UintegerValue(10000)); // Small transfer for simplicity
    ApplicationContainer sourceApps = source.Install(nodes.Get(0));
    sourceApps.Start(Seconds(1.0));
    sourceApps.Stop(Seconds(2.0));

    // Log EcnMode from the sender's socket
    Ptr<Application> app = sourceApps.Get(0);
    Simulator::Schedule(Seconds(1.1), [app]() {
        Ptr<BulkSendApplication> bulkApp = DynamicCast<BulkSendApplication>(app);
        if (bulkApp)
        {
            Ptr<Socket> socket = bulkApp->GetSocket();
            Ptr<TcpSocketBase> tcpSocket = DynamicCast<TcpSocketBase>(socket);
            if (tcpSocket)
            {
                LogEcnMode(tcpSocket);
            }
            else
            {
                NS_LOG_ERROR("Failed to cast socket to TcpSocketBase");
            }
        }
        else
        {
            NS_LOG_ERROR("Failed to cast application to BulkSendApplication");
        }
    });

    // Create TCP sink
    PacketSinkHelper sink("ns3::TcpSocketFactory",
                          InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApps = sink.Install(nodes.Get(1));
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(2.0));

    // Enable routing (not strictly necessary for single link, but included for completeness)
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Run simulation
    Simulator::Stop(Seconds(2.0));
    NS_LOG_INFO("Starting simulation with EcnMode set to " << ecnMode);
    Simulator::Run();
    Simulator::Destroy();

    NS_LOG_INFO("Simulation completed");
    return 0;
}