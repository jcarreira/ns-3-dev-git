/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

Author: Radhika Mittal
*/

#include <iostream>
#include <sstream>
#include "ns3/header.h"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/seq-ts-header.h"
#include "ns3/trace-helper.h"


#define MAXPACKETS 100
#define MAXBYTES 1024
#define BYTES 1024000
#define PKTCOUNT 1
#define LOGINTERVAL 1

using namespace ns3;
using namespace std;


NS_LOG_COMPONENT_DEFINE ("TCPTopology");



ofstream ofs1("recv.txt", ios::app); 

struct FlowsCompleted
{
    uint32_t size;
    double latency;
    double starttime;
    uint32_t flowid;
};

FlowsCompleted *flowsCompleted;
uint32_t flowsCompletedCnt;
double logTime;

Ptr<Socket> *sock1;
int NODES, LINKS;

class Sender : public Application
{
    public:
        Sender();
        virtual ~Sender();

        void Setup(Ptr<Socket> socket, Address address, uint32_t packetSize);

    private:

        virtual void StartApplication(void);
        virtual void StopApplication(void);

        void SendPacket(void);
        uint32_t GetPacketsSent();

        Ptr<Socket> m_socket;
        Address m_peer;
        uint32_t m_packetSize;
        uint32_t m_nPackets;
        DataRate m_dataRate;
        EventId m_sendEvent;
        bool m_running;
        uint32_t m_packetsSent;

};

Sender::Sender()
    : m_socket(0),
    m_peer(),
    m_packetSize(0),
    m_nPackets(0),
    m_dataRate(0),
    m_sendEvent(),
    m_running(false),
    m_packetsSent(0)
{
}

Sender::~Sender()
{
    m_socket=0;
}

void
Sender::Setup(Ptr<Socket> socket, Address address, uint32_t packetSize)
{
    m_socket = socket;
    m_peer = address;
    m_packetSize = packetSize;
    //m_nPackets = nPackets;
    //m_dataRate = dataRate;
}

void
Sender::StartApplication(void)
{
    NS_LOG_INFO("Sender StartApplication");
    m_running = true;
    m_packetsSent = 0;
    m_socket->Bind ();
    m_socket->Connect (m_peer);
    SendPacket ();
}

void
Sender::StopApplication(void)
{
    m_running = false;
    if (m_sendEvent.IsRunning())
    {
        Simulator::Cancel(m_sendEvent);
    }

    if (m_socket) {
        m_socket->Close();
    }
}

// Asynchronous callback to send the packet
void
Sender::SendPacket(void)
{
    NS_LOG_INFO("Sender SendPacket size: " << m_packetSize);
    Ptr<Packet> packet = Create<Packet>(m_packetSize);

    if(m_socket->Send(packet)==-1)
    {
        std::cout << std::endl << "!!Error in sending" 
            << std::endl << std::endl;
        exit(-1);
    }
}

uint32_t
Sender::GetPacketsSent()
{
    return m_packetsSent;
}

class TcpReceiver : public Application
{
    public:
        TcpReceiver();
        virtual ~TcpReceiver();
        uint32_t GetTotalRx () const;
        void Setup(Address address, uint32_t packetSize, double starttime, uint32_t flowid, uint8_t priority);
        Ptr<Socket> GetListeningSocket (void) const;
        std::list<Ptr<Socket> > GetAcceptedSockets (void) const;

    protected:
        virtual void DoDispose (void);

    private:
        // inherited from Application base class.
        virtual void StartApplication (void);    // Called at time specified by Start
        virtual void StopApplication (void);     // Called at time specified by Stop

        void HandleRead (Ptr<Socket>);
        void HandleAccept (Ptr<Socket>, const Address& from);
        void HandlePeerClose (Ptr<Socket>);
        void HandlePeerError (Ptr<Socket>);

        // In the case of TCP, each socket accept returns a new socket, so the
        // listening socket is stored seperately from the accepted sockets
        Ptr<Socket>     m_socket;       // Listening socket
        std::list<Ptr<Socket> > m_socketList; //the accepted sockets

        Address         m_local;        // Local address to bind to
        uint32_t        m_totalRx;      // Total bytes received
        uint32_t        m_totalBytes;
        bool            m_isFirstTime;
        double          m_firstTime;
        int m_id;
        double m_starttime;
        uint32_t        m_flowid;
        uint8_t         m_priority;
        TracedCallback<Ptr<const Packet>, const Address &> m_rxTrace;

};

TcpReceiver::TcpReceiver ()
{
    NS_LOG_FUNCTION (this);
    m_socket = 0;
    m_totalRx = 0;
}

TcpReceiver::~TcpReceiver()
{
    NS_LOG_FUNCTION (this);
}


void TcpReceiver::Setup(Address address, uint32_t totalBytes, double starttime, uint32_t flowid, uint8_t priority)
{
    m_local=address;
    m_totalBytes = totalBytes;
    m_starttime = starttime;
    m_flowid = flowid;
    m_priority = priority;
    m_isFirstTime = true;
}
uint32_t TcpReceiver::GetTotalRx () const
{
    return m_totalRx;
}

Ptr<Socket>
TcpReceiver::GetListeningSocket (void) const
{
    NS_LOG_FUNCTION (this);
    return m_socket;
}

std::list<Ptr<Socket> >
TcpReceiver::GetAcceptedSockets (void) const
{
    NS_LOG_FUNCTION (this);
    return m_socketList;
}

void TcpReceiver::DoDispose (void)
{
    NS_LOG_FUNCTION (this);
    m_socket = 0;
    m_socketList.clear ();

    // chain up
    Application::DoDispose ();
}


// Application Methods
void TcpReceiver::StartApplication ()    // Called at time specified by Start
{
//    std::cout << "TcpReceiver started " << std::endl;
    NS_LOG_FUNCTION (this);
    // Create the socket if not already
    if (!m_socket)
    {
        m_socket = Socket::CreateSocket (GetNode (), TcpSocketFactory::GetTypeId());
        m_socket->Bind (m_local);
        m_socket->Listen ();
        /*m_socket->ShutdownSend ();
          if (addressUtils::IsMulticast (m_local))
          {
          Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
          if (udpSocket)
          {
        // equivalent to setsockopt (MCAST_JOIN_GROUP)
        udpSocket->MulticastJoinGroup (0, m_local);
        }
        else
        {
        NS_FATAL_ERROR ("Error: joining multicast on a non-UDP socket");
        }
        }*/
    }

    m_socket->SetRecvCallback (MakeCallback (&TcpReceiver::HandleRead, this));
    m_socket->SetAcceptCallback (
            MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
            MakeCallback (&TcpReceiver::HandleAccept, this));
    m_socket->SetCloseCallbacks (
            MakeCallback (&TcpReceiver::HandlePeerClose, this),
            MakeCallback (&TcpReceiver::HandlePeerError, this));
}

void TcpReceiver::StopApplication ()     // Called at time specified by Stop
{
    NS_LOG_FUNCTION (this);
    while(!m_socketList.empty ()) //these are accepted sockets, close them
    {
        Ptr<Socket> acceptedSocket = m_socketList.front ();
        m_socketList.pop_front ();
        acceptedSocket->Close ();
    }
    if (m_socket)
    {
        m_socket->Close ();
        m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void TcpReceiver::HandleRead (Ptr<Socket> socket)
{
    NS_LOG_INFO ("HandleRead");
    NS_LOG_FUNCTION (this << socket);
    Ptr<Packet> packet;
    Address from;
    while (packet = socket->RecvFrom(from))
    {
        if (packet->GetSize () == 0)
        { //EOF
            break;
        }

        if (m_isFirstTime == true) {
            m_isFirstTime = false;
            m_firstTime = Simulator::Now().GetSeconds();
        }

        m_totalRx += packet->GetSize ();
        //packet->PeekHeader(h);
        /*buf = (char*)malloc(packet->GetSize());
          sz = packet->CopyData((uint8_t *)buf, packet->GetSize());
          */
        if (InetSocketAddress::IsMatchingType (from))
        {
            NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                    << "s packet sink received "
                    <<  packet->GetSize () << " bytes from "
                    << InetSocketAddress::ConvertFrom(from).GetIpv4 ()
                    << " port " << InetSocketAddress::ConvertFrom (from).GetPort ()
                    << " total Rx " << m_totalRx << " bytes");
        }
        else if (Inet6SocketAddress::IsMatchingType (from))
        {
            NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                    << "s packet sink received "
                    <<  packet->GetSize () << " bytes from "
                    << Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()
                    << " port " << Inet6SocketAddress::ConvertFrom (from).GetPort ()
                    << " total Rx " << m_totalRx << " bytes");
        }


        NS_LOG_INFO (m_totalRx << " " << m_totalBytes << " " << packet->GetSize());
        NS_LOG_INFO(((m_totalRx >= m_totalBytes)&&((m_totalRx - packet->GetSize()) < m_totalBytes)));


        if((m_totalRx >= m_totalBytes)&&((m_totalRx - packet->GetSize()) < m_totalBytes))
        {
            std::cout << std::endl << "Received all data (" << m_totalRx
                << ") at:" << Simulator::Now().GetSeconds() << " from " 
                << InetSocketAddress::ConvertFrom(from).GetIpv4 () << std::endl;
            
            NS_LOG_INFO(">>>>>>>>> Received all data (" << m_totalRx
                << ") at:" << Simulator::Now().GetSeconds() << " from " 
                << InetSocketAddress::ConvertFrom(from).GetIpv4 ());
            ofs1 << m_totalBytes << "\t" << Simulator::Now().GetSeconds() - m_starttime << "\t" << m_starttime << std::endl;
            NS_LOG_INFO("Goodput: " << m_totalBytes / m_firstTime /1024 /1024 << "MB/s");
            flowsCompleted[flowsCompletedCnt].size = m_totalBytes;
            flowsCompleted[flowsCompletedCnt].latency = Simulator::Now().GetSeconds()-m_starttime;
            flowsCompleted[flowsCompletedCnt].starttime = m_starttime;
            flowsCompleted[flowsCompletedCnt].flowid = m_flowid;
            flowsCompletedCnt++;

            //if(Simulator::Now().GetSeconds() > logTime + LOGINTERVAL)
            //{
            //    logTime = logTime + LOGINTERVAL;
            //    for(uint32_t i=0; i<flowsCompletedCnt; i++)
            //        ofs1 << flowsCompleted[i].size << "\t" << flowsCompleted[i].latency << "\t" 
            //            << flowsCompleted[i].starttime << "\t" << flowsCompleted[i].flowid << std::endl;
            //    flowsCompletedCnt = 0;
            //} 
        }

        /*if(m_totalRx > m_totalBytes)
          {
          std::cout<<"\n\nAlso Received extra (total bytes = "<<m_totalRx<<")\n\n";
          }*/

    }
}


void TcpReceiver::HandlePeerClose (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
}

void TcpReceiver::HandlePeerError (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
}


void TcpReceiver::HandleAccept (Ptr<Socket> s, const Address& from)
{
    NS_LOG_FUNCTION (this << s << from);
    s->SetRecvCallback (MakeCallback (&TcpReceiver::HandleRead, this));
    m_socketList.push_back (s);
}


uint32_t *linkutil;
static void LinkUtilLog(std::string context, Ptr<Packet const> p)
{
    if(p!=NULL)
    {
        int link = atoi(context.c_str());
        linkutil[link] += p->GetSize();
        //linkutilfs<<context<<"\t"<<p->GetSize()<<"\t"<<Simulator::Now().GetSeconds()<<"\n";
    }
}


ofstream linkutilfs("linkutil.txt", ios::app);
EventId linkutilevent;
void RecordLinkUtil()
{
    for(int i=0; i<LINKS; i++)
    {
        linkutilfs<<Simulator::Now().GetSeconds()<<"\t"<<i<<"\t"<<linkutil[i]<<"\n";
        linkutil[i] = 0;
    }
    linkutilevent = Simulator::Schedule(Seconds(0.1), RecordLinkUtil);
}




ofstream dropsofs("drops.txt", ios::app);
static void PacketDropped(std::string context, Ptr<Packet const> p)
{
    if(p!=NULL)
    {
        //MyPriorityTag tag;
        //p->PeekPacketTag(tag);
        //NS_ASSERT(tag.GetTypeId().GetName() == "ns3::MyPriorityTag");
        //dropsofs<<Simulator::Now().GetSeconds()<<"\t"<<context<<"\t"<<tag.GetId()<<"\t"<<(uint16_t)tag.GetPriority()<<"\n";
    }
}

/*ofstream cwndofs("cwnd.txt", ios::app);
  static void
  CwndChange (std::string context, uint32_t oldCwnd, uint32_t newCwnd)
  {
  cwndofs<<Simulator::Now().GetSeconds() << "\t" << newCwnd<<"\n";
  }*/

void set_device_queues(NodeContainer& nodes,
        int bufsize, int backgrounddrop)
{
    // for each node we set its devices' queues
    uint32_t queueid = 0;
    char str[100];
    for(int i=0; i<NODES; i++)
    {
        for(int j=0; j<(int)nodes.Get(i)->GetNDevices(); j++)
        {
            sprintf(str, "/NodeList/%d/DeviceList/%d/TxQueue/Id", i, j);
            Config::Set (str, UintegerValue (queueid++));
            sprintf(str, "%d", queueid-1);
            std::cout << "config queue str: " << str << std::endl;
            nodes.Get(i)->GetDevice(j)->GetObject<PointToPointNetDevice>()->GetQueue()->TraceConnect("Drop", str, MakeCallback(&PacketDropped));
            nodes.Get(i)->GetDevice(j)->GetObject<PointToPointNetDevice>()->GetQueue()->SetAttribute("MaxBytes", UintegerValue(bufsize));
            //nodes.Get(i)->GetDevice(j)->GetObject<PointToPointNetDevice>()->GetQueue()->SetAttribute("BackgroundDrop", DoubleValue(backgrounddrop));
        }
    }

}

void setMaxBytes(NodeContainer& nodes, int device, int host, unsigned long long int max_bytes ) {
        nodes.Get(host)->GetDevice(device)->GetObject<PointToPointNetDevice>()
            ->GetQueue()->SetAttribute("MaxBytes", UintegerValue(max_bytes));
}

int main (int argc, char *argv[])
{
    char workload[200], topofile[200], endhostfile[200];
    uint32_t initcwnd_base = 1;
    double endtime = 100.5;
    uint32_t bufsize = 20000; // 1MB
    //uint32_t bufsize = 50000000; // 47 MB
    //bool useP2 = 0;
    //bool multipriorities = 0;
    bool logCleanUp = 0;
    bool flushOut = 0;
    //double backgrounddrop = 0;
    //uint32_t prioritySlots = 4;

    Packet::EnableChecking();
    //LogComponentEnable ("Ipv4GlobalRouting", LOG_LEVEL_INFO);
    //LogComponentEnable ("Ipv4StaticRouting", LOG_LEVEL_INFO);
    //LogComponentEnable ("TcpSocketBase", LOG_LEVEL_ALL);
    //LogComponentEnable ("TcpRC3Sack", LOG_LEVEL_ALL);
    //LogComponentEnable ("TcpTxBuffer", LOG_LEVEL_ALL);  
                    
    ofs1 << "#size" << "\t" << "latency" << "\t" 
        << "starttime" << "\t" << "flowid" << std::endl;

    CommandLine cmd;
    cmd.AddValue("workload", "Workload", workload);
    cmd.AddValue("topofile", "Contains topology file with delay bandwidth", topofile);
    cmd.AddValue("endhostfile", "file containing the end hosts", endhostfile);
    cmd.AddValue("icwbase", "Initial Congestion Window for base TCP", initcwnd_base);
    cmd.AddValue("bufsize", "Queue Buffer Size", bufsize);
    cmd.AddValue("endtime", "End time", endtime);
    //cmd.AddValue("useP2", "Use aggressive priority 2 traffic", useP2);
    //cmd.AddValue("multipriorities", "multipriorities", multipriorities);
    //cmd.AddValue("prioritySlots", "prioritySlots", prioritySlots);
    cmd.AddValue("logCleanUp", "logCleanUp", logCleanUp);
    //cmd.AddValue("backgrounddrop", "backgrounddrop", backgrounddrop);
    cmd.AddValue("flushOut", "flushOut", flushOut);
    cmd.Parse(argc, argv); 


    Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (2048000000));
    Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (2048000000));
    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1460));

    // what should be here?
    //Config::SetDefault ("ns3::TcpSocket::SlowStartThreshold", UintegerValue (0xffffffff));
    
    //Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpRC3Sack"));
    //Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));
    
    Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));
    Config::SetDefault ("ns3::RttEstimator::InitialEstimation", TimeValue (Seconds (1)));
    //Config::SetDefault ("ns3::TcpRC3Sack::LimitedWindow", BooleanValue(false));
    //Config::SetDefault ("ns3::TcpRC3Sack::LogRTO", BooleanValue(true));
    //Config::SetDefault ("ns3::TcpRC3Sack::LogCleanUp", BooleanValue(logCleanUp));
    //Config::SetDefault ("ns3::TcpRC3Sack::UseP2", BooleanValue(useP2));
    //Config::SetDefault ("ns3::TcpRC3Sack::FlushOut", BooleanValue(flushOut));
    //Config::SetDefault ("ns3::TcpRC3Sack::MultiPriorities", BooleanValue(multipriorities));
    //Config::SetDefault ("ns3::TcpRC3Sack::PrioritySlots", UintegerValue(prioritySlots));

    printf("Opening files %s and %s\n", topofile, endhostfile);

    FILE *fp = fopen(topofile, "r");
    FILE *fp2 = fopen(endhostfile,"r");

    int err;

    if (!fp || !fp2) {
        puts("Error opening files.");
        return -1;
    }

    //reading topology
    //core nodes

    // handle comments in the topology file
    char line[1000];
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (line[0] != '#') {
            std::istringstream iss(line);
            iss >> NODES;
            break;
        }
    }

    //err = fscanf(fp, "%d", &NODES);
    err = fscanf(fp, "%d", &LINKS);

    std::cout << "Read: " << NODES<<", "<<LINKS << std::endl;

    NodeContainer nodes;
    nodes.Create(NODES);

    NodeContainer p2p[LINKS];

    int linkDelays[LINKS];
    int linkBandwidths[LINKS];

    // set up peer to peer links
    // each link connects two nodes
    int n1, n2;
    for(int i = 0; i < LINKS; i++)
    {
        err=fscanf(fp, "%d", &n1);
        err=fscanf(fp, "%d", &n2);
        err=fscanf(fp, "%d", &linkBandwidths[i]);
        err=fscanf(fp, "%d", &linkDelays[i]);

        p2p[i].Add(nodes.Get(n1)); 
        p2p[i].Add(nodes.Get(n2)); 
    }

    std::cout << "Read the links with delay-bandwidth" << std::endl;
    //end host

    //adding the links that were read


    PointToPointHelper pointToPoint[LINKS];
    NetDeviceContainer devices[LINKS];
    linkutil = new uint32_t[LINKS];

    // build one point to point helper for each link we have
    // configure the helper with right data rate and delay from the topology file
    for(int i = 0; i < LINKS; i++)
    {
        char str[100];

        //sprintf(str, "logs/tcp_log%d.tr", i);
        //AsciiTraceHelper ascii;
        //pointToPoint[i].EnablePcap("prefix");

        //setting link characteristics
        //pointToPoint[i].SetQueue("ns3::PriorityQueue");
        sprintf(str, "%dMbps", linkBandwidths[i]);
        pointToPoint[i].SetDeviceAttribute ("DataRate", StringValue (str));
        sprintf(str, "%dms", linkDelays[i]);
        //pointToPoint[i].SetChannelAttribute ("Delay", TimeValue (MicroSeconds(linkDelays[i])));
        pointToPoint[i].SetChannelAttribute ("Delay", StringValue (str));

        // Installing in device
        // p2p[i] has two nodes
        devices[i] = pointToPoint[i].Install (p2p[i]);

        // for logging link utilization
        linkutil[i] = 0;
        sprintf(str, "%d", i);
        devices[i].Get(0)->GetObject<PointToPointNetDevice> () -> TraceConnect("MacTx", str, MakeCallback(&LinkUtilLog));
        sprintf(str, "%d", i);
        devices[i].Get(1)->GetObject<PointToPointNetDevice> () -> TraceConnect("MacTx", str, MakeCallback(&LinkUtilLog));
    }

    std::cout << "Assigned delay and bandiwdth" << std::endl;

    // set nodes device queues
    // maxbufsize = bufsize
    // backgrounddrop = backgrounddrop
    set_device_queues(nodes, bufsize, 0);
    //set_device_queues(nodes, bufsize, backgrounddrop);

    std::cout << "Building stack" << std::endl;

    InternetStackHelper stack;
    stack.Install(nodes);

    // Setting addresses
    Ipv4AddressHelper address[LINKS];
    Ipv4Address baseAddress("10.0.0.0");
    uint32_t baseAddressNo = baseAddress.Get();
    uint32_t addressNo;
    for(int i = 0;i < LINKS;i++)
    {
        addressNo = baseAddressNo + (i*256);
        baseAddress.Set(addressNo);
        address[i].SetBase(baseAddress, "255.255.255.0");
    }

    // we assign the addresses to the devices and get interfaces
    Ipv4InterfaceContainer interfaces[LINKS];
    for(int i = 0;i < LINKS;i++) {
        interfaces[i] = address[i].Assign (devices[i]);
    }

    std::cout << "Setting interfaces" << std::endl;

    int host_interfaces[NODES];
    int host_interfaceIdx[NODES];
    int numhosts;
    int host, link, idx;
    if (fscanf(fp2, "%d", &numhosts) != 1) {
        puts("Error reading file");
        exit(-1);
    }

    std::cout << numhosts << std::endl;
    for(int i = 0; i < numhosts; i++)
    {
        if (fscanf(fp2, "%d %d %d", &host, &link, &idx) != 3)
        {
            puts("Error reading file");
            exit(-1);
        }
        host_interfaces[host] = link;
        host_interfaceIdx[host] = idx;

        // set 0th device of host max bytes'
        //setMaxBytes(nodes, 0, host, 1024*5);
        setMaxBytes(nodes, 0, host, 2048000000);
    }

    std::cout << "Read the end hosts" << std::endl;
    std::cout << "\n\n\nCore addresses set" 
        << std::endl << std::endl;

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    std::cout << "\n\n\nAll routes included...I am all set :)" 
        << std::endl << std::endl;

    FILE *fp3 = fopen(workload,"r");
    if (!fp3) {
        puts("Error opening workload file");
        return -1;
    }

    uint32_t num;
    if (fscanf(fp3, "%d", &num) != 1) {
        puts("Error reading workload file");
        return -1;
    }

    Ptr<Sender> *sendapp1 = new Ptr<Sender>[num];
    Ptr<TcpReceiver> *recvapp1 = new Ptr<TcpReceiver>[num];
    sock1 = new Ptr<Socket>[num];

    flowsCompleted = new FlowsCompleted[num];
    flowsCompletedCnt = 0;
    logTime = 0;

    uint16_t ports[NODES];
    for (int i = 0; i < NODES; i++)
        ports[i] = 1;

    RecordLinkUtil();

    std::cout << "Reading flows.." << std::endl;
    for(uint32_t i = 0; i < num; i++)
    {
        double starttime;
        uint32_t size, sender, dest;

        if (fscanf(fp3, "%lf %d %d %d", &starttime, &size, &sender, &dest) != 4) {
            puts("Error reading workload file");
            return -1;
        }
    
        if (i % 1000 == 0 || (num < 100)) {
            std::cout << "Flow starttime: " << starttime << " size: " << size 
            << " sender: " << sender << std::endl;
        }

        if(starttime < endtime)
        {
            //std::cout << "dest: " << dest << std::endl;
            //std::cout << "host_interfaces[dest]: " << host_interfaces[dest] << std::endl;
            //std::cout << "host_interfaceIdx[dest]: " << host_interfaceIdx[dest] << std::endl;
            //std::cout << "host_interfaces size: " << NODES << std::endl;
            //std::cout << "host_interfaceIdx size: " << NODES << std::endl;
            //std::cout << "ports size: " << NODES << std::endl;

            if (host_interfaces[dest] > LINKS) {
                std::cerr <<"ERROR. host_interfaces[dest] > LINKS" << std::endl;
                return -1;
            }
            Address sinkAddress(InetSocketAddress(interfaces[host_interfaces[dest]].GetAddress(host_interfaceIdx[dest]), ports[dest]++));

            sendapp1[i] = CreateObject<Sender>();
            sock1[i] = Socket::CreateSocket(nodes.Get(sender), TcpSocketFactory::GetTypeId());
            //sock1[i] -> SetAttribute("FlowId", UintegerValue (i));
            //sock1[i] -> SetAttribute("FlowSize", UintegerValue (size));
            //sock1[i] -> SetAttribute("Priority", UintegerValue (0));
            sock1[i] -> SetAttribute("InitialCwnd", UintegerValue (initcwnd_base));
            //sock1[i] -> SetAttribute("DeviceQueue", PointerValue(nodes.Get(sender)->GetDevice(0)->GetObject<PointToPointNetDevice>()->GetQueue()));
            //sock1[i] -> TraceConnect("CongestionWindow", "Cwind", MakeCallback(&CwndChange));
            sendapp1[i]->Setup(sock1[i], sinkAddress, size);
            nodes.Get(sender)->AddApplication(sendapp1[i]);
            sendapp1[i]->SetStartTime(Seconds(starttime));
            sendapp1[i]->SetStopTime(Seconds(endtime));

            recvapp1[i] = CreateObject<TcpReceiver>();
            recvapp1[i]->Setup(sinkAddress, size, starttime, i, 0);
            nodes.Get(dest)->AddApplication(recvapp1[i]);
            recvapp1[i]->SetStartTime(Seconds(0.));
            recvapp1[i]->SetStopTime(Seconds(endtime));
        } else {
            //ignore entry
            //puts("Start time after endtime");
            //return -1;
        }
    }

    if(err == -1) {
        std::cout << "Error" << std::endl;
        return -1;
    }

    // Flow Monitor
    /*Ptr<FlowMonitor> flowmon;
      FlowMonitorHelper flowmonHelper;
      flowmon = flowmonHelper.InstallAll ();*/

    //PcapHelperForDevice helper;
    //helper.EnablePcap("prefix", nodes);

    //AsciiTraceHelper ascii;
    //pointToPoint.EnableAsciiInternal(ascii.CreateFileStream("tcptopo.tr"));
    PointToPointHelper pointToPoint2;
    pointToPoint2.EnablePcapAll("tcptopo");
    Simulator::Stop(Seconds(endtime));

    std::cout << "Starting simulation" << std::endl;
    Simulator::Run ();
    //flowmon->SerializeToXmlFile ("tcptopo.flowmon", false, false);
    Simulator::Destroy ();

    for(uint32_t i = 0; i < flowsCompletedCnt; i++) {
        ofs1 << flowsCompleted[i].size <<"\t" 
            << flowsCompleted[i].latency <<"\t"
            << flowsCompleted[i].starttime <<"\t"
            << flowsCompleted[i].flowid << std::endl;
    }
        
    std::cout << "Simulation terminated successfully" << std::endl;
    std::cout << "flowsCompletedCnt: " <<  flowsCompletedCnt << std::endl;

    return 0;
}

