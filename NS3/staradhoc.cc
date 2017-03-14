#include <fstream>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/csma-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"

#include "ns3/ipv6-header.h"
#include "ns3/internet-apps-module.h"

using namespace ns3;

int main (int argc, char *argv[])
{
	uint32_t nStars = 5;      //numero de estrellas
	uint32_t leaves = 5;      //numero de nodos hoja en cada estrella
	uint32_t lanNodes = 2;    //numero de nodos lan en la estrella??
	uint32_t stopTime = 20;   //Tiempo de pausa de la simulacion

	Config::SetDefault ("ns3::OnOffApplication::PacketSize", StringValue ("1472"));
	Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("100kb/s"));

	/* Linea de comandos para settear otros valores por consola
	 * ./waf --run "staradhoc --stars=5" --vis
	 */
	CommandLine cmd;
	cmd.AddValue ("stars", "number of stars", nStars);
	cmd.AddValue ("starNodes", "number of leaf nodes", leaves);
	cmd.AddValue ("lanNodes", "number of LAN nodes", lanNodes);
	cmd.AddValue ("stopTime", "simulation stop time (seconds)", stopTime);
	cmd.Parse (argc, argv);

	//Contenedor de nodos root estrellas
	NodeContainer stars;
	stars.Create (nStars);

	//Enlace WiFi para el root de las estrellas
	WifiHelper wifi;
	WifiMacHelper mac;
	mac.SetType ("ns3::AdhocWifiMac");
	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate54Mbps"));
	YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
	YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
	wifiPhy.SetChannel (wifiChannel.Create ());
	NetDeviceContainer starDevices = wifi.Install (wifiPhy, mac, stars);

	//Cable
	CsmaHelper csma;

	//Pila IPv6 roots
	InternetStackHelper stack_ipv6;
	stack_ipv6.SetIpv4StackInstall (false);
	stack_ipv6.Install (stars);

	//Interfaz roots
	Ipv6AddressHelper ipv6_1;
	Ipv6InterfaceContainer interStar = ipv6_1.Assign (starDevices);

	//Movilidad para cada nodo central de la estrella
	MobilityHelper mobility;
	mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
									"MinX", DoubleValue (20.0),
									"MinY", DoubleValue (20.0),
									"DeltaX", DoubleValue (20.0),
									"DeltaY", DoubleValue (20.0),
									"GridWidth", UintegerValue (5),
									"LayoutType", StringValue ("RowFirst"));
	mobility.SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
								"Bounds", RectangleValue (Rectangle (-500, 500, -500, 500)),
								"Speed", StringValue ("ns3::ConstantRandomVariable[Constant=2]"),
								"Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.2]"));
	mobility.Install (stars);

	//LAN con cables
	for (uint32_t i = 0; i < nStars; ++i)
	{
		//Contenedor LAN para la i-esima estrella
		NodeContainer newLanNodes;
		newLanNodes.Create (lanNodes - 1);
		NodeContainer lan (stars.Get (i), newLanNodes);

		//cable
		//Declaracion arriba
		csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate (5000000)));
		csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
		NetDeviceContainer lanDevices = csma.Install (lan);

		//pila
		InternetStackHelper stack_leaves;
		stack_leaves.SetIpv4StackInstall (false);
		stack_leaves.Install (newLanNodes);

		//Interface
		Ipv6AddressHelper ipv6_2;
		Ipv6InterfaceContainer interfaceipv6 = ipv6_2.Assign (lanDevices);

		//modelo de movilidad para cada nodo de lan, para que se muevan junto con el centro
		//de la estrella
		MobilityHelper mobilityLan;
		Ptr<ListPositionAllocator> subnetAlloc = CreateObject<ListPositionAllocator> ();
		for (uint32_t j = 0; j < newLanNodes.GetN (); ++j)
		{
			subnetAlloc->Add (Vector (0.0, j*10 + 10, 0.0));
		}
		mobilityLan.PushReferenceMobilityModel (stars.Get (i));
		mobilityLan.SetPositionAllocator (subnetAlloc);
		mobilityLan.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
		mobilityLan.Install (newLanNodes);

		//Servidor y aplicaciones
		uint32_t packetSize = 1024;
		uint32_t maxPacketCount = 5;
		Time interPacketInterval = Seconds (1.);

		Ping6Helper ping6;
		//ping6.SetIfIndex (interfaceipv6.GetInterfaceIndex (0));
		ping6.SetIfIndex (interStar.GetInterfaceIndex (0));
		ping6.SetRemote (Ipv6Address::GetAllNodesMulticast ());

		ping6.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
		ping6.SetAttribute ("Interval", TimeValue (interPacketInterval));
		ping6.SetAttribute ("PacketSize", UintegerValue (packetSize));

		ApplicationContainer apps2 = ping6.Install (stars.Get (i));
		apps2.Start (Seconds (2.0));
		apps2.Stop (Seconds (10.0));
	}

	//Hojas de la estrella con WiFi
	for (uint32_t i = 0; i < nStars; ++i)
	{
		//Contenedor WiFi para las hojas de la estrella
		NodeContainer stas;
		stas.Create (leaves - 1);
		NodeContainer infra (stars.Get (i), stas);

		//Enlace wifi para hojas
		WifiHelper wifiInfra;
		WifiMacHelper macInfra;
		wifiPhy.SetChannel (wifiChannel.Create ());
		std::string ssidString ("wifi-infra");
		std::stringstream ss;
		ss << i;
		ssidString += ss.str ();
		Ssid ssid = Ssid (ssidString);
		wifiInfra.SetRemoteStationManager ("ns3::ArfWifiManager");

		//setup stas
		macInfra.SetType ("ns3::StaWifiMac","Ssid", SsidValue (ssid));
		NetDeviceContainer staDevices = wifiInfra.Install (wifiPhy, macInfra, stas);
		macInfra.SetType ("ns3::ApWifiMac",
							"Ssid", SsidValue (ssid),
							"BeaconGeneration", BooleanValue (true),
							"BeaconInterval", TimeValue(Seconds(2.5)));
		NetDeviceContainer apDevices = wifiInfra.Install (wifiPhy, macInfra, stars.Get (i));
		NetDeviceContainer infraDevices (apDevices, staDevices);

		//pila
		InternetStackHelper stack_wifi;
		stack_wifi.SetIpv4StackInstall (false);
		stack_wifi.Install (stas);

		//Interface
		Ipv6AddressHelper ipv6_3;
		Ipv6InterfaceContainer interface_wifi = ipv6_3.Assign (apDevices);

		//Movilidad para cada hoja de wifi
		Ptr<ListPositionAllocator> subnetAlloc = CreateObject<ListPositionAllocator> ();
		for (uint32_t j = 0; j < infra.GetN (); ++j)
		{
			subnetAlloc->Add (Vector (0.0, j, 0.0));
		}
		mobility.PushReferenceMobilityModel (stars.Get (i));
		mobility.SetPositionAllocator (subnetAlloc);
		mobility.SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
									"Bounds", RectangleValue (Rectangle (-10, 10, -10, 10)),
									"Speed", StringValue ("ns3::ConstantRandomVariable[Constant=3]"),
									"Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.4]"));
		mobility.Install (stas);


		//aplicaciones
		uint32_t packetSize = 1024;
		uint32_t maxPacketCount = 5;
		Time interPacketInterval = Seconds (1.);

		Ping6Helper ping6;
		ping6.SetIfIndex (interStar.GetInterfaceIndex (0));
		ping6.SetRemote (Ipv6Address::GetAllNodesMulticast ());

		ping6.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
		ping6.SetAttribute ("Interval", TimeValue (interPacketInterval));
		ping6.SetAttribute ("PacketSize", UintegerValue (packetSize));

		ApplicationContainer apps3 = ping6.Install (stars.Get(i));
		apps3.Start (Seconds (2.0));
		apps3.Stop (Seconds (10.0));
	}

	/**
	 * Configurar la traza pcap
	 * modo no promiscuo del pcap
	 */
	csma.EnablePcapAll ("staradhoc", false);
	wifiPhy.EnablePcap ("staradhoc", starDevices, false);
	//wifiPhy.EnablePcap ("mixed-wireless", appSink->GetId (), 0);

	/**
	 * Aplicaciones para los nodos centrales de la estrella
	 */
	uint32_t packetSize = 1024;
	uint32_t maxPacketCount = 5;
	Time interPacketInterval = Seconds (1.);

	Ping6Helper ping6;
	ping6.SetIfIndex (interStar.GetInterfaceIndex (0));
	ping6.SetRemote (Ipv6Address::GetAllNodesMulticast ());

	ping6.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
	ping6.SetAttribute ("Interval", TimeValue (interPacketInterval));
	ping6.SetAttribute ("PacketSize", UintegerValue (packetSize));

	ApplicationContainer apps = ping6.Install (stars);
	apps.Start (Seconds (2.0));
	apps.Stop (Seconds (10.0));

	Simulator::Stop (Seconds (stopTime));
	Simulator::Run ();
	Simulator::Destroy ();
}