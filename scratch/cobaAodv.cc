#include <iostream>
#include <cmath>
#include "ns3/aodv-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"

using namespace ns3;

int main(int argc, char const *argv[])
{
    /* code */
    uint32_t nBiasa = 4;

    //create nodes
    NodeContainer nodes;
    nodes.Create (nBiasa);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> posAlloc = CreateObject <ListPositionAllocator>();
    posAlloc -> Add (Vector (100, 0, 0));
    posAlloc -> Add (Vector (200, 0, 0));
    posAlloc -> Add (Vector (300, 0, 0));
    posAlloc -> Add (Vector (400, 0, 0));
    mobility.SetPositionAllocator (posAlloc);
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (nodes);

    //create devices
    WifiMacHelper wifiMac;
    wifiMac.SetType ("ns3::AdhocWifiMac");
    
    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
    wifiPhy.SetChannel (wifiChannel.Create ());

    WifiHelper wifi;
    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue (0));

    NetDeviceContainer devices;
    devices = wifi.Install (wifiPhy, wifiMac, nodes);

    //Instal internet stack
    InternetStackHelper stack;
    AodvHelper aodv;
    stack.SetRoutingHelper (aodv);
    stack.Install (nodes);

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer nodesInterfaces;
    nodesInterfaces = address.Assign (devices);

    //Instal aplikasi
    //reciver
    Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), 9));
    PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", sinkLocalAddress);
    ApplicationContainer sinkApp = sinkHelper.Install (nodes.Get (3));
    sinkApp.Start (Seconds (1.0));
    sinkApp.Stop (Seconds (10.0));

    //sender
    OnOffHelper clientHelper ("ns3::UdpSocketFactory", Address ());
    //clientHelper.SetAttribute ("Remote", remoteAddress);
    ApplicationContainer clientApps = clientHelper.Install (nodes.Get (1));
    clientApps.Start (Seconds (2.0));
    clientApps.Stop (Seconds (9.0));

    Simulator::Stop (Seconds (10.0));

    AnimationInterface anim ("cobaAodvAnim.xml");

    Simulator::Run ();
    Simulator::Destroy ();

    return 0;
}
    