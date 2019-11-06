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

NS_LOG_COMPONENT_DEFINE ("Normal");

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
    c.Create(10);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> posAlloc = CreateObject <ListPositionAllocator>();
    posAlloc -> Add (Vector (100, 100, 0));//node 1
    posAlloc -> Add (Vector (250, 100, 0));//node 2
    posAlloc -> Add (Vector (400, 100, 0));//node 3
    posAlloc -> Add (Vector (550, 100, 0));//node 4
    posAlloc -> Add (Vector (700, 100, 0));//node 5
    posAlloc -> Add (Vector (100, 250, 0));//node 6
    posAlloc -> Add (Vector (250, 250, 0));//node 7
    posAlloc -> Add (Vector (400, 250, 0));//node 8
    posAlloc -> Add (Vector (550, 250, 0));//node 9
    posAlloc -> Add (Vector (700, 250, 0));//node 10

    mobility.SetPositionAllocator (posAlloc);
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
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
    InternetStackHelper internet;
    internet.SetRoutingHelper (aodv);
    internet.Install (c);

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer nodeInterfaces;
    nodeInterfaces = address.Assign (devices);

    //aplikasi
    //reciver
    
    NS_LOG_INFO ("Create Applications.");

    Address sinkAddress (InetSocketAddress (nodeInterfaces.GetAddress (9), 6));
    PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 6));
    ApplicationContainer sinkApps = packetSinkHelper.Install (c.Get (9));
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

    AnimationInterface anim ("normalAodv2.xml");
    anim.UpdateNodeColor (c.Get (1), 0, 255, 0);//source
    anim.UpdateNodeColor (c.Get (9), 0, 0, 255);//destination
    
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

    monitor->SerializeToXmlFile("simulation2.flowmon", true, true);

    return 0;
}
