#include "ns3/aodv-module.h"
#include "ns3/netanim-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/mobility-module.h"
#include "myapp.h"

NS_LOG_COMPONENT_DEFINE ("BlackholeDirection");

using namespace ns3;

void RecivePacket(Ptr<const Packet> p, const Address & addr)
{
    std::cout << Simulator::Now ().GetSeconds () << "\t" << p->GetSize() <<"\n";
}

int main(int argc, char const *argv[])
{
    bool enableFlowMonitor = false;
    std::string phyMode ("DsssRate1Mbps");

    // Setup flow monitor
    CommandLine cmd;
    cmd.AddValue ("EnableMonitor", "Enable Flow Monitor", enableFlowMonitor);
    cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
    
    //Membuat node dan menentukan posisi
    NS_LOG_INFO ("Create nodes.");
    NodeContainer c;
    NodeContainer not_malicious;
    NodeContainer malicious;
    c.Create(20);

    malicious.Add(c.Get(12));
    not_malicious.Add(c.Get(1));
    not_malicious.Add(c.Get(2));
    not_malicious.Add(c.Get(3));
    not_malicious.Add(c.Get(4));
    not_malicious.Add(c.Get(5));
    not_malicious.Add(c.Get(6));
    not_malicious.Add(c.Get(7));
    not_malicious.Add(c.Get(8));
    not_malicious.Add(c.Get(9));

    not_malicious.Add(c.Get(10));
    not_malicious.Add(c.Get(11));
    not_malicious.Add(c.Get(0));
    not_malicious.Add(c.Get(13));
    not_malicious.Add(c.Get(14));
    not_malicious.Add(c.Get(15));
    not_malicious.Add(c.Get(16));
    not_malicious.Add(c.Get(17));
    not_malicious.Add(c.Get(18));
    not_malicious.Add(c.Get(19));
    
    ObjectFactory pos;
    pos.SetTypeId("ns3::RandomRectanglePositionAllocator");
    pos.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=500.0]"));
    pos.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=500.0]"));
    
    MobilityHelper mobility;
    Ptr<PositionAllocator> posAlloc = pos.Create() -> GetObject <PositionAllocator>();

    mobility.SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
                                "Speed", StringValue ("ns3::UniformRandomVariable[Min=0|Max=60]"),
                                "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                                "Bounds", RectangleValue (Rectangle (0, 500, 0, 500)));
    mobility.SetPositionAllocator (posAlloc);
    mobility.Install (c);

    //Membuat device
    WifiHelper wifi;

    YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
    wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11);

    YansWifiChannelHelper wifiChannel ;
    wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss ("ns3::TwoRayGroundPropagationLossModel",
                                        "SystemLoss", DoubleValue(1),
                                        "HeightAboveZ", DoubleValue(1.5));

    // For range near 250m
    wifiPhy.Set ("TxPowerStart", DoubleValue(33));
    wifiPhy.Set ("TxPowerEnd", DoubleValue(33));
    wifiPhy.Set ("TxPowerLevels", UintegerValue(1));
    wifiPhy.Set ("TxGain", DoubleValue(0));
    wifiPhy.Set ("RxGain", DoubleValue(0));
    wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue(-61.8));
    wifiPhy.Set ("CcaMode1Threshold", DoubleValue(-64.8));

    wifiPhy.SetChannel (wifiChannel.Create ());

    WifiMacHelper wifiMac;
    wifiMac.SetType ("ns3::AdhocWifiMac");

    // Set 802.11b standard
    wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                    "DataMode",StringValue(phyMode),
                                    "ControlMode",StringValue(phyMode));


    NetDeviceContainer devices;
    devices = wifi.Install (wifiPhy, wifiMac, c);

    //Instal internet
    AodvHelper aodv;
    AodvHelper malicious_aodv;
    
    //matikan hallo
    //aodv.Set("EnableHello", BooleanValue (false));
    malicious_aodv.Set("EnableHello", BooleanValue (false));

    InternetStackHelper internet;
    internet.SetRoutingHelper (aodv);
    internet.Install (not_malicious);
    
    malicious_aodv.Set("IsMalicious",BooleanValue(true));
    internet.SetRoutingHelper (malicious_aodv);
    internet.Install (malicious);

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer nodeInterfaces;
    nodeInterfaces = address.Assign (devices);


    //aplikasi
    //reciver
    
    NS_LOG_INFO ("Create Applications.");

    Address sinkAddress (InetSocketAddress (nodeInterfaces.GetAddress (15), 6));
    PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 6));
    ApplicationContainer sinkApps = packetSinkHelper.Install (c.Get (15));
    sinkApps.Start (Seconds (0.0));
    sinkApps.Stop (Seconds (50.0));


    //sender
    Ptr<Socket> ns3UdpSocket = Socket::CreateSocket (c.Get (1), UdpSocketFactory::GetTypeId ());
    Ptr<MyApp> app = CreateObject<MyApp> ();
    app -> Setup (ns3UdpSocket, sinkAddress, 1040, 100, DataRate ("250Kbps"));
    c.Get (1) -> AddApplication (app);
    
    app -> SetStartTime (Seconds (2.0));
    app -> SetStopTime (Seconds (50.0));
    
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    
    Simulator::Stop (Seconds (50.0));

    //membuat tampilan
    AnimationInterface anim ("blackholeAodvDirection.xml");
    anim.UpdateNodeColor (c.Get (1), 0,255,0);//source
    anim.UpdateNodeColor (c.Get (15), 0,0,255);//destination
    anim.UpdateNodeColor (c.Get (12), 0, 0, 0);//malicious


    
    Simulator::Run();

    monitor->CheckForLostPackets ();

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
        {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
        if ((t.sourceAddress=="10.1.1.2"))
        {
            std::cout << "Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
            std::cout << "  Tx Bytes    :   " << i->second.txBytes << "\n";
            std::cout << "  Rx Bytes    :   " << i->second.rxBytes << "\n";
            std::cout << "  Delivery    :   " << (float)i->second.rxBytes / (float)i->second.txBytes * 100 << "%\n";
            std::cout << "  Throughput  :   " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())/1024/1024 << " Mbps\n";
            std::cout << "  Delay       :   " << i->second.delaySum << "\n";
            std::cout << "  Packet Loss :   " << i->second.lostPackets << "\n";
        }
        }

    monitor->SerializeToXmlFile("simulationBlackDirection1.flowmon", true, true);

    return 0;
}
