// tcp_variants_scenarios.cc
// Clean version for NS-3.43 (supports TcpNewReno + TcpVegas)

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/queue-disc.h"
#include "ns3/tcp-congestion-ops.h"

using namespace ns3;

// --- Helper: set TCP variant on a node ---
static void
SetNodeTcpVariant (Ptr<Node> node, std::string variantName)
{
  // Map missing types to NewReno
  if (variantName == "Reno" || variantName == "Tahoe") {
    std::cout << "Warning: " << variantName
              << " not available in NS-3.43, using NewReno instead." << std::endl;
    variantName = "NewReno";
  }

  TypeId tid;
  if (variantName == "NewReno") tid = TypeId::LookupByName ("ns3::TcpNewReno");
  else if (variantName == "Vegas") tid = TypeId::LookupByName ("ns3::TcpVegas");
  else NS_FATAL_ERROR ("Unsupported TCP variant in ns-3.43: " << variantName);

  node->GetObject<TcpL4Protocol>()->SetAttribute ("SocketType", TypeIdValue (tid));
}



// --- Metrics structure ---
struct Metrics {
  double throughputMbps{0.0};
  double delayMs{0.0};
  double lossPct{0.0};
};

static Metrics
ComputeMetrics (Ptr<FlowMonitor> fm, FlowMonitorHelper& helper)
{
  fm->CheckForLostPackets ();
  Metrics m{};
  double sumThroughput = 0.0;
  double sumDelay = 0.0;
  uint64_t packets = 0;
  uint64_t lost = 0;

  auto stats = fm->GetFlowStats ();
  for (auto const &kv : stats)
  {
    const FlowMonitor::FlowStats &s = kv.second;
    if (s.timeLastRxPacket > s.timeFirstTxPacket)
    {
      double t = (s.timeLastRxPacket - s.timeFirstTxPacket).GetSeconds ();
      if (t > 0.0) sumThroughput += (s.rxBytes * 8.0) / t / 1e6;
    }
    if (s.rxPackets > 0)
    {
      sumDelay += s.delaySum.GetSeconds ();
      packets  += s.rxPackets;
    }
    lost += s.lostPackets;
  }

  m.throughputMbps = sumThroughput;
  m.delayMs = (packets > 0) ? (sumDelay / packets) * 1e3 : 0.0;
  uint64_t total = 0;
  for (auto const &kv : stats) total += (kv.second.rxPackets + kv.second.lostPackets);
  m.lossPct = (total > 0) ? (100.0 * lost / total) : 0.0;
  return m;
}

int main (int argc, char *argv[])
{
  uint32_t scenario = 1;               
  std::string tcpVariantFlow1 = "Vegas"; 
  std::string queueType = "DropTail";  
  std::string cbrRate = "1Mbps";       
  uint32_t runTime = 50;               
  uint32_t cbrPktSize = 950;           
  uint32_t tcpSegSize = 1000;          
  bool verbose = false;

  CommandLine cmd;
  cmd.AddValue ("scenario", "1 or 2", scenario);
  cmd.AddValue ("variant",  "NewReno|Vegas (Flow1)", tcpVariantFlow1);
  cmd.AddValue ("queue",    "DropTail|RED", queueType);
  cmd.AddValue ("cbrRate",  "CBR rate, e.g., 5Mbps", cbrRate);
  cmd.AddValue ("runTime",  "Simulation time (s)", runTime);
  cmd.Parse (argc, argv);

  if (scenario == 2 && runTime < 100) runTime = 100;

  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (tcpSegSize));

  std::string bw = "10Mbps";
  std::string delay = "10ms";

  InternetStackHelper internet;
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue (bw));
  p2p.SetChannelAttribute ("Delay", StringValue (delay));

  TrafficControlHelper tch;
  bool useRed = (queueType == "RED");

  if (scenario == 1)
  {
    NodeContainer n; n.Create (5);
    internet.Install (n);

    NetDeviceContainer d01 = p2p.Install (n.Get(0), n.Get(1));
    NetDeviceContainer d12 = p2p.Install (n.Get(1), n.Get(2));
    NetDeviceContainer d23 = p2p.Install (n.Get(2), n.Get(3));
    NetDeviceContainer d34 = p2p.Install (n.Get(3), n.Get(4));

    if (useRed)
    {
      tch.SetRootQueueDisc ("ns3::RedQueueDisc");
      tch.Install (d01); tch.Install (d12); tch.Install (d23); tch.Install (d34);
    }

    Ipv4AddressHelper ip;
    ip.SetBase ("10.0.1.0", "255.255.255.0"); ip.Assign (d01);
    ip.SetBase ("10.0.2.0", "255.255.255.0"); ip.Assign (d12);
    ip.SetBase ("10.0.3.0", "255.255.255.0"); ip.Assign (d23);
    ip.SetBase ("10.0.4.0", "255.255.255.0"); ip.Assign (d34);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    // Flow 1: TCP (Vegas or NewReno)
    SetNodeTcpVariant (n.Get(0), tcpVariantFlow1);
    uint16_t tcpPort = 5000;
    Address sinkAddr (InetSocketAddress ("10.0.4.2", tcpPort));
    PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkAddr);
    sinkHelper.Install (n.Get (4));

    BulkSendHelper bulk ("ns3::TcpSocketFactory", sinkAddr);
    bulk.SetAttribute ("MaxBytes", UintegerValue (0));
    bulk.Install (n.Get (0));

    // Flow 2: UDP CBR
    uint16_t udpPort = 6000;
    Address udpSinkAddr (InetSocketAddress ("10.0.4.2", udpPort));
    PacketSinkHelper udpSink ("ns3::UdpSocketFactory", udpSinkAddr);
    udpSink.Install (n.Get (4));

    OnOffHelper onoff ("ns3::UdpSocketFactory", udpSinkAddr);
    onoff.SetAttribute ("DataRate", StringValue (cbrRate));
    onoff.SetAttribute ("PacketSize", UintegerValue (cbrPktSize));
    onoff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    onoff.Install (n.Get (1));
  }
  else if (scenario == 2)
  {
    NodeContainer n; n.Create (9);
    internet.Install (n);

    std::vector<NetDeviceContainer> devs;
    for (uint32_t i=0;i<8;i++) {
      devs.push_back (p2p.Install (n.Get(i), n.Get(i+1)));
    }

    if (useRed)
    {
      tch.SetRootQueueDisc ("ns3::RedQueueDisc");
      for (auto &dc : devs) { tch.Install (dc); }
    }

    Ipv4AddressHelper ip;
    for (uint32_t i=0;i<8;i++) {
      std::ostringstream base; base << "10.1." << (i+1) << ".0";
      ip.SetBase (base.str().c_str(), "255.255.255.0");
      ip.Assign (devs[i]);
    }
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    // Flow 1: TCP (user choice)
    SetNodeTcpVariant (n.Get(0), tcpVariantFlow1);
    uint16_t tcpPort1 = 7001;
    Address sink1Addr (InetSocketAddress ("10.1.7.2", tcpPort1));
    PacketSinkHelper sink1 ("ns3::TcpSocketFactory", sink1Addr);
    sink1.Install (n.Get (7));
    BulkSendHelper bulk1 ("ns3::TcpSocketFactory", sink1Addr);
    bulk1.SetAttribute ("MaxBytes", UintegerValue (0));
    bulk1.Install (n.Get (0));

    // Flow 2: fixed NewReno
    SetNodeTcpVariant (n.Get(2), "NewReno");
    uint16_t tcpPort2 = 7002;
    Address sink2Addr (InetSocketAddress ("10.1.8.2", tcpPort2));
    PacketSinkHelper sink2 ("ns3::TcpSocketFactory", sink2Addr);
    sink2.Install (n.Get (8));
    BulkSendHelper bulk2 ("ns3::TcpSocketFactory", sink2Addr);
    bulk2.SetAttribute ("MaxBytes", UintegerValue (0));
    bulk2.Install (n.Get (2));

    // Flow 3: UDP CBR
    uint16_t udpPort = 8000;
    Address udpSinkAddr (InetSocketAddress ("10.1.7.2", udpPort));
    PacketSinkHelper udpSink ("ns3::UdpSocketFactory", udpSinkAddr);
    udpSink.Install (n.Get (7));
    OnOffHelper onoff ("ns3::UdpSocketFactory", udpSinkAddr);
    onoff.SetAttribute ("DataRate", StringValue (cbrRate));
    onoff.SetAttribute ("PacketSize", UintegerValue (cbrPktSize));
    onoff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    onoff.Install (n.Get (1));
  }

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> fm = flowmon.InstallAll ();

  Simulator::Stop (Seconds (runTime));
  Simulator::Run ();

  auto m = ComputeMetrics (fm, flowmon);
  std::cout << "\n=== RESULTS ===\n"
            << "Scenario: " << scenario
            << " | Variant(Flow1): " << tcpVariantFlow1
            << " | Queue: " << queueType
            << " | CBR: " << cbrRate
            << " | RunTime: " << runTime << "s\n"
            << "Throughput (sum, Mbps): " << m.throughputMbps << "\n"
            << "Avg Delay (ms): " << m.delayMs << "\n"
            << "Loss (%): " << m.lossPct << "\n";

  Simulator::Destroy ();
  return 0;
}
