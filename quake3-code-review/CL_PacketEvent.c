
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







5.	we can see that when the Client reads in a msg that has a snapshot information

	-	notice that this function has two variables of type clSnapshot_t? This snapshot is actually different
		from the one used on the server
		
		if you look inside the struct	

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




					cl_parse.c

					void CL_ParseSnapshot( msg_t *msg )
					{

						clSnapshot_t	*old;
						clSnapshot_t	newSnap;
						int			deltaNum;


						deltaNum = MSG_ReadByte( msg );
						if ( !deltaNum ) {
							newSnap.deltaNum = -1;
						} else {
							newSnap.deltaNum = newSnap.messageNum - deltaNum;
						}




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
					}




6.	so let see how the client handles the playerState

					msg.c

					void MSG_ReadDeltaPlayerstate (msg_t *msg, playerState_t *from, playerState_t *to )
					{



					}





7.	What this function is doing is very similar to what quake2 did. so if you confused, refer back to quake2.

					cl_parse.c

					CL_ParsePacketEntities()
					{
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















					