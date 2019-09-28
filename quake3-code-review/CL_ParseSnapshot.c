
As said, all network messages takes the form of events in quake3. 






1.	you can see that if we get an SE_NONE event, we go into the while loop

	-	I did no understand what loopback was, so I had to google about it.

		 Loopback is a communication channel with only one endpoint. 
		 TCP/IP networks specify a loopback that allows client software to communicate 
		 with server software on the same computer. users can specify an IP address, 
		 usually 127.0.0.1, which will point back to the computer_s TCP/IP network configuration. 
		 The range of addresses for loopback functionality is the range of 127.0.0.0 to 127.255.255.255. Similar to ping, loopback enables a user to test 
		 one_s own network to ensure the IP stack is functioning properly.


	-	so what is happening here is that if we do not have any events in the queue, we manually send an event to ourself (hence the loopback)
		so the CL_PacketEvent(){} cand read something.

					common.c

					int Com_EventLoop( void ) 
					{

						while ( 1 ) {
							ev = Com_GetEvent();

							// if no more events are available
							if ( ev.evType == SE_NONE ) {
								// manually send packet events for the loopback channel
								while ( NET_GetLoopPacket( NS_CLIENT, &evFrom, &buf ) ) {
									CL_PacketEvent( evFrom, &buf );
								}

							...
							...
							...
							...

						}



						...
						...
						...
						...


					}








2.					

					/*
					=================
					CL_PacketEvent

					A packet has arrived from the main event loop
					=================
					*/
					CL_PacketEvent()
					{

						...
						...

						clc.lastPacketTime = cls.realtime;
						CL_ParseServerMessage( msg );

						...
						...
					}




4.					

					cl_parse.c

					CL_ParseServerMessage()
					{


						...
						...



						// get the reliable sequence acknowledge number
						clc.reliableAcknowledge = MSG_ReadLong( msg );
						// 
						if ( clc.reliableAcknowledge < clc.reliableSequence - MAX_RELIABLE_COMMANDS ) {
							clc.reliableAcknowledge = clc.reliableSequence;
						}





						whlie(1)
						{


							...
							...


						// other commands
							switch ( cmd ) {
							default:
								Com_Error (ERR_DROP,"CL_ParseServerMessage: Illegible server message\n");
								break;			
							case svc_nop:
								break;
							case svc_serverCommand:
								CL_ParseCommandString( msg );
								break;
							case svc_gamestate:
								CL_ParseGamestate( msg );
								break;
							case svc_snapshot:
								CL_ParseSnapshot( msg );
								break;
							case svc_download:
								CL_ParseDownload( msg );
								break;
							}
						}

					}




####################################################################################
####################### CL_ParseSnapshot(){} #######################################
####################################################################################

1.	now we will how the Client reads in a msg that has a snapshot information

	so the game apparently defines a new struct for the client to store snapshot information. 
	this is obviously different from the one defined for the server (ClientSnapshot_t in server.h)
	the it is also using a circular buffer (circular buffer being parseEntities)

	and the array of snapshots stores information about accessing information into parseEntities;

	
					client.h

					typedef struct {
						int			timeoutcount;		// it requres several frames in a timeout condition
														// to disconnect, preventing debugging breaks from
														// causing immediate disconnects on continue
						clSnapshot_t	snap;			// latest received from server

						...
						...

						int			serverId;			// included in each client message so the server
																	// can tell if it is for a prior map_restart
						// big stuff at end of structure so most offsets are 15 bits or less
						clSnapshot_t	snapshots[PACKET_BACKUP];

						entityState_t	entityBaselines[MAX_GENTITIES];	// for delta compression when not in previous frame

						entityState_t	parseEntities[MAX_PARSE_ENTITIES];
					} clientActive_t;



					client.h

					// snapshots are a view of the server at a given time
					typedef struct {
						qboolean		valid;			// cleared if delta parsing was invalid
						
						...
						...

						int				messageNum;		// copied from netchan->incoming_sequence

						...
						...

						int				cmdNum;			// the next cmdNum the server is expecting
						playerState_t	ps;						// complete information about the current player at this time

						int				numEntities;			// all of the entities that need to be presented
						int				parseEntitiesNum;		// at the time of this snapshot

						int				serverCommandNum;		// execute all commands up to this before
																// making the snapshot current
					} clSnapshot_t;















		you can see some of the important information stored in here, including the player state, and the number of entities for a particular snapshop


	-	you can also see that this whole time, we are assigning to the variable cl.snap

		cl is actually of type clientActive_t, declared as extern variable, meaning this will be used throughout


					typedef struct {

						...
						...

						clSnapshot_t	snap;			// latest received from server
					
						...
						...

					} clientActive_t;

					extern	clientActive_t		cl;



6.	we take a look at how the snapshot is parsed.
	since we are doing deltas, we get 


					cl_parse.c

					void CL_ParseSnapshot( msg_t *msg )
					{

						clSnapshot_t	*old;
						clSnapshot_t	newSnap;
						int			deltaNum;


						...
						...

						newSnap.serverTime = MSG_ReadLong( msg );

						...
						...


						deltaNum = MSG_ReadByte( msg );
						if ( !deltaNum ) {
							newSnap.deltaNum = -1;
						} else {
							newSnap.deltaNum = newSnap.messageNum - deltaNum;
						}




						old = &cl.snapshots[newSnap.deltaNum & PACKET_MASK];


						// read playerinfo
						SHOWNET( msg, "playerstate" );
						if ( old ) {
							MSG_ReadDeltaPlayerstate( msg, &old->ps, &newSnap.ps );
						} else {
							MSG_ReadDeltaPlayerstate( msg, NULL, &newSnap.ps );
						}



						// read packet entities
						SHOWNET( msg, "packet entities" );
						CL_ParsePacketEntities( msg, old, &newSnap );


						...
						...
						...
						...


						// copy to the current good spot
						cl.snap = newSnap;
						cl.snap.ping = 999;
						// calculate ping time
						for ( i = 0 ; i < PACKET_BACKUP ; i++ ) 
						{
							packetNum = ( clc.netchan.outgoingSequence - 1 - i ) & PACKET_MASK;
							if ( cl.snap.ps.commandTime >= cl.outPackets[ packetNum ].p_serverTime ) 
							{
								cl.snap.ping = cls.realtime - cl.outPackets[ packetNum ].p_realtime;
								break;
							}
						}
					}




7.	so let see how the client handles the playerState

					msg.c

					void MSG_ReadDeltaPlayerstate (msg_t *msg, playerState_t *from, playerState_t *to )
					{



					}





8.	What this function is doing is very similar to what quake2 did. so if you confused, refer back to quake2.

	-	recall that clSnapshot_t member variable parseEntitiesNum is the same as "firstEntityIndex", so you can see
		the client marking the start of the snapshot entity in the client.parseEntities array. 

					cl_parse.c

					CL_ParsePacketEntities( msg_t *msg, clSnapshot_t *oldframe, clSnapshot_t *newframe) 
					{

						newframe->parseEntitiesNum = cl.parseEntitiesNum;
						newframe->numEntities = 0;



						entityState_t	*state;

						// save the parsed entity state into the big circular buffer so
						// it can be used as the source for a later delta
						state = &cl.parseEntities[cl.parseEntitiesNum & (MAX_PARSE_ENTITIES-1)];




					}





8.	Then later on CL_GetSnapShot() is called where 

					
					cl_cgame.c

					qboolean	CL_GetSnapshot( int snapshotNumber, snapshot_t *snapshot ) 
					{

						...
						...

						snapshot->numEntities = count;
						for ( i = 0 ; i < count ; i++ ) {
							snapshot->entities[i] = 
								cl.parseEntities[ ( clSnap->parseEntitiesNum + i ) & (MAX_PARSE_ENTITIES-1) ];
						}

						...

					}




9.	
	-	so you can see how we are storing it in cl.parseEntities


					CL_DeltaEntity()
					{

						state = &cl.parseEntities[cl.parseEntitiesNum & (MAX_PARSE_ENTITIES-1)];

						...
						

						if ( unchanged ) {
							*state = *old;
						} else {
							MSG_ReadDeltaEntity( msg, old, state, newnum );
						}
						...

						cl.parseEntitiesNum++;
						frame->numEntities++;

					}








NOTE:	you might have noticed that on the rendering side, there is another snapshot type


					cg_publish.h

					// Snapshots are generated at regular time intervals by the server,
					// but they may not be sent if a client's rate level is exceeded, or
					// they may be dropped by the network.
					typedef struct {
						int				snapFlags;			// SNAPFLAG_RATE_DELAYED, etc
						int				ping;

						int				serverTime;		// server time the message is valid for (in msec)

						...
						...

						playerState_t	ps;						// complete information about the current player at this time

						int				numEntities;			// all of the entities that need to be presented
						entityState_t	entities[MAX_ENTITIES_IN_SNAPSHOT];	// at the time of this snapshot

						int				numServerCommands;		// text based server commands to execute when this
						int				serverCommandSequence;	// snapshot becomes current
					} snapshot_t;






actually what happens is that in CL_GetSnapshot(){}, we are constructing the cg-snapshot_t, by from a client parsed snapshot


					cl_cgame.c

					CL_GetSnapshot( int snapshotNumber, snapshot_t *snapshot )
					{
						clSnapshot_t	*clSnap;

						...
						...
						clSnap = &cl.snapshots[snapshotNumber & PACKET_MASK];



						snapshot->ps = clSnap->ps;
						count = clSnap->numEntities;


						snapshot->numEntities = count;
						for ( i = 0 ; i < count ; i++ ) {
							snapshot->entities[i] = 
								cl.parseEntities[ ( clSnap->parseEntitiesNum + i ) & (MAX_PARSE_ENTITIES-1) ];
						}

						// FIXME: configstring changes and server commands!!!

						return qtrue;

					}	

