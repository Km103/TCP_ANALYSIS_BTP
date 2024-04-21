/**This program is free software;
you can redistribute it and / or modify* it under the terms of the GNU General Public License
                                     version 2 as* published by the Free Software Foundation;
**This program is distributed in the hope that it will be useful, *but WITHOUT ANY WARRANTY; without
even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;
if not
    , write to the Free Software *Foundation, Inc., 59 Temple Place, Suite 330, Boston,
        MA 02111 - 0.07 USA **Author : George F.Riley<riley @ece.gatech.edu>
        */

#include "ns3/applications-module.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

typedef NetDeviceContainer NDC;
typedef Ipv4InterfaceContainer Ipv4IC;

int
main(int argc, char* argv[])
{
    std::string animFile = "dumbell_vegass_forward.xml";

    Config::SetDefault("ns3::OnOffApplication::PacketSize", UintegerValue(512));
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue("5Mbps"));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpVegas"));
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    NodeContainer m_leftLeaf;                       //!< Left Leaf nodes
    NetDeviceContainer m_leftLeafDevices;           //!< Left Leaf NetDevices
    NodeContainer m_rightLeaf;                      //!< Right Leaf nodes
    NetDeviceContainer m_rightLeafDevices;          //!< Right Leaf NetDevices
    NodeContainer m_routers;                        //!< Routers
    NetDeviceContainer m_routerDevices;             //!< Routers NetDevices
    NetDeviceContainer m_leftRouterDevices;         //!< Left router NetDevices
    NetDeviceContainer m_rightRouterDevices;        //!< Right router NetDevices
    Ipv4InterfaceContainer m_leftLeafInterfaces;    //!< Left Leaf interfaces (IPv4)
    Ipv4InterfaceContainer m_leftRouterInterfaces;  //!< Left router interfaces (IPv4)
    Ipv4InterfaceContainer m_rightLeafInterfaces;   //!< Right Leaf interfaces (IPv4)
    Ipv4InterfaceContainer m_rightRouterInterfaces; //!< Right router interfaces (IPv4)
    Ipv4InterfaceContainer m_routerInterfaces;      //!< Router interfaces (IPv4)

    // Create Nodes
    m_leftLeaf.Create(3);
    m_routers.Create(2);
    m_rightLeaf.Create(3);

    // helper for p2p links
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("1ms"));

    PointToPointHelper RouterHelper;
    RouterHelper.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    RouterHelper.SetChannelAttribute("Delay", StringValue("1ms"));

    m_routerDevices = RouterHelper.Install(m_routers);

    // left side links
    for (uint32_t i = 0; i < 3; ++i)
    {
        NetDeviceContainer c = p2p.Install(m_routers.Get(0), m_leftLeaf.Get(i));
        m_leftRouterDevices.Add(c.Get(0));
        m_leftLeafDevices.Add(c.Get(1));
    }

    // Add the right side links
    for (uint32_t i = 0; i < 3; ++i)
    {
        NetDeviceContainer c = p2p.Install(m_routers.Get(1), m_rightLeaf.Get(i));
        m_rightRouterDevices.Add(c.Get(0));
        m_rightLeafDevices.Add(c.Get(1));
    }

    InternetStackHelper stack;
    stack.Install(m_routers);
    stack.Install(m_leftLeaf);
    stack.Install(m_rightLeaf);

    Ipv4AddressHelper leftIp;
    Ipv4AddressHelper rightIp;
    Ipv4AddressHelper routerIp;

    leftIp.SetBase("10.1.1.0", "255.255.255.0");
    rightIp.SetBase("10.2.1.0", "255.255.255.0");
    routerIp.SetBase("10.3.1.0", "255.255.255.0");

    m_routerInterfaces = routerIp.Assign(m_routerDevices);
    // Assign to left side
    for (uint32_t i = 0; i < 3; ++i)
    {
        NetDeviceContainer ndc;
        ndc.Add(m_leftLeafDevices.Get(i));
        ndc.Add(m_leftRouterDevices.Get(i));
        Ipv4InterfaceContainer ifc = leftIp.Assign(ndc);
        m_leftLeafInterfaces.Add(ifc.Get(0));
        m_leftRouterInterfaces.Add(ifc.Get(1));
        leftIp.NewNetwork();
    }
    // Assign to right side
    for (uint32_t i = 0; i < 3; ++i)
    {
        NetDeviceContainer ndc;
        ndc.Add(m_rightLeafDevices.Get(i));
        ndc.Add(m_rightRouterDevices.Get(i));
        Ipv4InterfaceContainer ifc = rightIp.Assign(ndc);
        m_rightLeafInterfaces.Add(ifc.Get(0));
        m_rightRouterInterfaces.Add(ifc.Get(1));
        rightIp.NewNetwork();
    }

    // TCP Sender
    OnOffHelper tcpSourceHelper("ns3::TcpSocketFactory",
                                InetSocketAddress(m_rightLeafInterfaces.GetAddress(1), 50000));
    tcpSourceHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    tcpSourceHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer tcpSources;
    tcpSources.Add(tcpSourceHelper.Install(m_leftLeaf.Get(1)));
    tcpSources.Start(Seconds(1.0));
    tcpSources.Stop(Seconds(10.0));

    // TCP Reciever
    Address tcpSinkAddr(InetSocketAddress(m_rightLeafInterfaces.GetAddress(1), 50000));
    PacketSinkHelper tcpSinkHelper("ns3::TcpSocketFactory", tcpSinkAddr);

    ApplicationContainer tcpSinks;
    tcpSinks.Add(tcpSinkHelper.Install(m_rightLeaf.Get(1)));
    tcpSinks.Start(Seconds(1.0));
    tcpSinks.Stop(Seconds(10.0));

    // UDP Sender

    InetSocketAddress udpSinkAddr(m_rightLeafInterfaces.GetAddress(0), 4000);
    // std::cout << IfaceRT02.GetAddress(1) << std::endl;

    OnOffHelper udpSourceHelper("ns3::UdpSocketFactory", Address());
    udpSourceHelper.SetAttribute("Remote", AddressValue(udpSinkAddr));
    udpSourceHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    udpSourceHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer udpSources;
    udpSources.Add(udpSourceHelper.Install(m_leftLeaf.Get(0)));
    udpSources.Start(Seconds(1.0));
    udpSources.Stop(Seconds(10.0));

    PacketSinkHelper udpSinkHelper("ns3::UdpSocketFactory", udpSinkAddr);

    ApplicationContainer udpSinks = udpSinkHelper.Install(m_rightLeaf.Get(0));

    udpSinks.Start(Seconds(1.0));
    udpSinks.Stop(Seconds(10.0));

    // Another Udp

    InetSocketAddress udpSinkAddr2(m_rightLeafInterfaces.GetAddress(2), 40000);

    OnOffHelper udpSourceHelper2("ns3::UdpSocketFactory", Address());
    udpSourceHelper2.SetAttribute("Remote", AddressValue(udpSinkAddr2));
    udpSourceHelper2.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    udpSourceHelper2.SetAttribute("OffTime",
                                  StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer udpSources2;
    udpSources2.Add(udpSourceHelper2.Install(m_leftLeaf.Get(2)));
    udpSources2.Start(Seconds(1.0));
    udpSources2.Stop(Seconds(10.0));

    PacketSinkHelper udpSinkHelper2("ns3::UdpSocketFactory", udpSinkAddr2);

    ApplicationContainer udpSinks2 = udpSinkHelper2.Install(m_rightLeaf.Get(2));

    udpSinks2.Start(Seconds(1.0));
    udpSinks2.Stop(Seconds(10.0));

    // Flow monitor
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(m_routers);
    mobility.Install(m_leftLeaf);
    mobility.Install(m_rightLeaf);

    AnimationInterface anim(animFile);
    anim.EnablePacketMetadata();                                // Optional
    anim.EnableIpv4L3ProtocolCounters(Seconds(0), Seconds(10)); // Optional
    Ptr<ConstantPositionMobilityModel> r0 =
        m_routers.Get(0)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> r1 =
        m_routers.Get(1)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> e0 =
        m_leftLeaf.Get(1)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> e1 =
        m_rightLeaf.Get(1)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> t0 =
        m_leftLeaf.Get(0)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> t1 =
        m_rightLeaf.Get(0)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> t2 =
        m_leftLeaf.Get(2)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> t3 =
        m_rightLeaf.Get(2)->GetObject<ConstantPositionMobilityModel>();

    r0->SetPosition(Vector(25.0, 44.0, 0.0));
    r1->SetPosition(Vector(44.0, 44.0, 0.0));

    e0->SetPosition(Vector(0.0, 44.0, 0.0));
    e1->SetPosition(Vector(75.0, 44.0, 0.0));

    t0->SetPosition(Vector(10.0, 24.0, 0.0));
    t1->SetPosition(Vector(65.0, 24.0, 0.0));
    t2->SetPosition(Vector(10.0, 64.0, 0.0));
    t3->SetPosition(Vector(65.0, 64.0, 0.0));

    // Set up the actual simulation
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Stop(Seconds(10));
    Simulator::Run();
    std::cout << "Animation Trace file created:" << animFile << std::endl;
    Simulator::Destroy();

    flowMonitor->SerializeToXmlFile("dumbell_vegas_forward_flow.xml", true, true);
    return 0;
}