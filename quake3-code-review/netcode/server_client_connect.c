





when I start the game, it will go through the following flow

1.					
					common.c

					void Com_Frame()
					{

						...
						...

						SV_Frame();

						if ( !com_dedicated->integer ) {

							...
							...

							Com_EventLoop();
							Cbuf_Execute ();

							...
							...

							CL_Frame( msec );

							...
							...
						}

					}


	
2.	we treat the arrival of a packet as an event


					common.c

					int Com_EventLoop()
					{

						while ( 1 ) 
						{

							ev = Com_GetEvent();



							// if no more events areCom available
							if ( ev.evType == SE_NONE ) {
								// manually send packet events for the loopback channel
								while ( NET_GetLoopPacket( NS_CLIENT, &evFrom, &buf ) ) 
								{
									CL_PacketEvent( evFrom, &buf );
								}

								while ( NET_GetLoopPacket( NS_SERVER, &evFrom, &buf ) ) {
									// if the server just shut down, flush the events
									if ( com_sv_running->integer ) 
									{
										Com_RunAndTimeServerPacket( &evFrom, &buf );
									}
								}

								return ev.evTime;
							}


							...
							...

						}
					}








3.	let us take a look at NET_GetLoopPacket(); function

	-	the netadr_t class is defined in qcommon.h

					typedef struct {
						netadrtype_t	type;

						byte	ip[4];
						byte	ipx[10];

						unsigned short	port;
					} netadr_t;

	it is the standard 4 byte struct representing a IPv4 address, with a port. 
	ipx is not really used anymore. so we will not look at it


	-	the msg_t struct is as follow

						//
					// msg.c
					//
					typedef struct {
						qboolean	allowoverflow;	// if false, do a Com_Error
						qboolean	overflowed;		// set to true if the buffer size failed (with allowoverflow set)
						qboolean	oob;			// set to true if the buffer size failed (with allowoverflow set)
						byte	*data;
						int		maxsize;
						int		cursize;
						int		readcount;
						int		bit;				// for bitwise reads and writes
					} msg_t;





					net_chan.c

					qboolean NET_GetLoopPacket( netsrc_t sock, netadr_t *net_from, msg_t *net_message )		
					{

					}







	
4.	

					common.c

					void Com_RunAndTimeServerPacket()
					{
						...
						...

						SV_PacketEvent( *evFrom, buf );

						...
						...
					}



4.					
					sv_main.c

					void SV_PacketEvent( netadr_t from, msg_t *msg )
					{

						if ( msg->cursize >= 4 && *(int *)msg->data == -1) {
							SV_ConnectionlessPacket( from, msg );
							return;
						}

						...
						...

					}





5.					
					void SV_ConnectionlessPacket( netadr_t from, msg_t *msg ) 
					{


						...
						...
						...
						
						else if (!Q_stricmp(c, "connect")) 
						{
							SV_DirectConnect( from );
						} 

						else if (!Q_stricmp(c, "ipAuthorize")) 
						{
							SV_AuthorizeIpPacket( from );
						} 

						else if (!Q_stricmp(c, "rcon")) {
							SVC_RemoteCommand( from, msg );
						} 

						else if (!Q_stricmp(c, "disconnect")) 
						{
							// if a client starts up a local server, we may see some spurious
							// server disconnect messages when their new server sees our final
							// sequenced messages to the old client
						} 
						else 
						{
							Com_DPrintf ("bad connectionless packet from %s:\n%s\n"
							, NET_AdrToString (from), s);
						}
					}


6.	we take a look at the SV_DirectConnect(); function




					sv_client.c

					void SV_DirectConnect(netadr_t from)
					{

						...
						...

					gotnewcl:	

					}


