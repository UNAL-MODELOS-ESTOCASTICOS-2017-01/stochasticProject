#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/ipv6-header.h"

#include "ns3/csma-module.h"
#include "ns3/internet-apps-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("star with ipv6");

int main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);
  
  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  //Crear los nodos
  NS_LOG_INFO("Crear nodos");
  NodeContainer nodes;
  nodes.Create (11);

  //Agregar pila ip a todos los nodos
  NS_LOG_INFO ("Agregar pila ip");
  InternetStackHelper stack_ipv6;
  stack_ipv6.SetIpv4StackInstall (false);
  stack_ipv6.Install (nodes);

  //Establecer las conexiones
  NodeContainer net1 (nodes.Get(0), nodes.Get(1));
  NodeContainer net2 (nodes.Get(0), nodes.Get(2));
  NodeContainer net3 (nodes.Get(0), nodes.Get(3));
  NodeContainer net4 (nodes.Get(0), nodes.Get(4));
  NodeContainer net5 (nodes.Get(0), nodes.Get(5));
  NodeContainer net6 (nodes.Get(0), nodes.Get(6));
  NodeContainer net7 (nodes.Get(0), nodes.Get(7));
  NodeContainer net8 (nodes.Get(0), nodes.Get(8));
  NodeContainer net9 (nodes.Get(0), nodes.Get(9));
  NodeContainer net10 (nodes.Get(0), nodes.Get(10));

  //enlace entre los nodos punto a punto
  CsmaHelper csma;
  csma.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));

  //Hacer las conexiones
  NetDeviceContainer d1 = csma.Install (net1);
  NetDeviceContainer d2 = csma.Install (net2);
  NetDeviceContainer d3 = csma.Install (net3);
  NetDeviceContainer d4 = csma.Install (net4);
  NetDeviceContainer d5 = csma.Install (net5);
  NetDeviceContainer d6 = csma.Install (net6);
  NetDeviceContainer d7 = csma.Install (net7);
  NetDeviceContainer d8 = csma.Install (net8);
  NetDeviceContainer d9 = csma.Install (net9);
  NetDeviceContainer d10 = csma.Install (net10);
 
  //Asignar interfaces
  NS_LOG_INFO ("Asignacion de direcciones ipv6");
  Ipv6AddressHelper ipv6;

  ipv6.SetBase (Ipv6Address ("2001:1::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer i1 = ipv6.Assign (d1);

  ipv6.SetBase (Ipv6Address ("2001:2::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer i2 = ipv6.Assign (d2);

  ipv6.SetBase (Ipv6Address ("2001:3::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer i3 = ipv6.Assign (d3);

  ipv6.SetBase (Ipv6Address ("2001:4::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer i4 = ipv6.Assign (d4);

  ipv6.SetBase (Ipv6Address ("2001:5::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer i5 = ipv6.Assign (d5);

  ipv6.SetBase (Ipv6Address ("2001:6::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer i6 = ipv6.Assign (d6);

  ipv6.SetBase (Ipv6Address ("2001:7::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer i7 = ipv6.Assign (d7);

  ipv6.SetBase (Ipv6Address ("2001:8::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer i8 = ipv6.Assign (d8);

  ipv6.SetBase (Ipv6Address ("2001:9::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer i9 = ipv6.Assign (d9);

  ipv6.SetBase (Ipv6Address ("2001:10::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer i10 = ipv6.Assign (d10);

  //Servidor y aplicaciones
  uint32_t packetSize = 1024;
  uint32_t maxPacketCount = 5;
  Time interPacketInterval = Seconds (1.);
 
  Ping6Helper ping6;
  ping6.SetIfIndex (i1.GetInterfaceIndex (0));
  ping6.SetRemote (Ipv6Address::GetAllNodesMulticast ());
  ping6.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  ping6.SetAttribute ("Interval", TimeValue (interPacketInterval));
  ping6.SetAttribute ("PacketSize", UintegerValue (packetSize));
  ApplicationContainer apps = ping6.Install (nodes.Get (0));
  apps.Start (Seconds (2.0));
  apps.Stop (Seconds (10.0));

  csma.EnablePcapAll ("starwithipv6", true);

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
