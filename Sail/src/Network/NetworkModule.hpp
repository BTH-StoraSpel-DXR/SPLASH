#pragma once
#include <WS2tcpip.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <thread>
#include <mutex>

#include "NetworkStructs.hpp"
#pragma comment (lib, "ws2_32.lib")

struct Connection
{
	std::string ip;
	std::string port;
	TCP_CONNECTION_ID tcp_id;
	bool isConnected;
	bool wasKicked;
	SOCKET socket;
	std::thread* thread;//The thread used to listen for messages

	Connection() {
		tcp_id = 0;
		isConnected = false;
		wasKicked = false;
		socket = NULL;
		ip = port = "";
		thread = nullptr;
	}
};

/*
	This function needs to be implemented by the application and passed to SetupHost or SetupClient.
*/
class Network
{
public:
	enum HostFlags : USHORT
	{
		USE_RANDOM_IDS = 1U,
		//ALLOW_CLIENT_ID_REQUEST = 2U, //Allow a client to reconnect with the same id if they remember it after a disconnect
		ENABLE_LAN_SEARCH_VISIBILITY = 2U
	};

	enum INITIALIZED_STATUS : char {
		NOT_INITIALIZED = 0,
		INITIALIZED = 1,
		IS_CLIENT = 2,
		IS_SERVER = 4,
	};

	Network();
	~Network();

	/*
		Initializes winsock. Call this once.
		No need to call this again after calling shutdown() even if the network is intended to be reused for hosting / joining.
	*/
	bool initialize();

	void checkForPackages(NetworkEventHandler& handler);
	void checkForPackages(void (*m_callbackfunction)(NetworkEvent));

	/*
		Call host() to a host session. Dont call this and join() in the same application.
	*/
	bool host(unsigned short port, USHORT hostFlags = (USHORT)HostFlags::USE_RANDOM_IDS);
	/*
		Call join() to join an already hosted session. Dont call this and host() in the same application.
	*/
	bool join(const char* host_ip, unsigned short hostport);
	/*
		Allow or disallow new connections as host.
	*/
	virtual void setAllowJoining(bool b);

	/*
		Send a message to connection "receiverID".
		If SetupClient has been called, any number passed to receiverID will all send to the connected host.
		If SetupHost has been called, passing -1 to receiverID will send the message to all connected clients.

		Return true if message could be sent to all receivers.
	*/
	bool send(const char* message, size_t size, TCP_CONNECTION_ID receiverID = 0);
	/*
		Set server meta description.
		This meta data is optional and sent to the clients on lan when a client calls searchHostsOnLan().
		The meta data can then be retreived from the HOST_ON_LAN_FOUND network event(s) if any hosts exists on the lan with the host flag ENABLE_LAN_SEARCH_VISIBILITY.

		This data is broadcasted with UDP and can be used to give clients information before connection to the host.
		This can be data like server name, number of active connections etc.

		It is up to the programmer to encode and decode any message they want to pass as metadata.

		The server meta data is not guaranteed to arrive to the clients since this is sent with UDP.

		The server meta description can not be greater than 58 Bytes long.
	*/
	void setServerMetaDescription(const char* desc, int descSize);
	/*
		This function will broadcast a UDP message on the LAN requesting all hosts(if any) to identify themself.
		The NetworkEvent HOST_ON_LAN_FOUND will trigger for each host on the network that responded.

		Only hosts initialized with the ENABLE_LAN_SEARCH_VISIBILITY flag will respond to this message.
	*/
	bool searchHostsOnLan();

	/*
		Returns true if this is a Host
	*/
	bool isServer();
	INITIALIZED_STATUS getInitializeStatus();
	/*
		Shutsdown the current network setup(host or client setup). Call this if you want to reuse the network to join a new host, host again, or switch between hosting/joining.
	*/
	void shutdown();
	/*
		Expands a compressed ipv4 address into a readable char array in dotted-decimal notation.
		For example the compressed address ip = ULONG_MAX will be expanded into "255.255.255.255"

		@param ip, the compressed ipv4 address that should be expanded into a readable char array.
		@param buffer, a pointer to a char array to place the expanded ip address.
		@param buffersize, the max size of the buffer.
	*/
	static void ip_int_to_ip_string(ULONG ip, char* buffer, int buffersize);
	/*
		Compresses a ipv4 address from dotted-decimal notation to a single ULONG.
		Example1 the ip = "255.255.255.255" will be returned as ULONG_MAX
		Example2 the ip = "0.0.0.255" will be returned as 255

		@param ip, a char pointer to a ipv4 address. Exemple "192.168.1.10"
		@param buffersize, max size of the buffer passed in the ip parameter

		@return, the ip address compressed into a single int value.
	*/
	static ULONG ip_string_to_ip_int(char* ip);

	/*
		This function does not have a great documentation.
	*/
	void stopUDP();
	/*
		This function does have a great documentation.
	*/
	void startUDP();

	void kickConnection(TCP_CONNECTION_ID tcp_id);
	bool wasKicked(TCP_CONNECTION_ID tcp_id);

	size_t averagePacketSizeSinceLastCheck();
private:

	enum UDP_DATA_PACKAGE_TYPE : char
	{
		UDP_DATA_PACKAGE_TYPE_HOSTINFO = 1,
		UDP_DATA_PACKAGE_TYPE_HOSTINFO_REQUEST = 2,
	};

	struct UDP_DATA
	{
		union {
			char raw_data[MAX_PACKAGE_SIZE];
			struct {
				char packagetype;
				char padding;//needed for correct memmory alignment
				union {
					char data[MAX_PACKAGE_SIZE - 2];
					struct
					{
						USHORT port;
						char hostdescription[HOST_META_DESC_SIZE];
						char padding[2];
					} hostdata;
				} packageData;
			} package;
		};
	};
	static_assert(sizeof(UDP_DATA) == MAX_PACKAGE_SIZE, "sizeof(UDP_DATA) is not what you expect! Check your struct man.");

	bool m_shutdown = false;
	bool m_shutdownUDP = false;
	char m_serverMetaDesc[HOST_META_DESC_SIZE];
	bool m_allowConnections = true;

	//TCP CONNECTION
	SOCKET m_soc = 0;
	sockaddr_in m_myAddr = { 0 };
	std::thread* m_clientAcceptThread = nullptr;
	USHORT m_hostPort = 0;

	//UDP CONNECTION
	const USHORT m_udp_localbroadcastport = 444;
	SOCKET m_udp_broadcast_socket = 0;
	SOCKET m_udp_directMessage_socket = 0;
	sockaddr_in m_udp_broadcast_address = { 0 };
	sockaddr_in m_udp_direct_address = { 0 };

	std::thread* m_UDPListener = nullptr;

	//GENERIC
	INITIALIZED_STATUS m_initializedStatus = INITIALIZED_STATUS::NOT_INITIALIZED;

	USHORT m_hostFlags;
	TCP_CONNECTION_ID m_nextID = 0; //Only used if m_useRandomIDs == false

	/*Do not access m_connections without mutex lock*/
	std::unordered_map<size_t, Connection*> m_connections;
	std::mutex m_mutex_connections;

	NetworkEventData* m_awaitingMessages;
	NetworkEvent* m_awaitingEvents;

	int m_pstart = 0;
	int m_pend = 0;
	std::mutex m_mutex_packages;

	size_t m_nrOfPacketsSentSinceLast = 0;
	size_t m_sizeOfPacketsSentSinceLast = 0;
private:
	bool startUDPSocket(unsigned short port);
	void listenForUDP();
	bool udpSend(sockaddr* addr, char* msg, int msgSize);

	TCP_CONNECTION_ID generateID();
	bool send(const char* message, size_t size, Connection* conn);
	void addNetworkEvent(NetworkEvent n, int dataSize, const char* data = nullptr);

	/*
		Only used by the server. This function is called in a new thread and waits for new incomming connection requests.
		Accepted connections are stored in m_connections. A new thread is created for each connection directly Listen() in order to listen for incomming messages from that connection.
	*/
	void waitForNewConnections();

	/*
		This function is called by multiple threads. Listen for incomming messages from one specific connection.
		If this application is a client, only one thread will call Listen() which will listen to the host.
		If this application is a host, this function will be called by one thread for each client connected to the host.

		Host connection requests is handled in WaitForNewConnections()
	*/
	void listen(Connection* conn);//Rename this function
};

