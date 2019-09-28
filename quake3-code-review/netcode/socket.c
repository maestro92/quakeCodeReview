Previously, we have been examining the code on the packet level, here in this series, we will be looking the code at the socket level


1.	we first intialize the networking stuff in NET_Init()

					win_main.c

					int WINAPI WinMain()
					{


						NET_Init();

						...
						...


					}


2.	that calls the NET_Init(); code

					win_net.c

					void NET_Init()
					{


						// this is really just to get the cvars registered
						NET_GetCvars();

						//FIXME testing!
						NET_Config( qtrue );

					}

	we look into the NET_Config class

					win_net.c

					void NET_Config()
					{

						...
						...

						if( start ) 
						{
							if (! net_noudp->integer ) {
								NET_OpenIP();
							}
							if (! net_noipx->integer ) {
								NET_OpenIPX();
							}
						}
					}


4.	IPX is not really used anymore, so we will only focus on the NET_OpenIP(); function.
	let us take a look at what happens in there	
					
	-	you can see how ip and port are defined at the beginning.

		PORT_SERVER is defined as 

					qcomon.h

					define PORT_SERVER			27960

		so that is what our port is gonna be


		ip->string is localhost



					win_net.c

					void NET_OpenIP(void)
					{

						ip = Cvar_Get( "net_ip", "localhost", CVAR_LATCH );
						port = Cvar_Get( "net_port", va( "%i", PORT_SERVER ), CVAR_LATCH )->integer;


						for( i = 0 ; i < 10 ; i++ ) 
						{
							ip_socket = NET_IPSocket( ip->string, port + i );
							if ( ip_socket ) 
							{
								Cvar_SetValue( "net_port", port + i );
								if ( net_socksEnabled->integer ) 
								{
									NET_OpenSocks( port + i );
								}
								NET_GetLocalAddress();
								return;
							}
						}


					}




5.	we look at what NET_IPSocket does.


	-	you can see we initalize the socket by calling 

					( newsocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) == INVALID_SOCKET

		AF_INET:
			AF stands for address family. It means addresses from the internet. It is used to desginated the type
			of addresses that your socket can communicate with (in this case, Internet Protocol v4 addresses)
			when you create a socket, you have to specifiy its address family, and then you can only use addresses of that type
			with the socket. 

			The linux kernel, for example supports 29 other address families such as UNIX (AF_UNIX) sockets and IPX (AF_IPX), and also
			communications with IRDA and Bluetooth (AF_IRDA and AF_BLUETOOTH, but it is doubtful you will use these at such low level)

			for the most part, sticking with AF_NET for socket programming over a network is the safest option. There is also 
			AF_INET6 for the Internet Protocol v6 addresses.


		https://stackoverflow.com/questions/1593946/what-is-af-inet-and-why-do-i-need-it


		SOCK_DGRAM:
			this tells the socket to open up a UDP socket 


		IPPROTO_UDP:
			the IP protocol in the IP header to be received or sent. 



		apparently it is recommended that the safest way to create a UDP or a TCP IPv4 socket object is as follows:

			int sock_udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			int sock_tcp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		https://stackoverflow.com/questions/5385312/ipproto-ip-vs-ipproto-tcp-ipproto-udp

		The reason is that it is generally better to be explicit than implicit. 




	-	the second important command is to make the socket non-blocking

		apparently the nowadays, we should use O_NONBLOCK instead of FIONBIO, since it is standardized by POSIX


		https://stackoverflow.com/questions/1150635/unix-nonblocking-i-o-o-nonblock-vs-fionbio



	-	SOL_SOCKET

		used as the level argment to getsockopt or setsockopt to manipulate socket-level options 

		SO_BROADCAST
		This option controls whether datagrams may be broadcast from the socket. The value has type int; a nonzero value means "yes".

					win_net.c

					int NET_IPSocket( char *net_interface, int port ) 
					{

						SOCKET				newsocket;
						struct sockaddr_in	address;



						if( ( newsocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) == INVALID_SOCKET ) {
							err = WSAGetLastError();
							if( err != WSAEAFNOSUPPORT ) {
								Com_Printf( "WARNING: UDP_OpenSocket: socket: %s\n", NET_ErrorString() );
							}
							return 0;
						}




						// make it non-blocking
						if( ioctlsocket( newsocket, FIONBIO, &_true ) == SOCKET_ERROR ) {
							Com_Printf( "WARNING: UDP_OpenSocket: ioctl FIONBIO: %s\n", NET_ErrorString() );
							return 0;
						}



						// make it broadcast capable
						if( setsockopt( newsocket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i) ) == SOCKET_ERROR ) {
							Com_Printf( "WARNING: UDP_OpenSocket: setsockopt SO_BROADCAST: %s\n", NET_ErrorString() );
							return 0;
						}


						...
						...

						if( bind( newsocket, (void *)&address, sizeof(address) ) == SOCKET_ERROR ) {
							Com_Printf( "WARNING: UDP_OpenSocket: bind: %s\n", NET_ErrorString() );
							closesocket( newsocket );
							return 0;
						}

						return newsocket;

					}





6.	if NET_IPSocket returns a valid socket, we go ahead and call NET_OpenSocks();
	it seems like, we are calling another TCP socket here with the first socket call


					if ( ( socks_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP ) ) == INVALID_SOCKET ) 





					#define INVALID_SOCKET  (SOCKET)(~0)
					#define SOCKET_ERROR            (-1)

					win_net.c

					void NET_OpenSocks(int port)
					{

						...
						...

						if ( ( socks_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP ) ) == INVALID_SOCKET ) 
						{
							err = WSAGetLastError();
							Com_Printf( "WARNING: NET_OpenSocks: socket: %s\n", NET_ErrorString() );
							return;
						}


						h = gethostbyname( net_socksServer->string );
						if ( h == NULL ) {
							err = WSAGetLastError();
							Com_Printf( "WARNING: NET_OpenSocks: gethostbyname: %s\n", NET_ErrorString() );

							// printf("WARNING: NET_OpenSocks: gethostbyname: %s\n", NET_ErrorString());
							return;
						}

					}
							



6.	

	-	gethostname 
	
	a systemcall used to access the hostname of the current processor.
	returns the null-terminated hostname in the character array name. 

	on success, zero is returned. On error, -1 is returned, and errno(number of last error) is set appropriately
	http://man7.org/linux/man-pages/man2/gethostname.2.html
	
	
	this makes sense becuz in the code, we do check if it equals SOCKET_ERROR, which is defined as -1;



	-	gethostbyname
	returns a structure of type hostent for the given host name. 

	here name is either a hostname or an IPv4 address in standard dot notation
	http://man7.org/linux/man-pages/man3/gethostbyname.3.html


					win_net.c
					
					void NET_GetLocalAddress()
					{

						...
						...

						if( gethostname( hostname, 256 ) == SOCKET_ERROR ) {
							error = WSAGetLastError();
							return;
						}


						hostInfo = gethostbyname( hostname );
						if( !hostInfo ) {
							error = WSAGetLastError();
							return;
						}



					}


