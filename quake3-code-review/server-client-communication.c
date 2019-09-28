

This article talks about clinet and server communication, sequence numbers and ping between the two ends

######################################################################################
#################################### netchan #########################################
######################################################################################

first of course, I have to talk about the netchan. this struct is shared by both the client and the server, which is why
this struct sits in qcommon.h


like what the comments says: 

	" Netchan handles packet fragmentation and out of order / duplicate suppression "

the main functionality of this truct is used to record data relevant to packet fragmetnation, out of order / duplicate scenarios

					qcommon.h

					/*
					Netchan handles packet fragmentation and out of order / duplicate suppression
					*/

					typedef struct {
						netsrc_t	sock;

						int			dropped;			// between last packet and previous

						...
						...

						// sequencing variables
						int			incomingSequence;
						int			outgoingSequence;

						...
						...
					} 

					netchan_t;



the important thing here are

					int 	incomingSequence  
					int 	outgoingSequence



#####################################################################################################
############################# outgoingSequence and incomingSequence #################################
#####################################################################################################


Note that incomingSequence is only used in the Netchan layer, the game logic never actually uses/touches/reads this incomingSequence variable

if you do a search on "Find All Reference", you will see that 
the majority happens in the Netchan_Process(){} function		


					net_chan.c

					void Netchan_Process()
					{

						...

						// get sequence numbers		
						MSG_BeginReadingOOB( msg );
						sequence = MSG_ReadLong( msg );

						...
						...


						//
						// discard out of order or duplicated packets
						//
						if ( sequence <= chan->incomingSequence ) 
						{
							// OUT OF ORDER PACKET
							return qfalse;
						}





						//
						// dropped packets don't keep the message from being used
						//
						chan->dropped = sequence - (chan->incomingSequence+1);
						if ( chan->dropped > 0 ) 
						{
							// DROPPED PACKET
							// however, we still use the packet in the game, 
							// we are just printing it out, telling the user we have a dropped packet
						}


						chan->incomingSequence = sequence;
						return qtrue;

					}


and that is pretty much where all the occurences happen. As you can see, "incomingSequence" is only used on the Netchan layer
				


On the other hand, the outgoingSequence is used in the game logic

on the server side, the outgoingSequence is used to determine which snapshot to construct

					
					sv_snapshot.c			

					static void SV_WriteSnapshotToClient( client_t *client, msg_t *msg ) 
					{
						...

						// this is the snapshot we are creating
						frame = &client->frames[ client->netchan.outgoingSequence & PACKET_MASK ];

						...
						...
						...

					}



the second use of outgoingSequence in the game logic is used to calculate ping, I will talk about that later


I also want to mention that when either the client or the server sends out messages, they will call Netchan_Transmit(){},
in there the the outgoingSequence will be incremented. 

It is sort of like outgoingSequence is read only outside of the Netchan code

	
					net_chan.c

					void Netchan_Transmit( netchan_t *chan, int length, const byte *data ) 
					{

						...
						...

						// write the packet header
						MSG_InitOOB (&send, send_buf, sizeof(send_buf));

						MSG_WriteLong( &send, chan->outgoingSequence );
						chan->outgoingSequence++;

						MSG_WriteData( &send, data, length );

						// send the datagram
						NET_SendPacket( chan->sock, send.cursize, send.data, chan->remoteAddress );
			
						...
						...
					}
					
























##################################################################################
################## How clients are recorded on the server  #######################
##################################################################################

Now lets talk about the server and client. First of all the server has 
its own copy of client data. Between every single client connection, it has their own sequence and ack numbers

so client data struct is declared in server.h


					server.h

					typedef struct client_s 
					{

						clientState_t	state;
						char			userinfo[MAX_INFO_STRING];		// name, etc					
						int				messageAcknowledge;

						...
						...
						...

						int				deltaMessage;		// frame last client usercmd message

						...
						...
						...
					
						int				ping;
						int				rate;				// bytes / second

						...
						...
						netchan_t		netchan;
					} client_t;


I have included only the important ones. Note that both messageAcknowledge and deltaMessage are heavily used in the game logic







As for the client representation on the client side, that is included in client.h


					client.h 

					typedef struct 
					{
						...
						...

						// message sequence is used by both the network layer and the
						// delta compression layer
						int			serverMessageSequence;

						// reliable messages received from server
						int			serverCommandSequence;

						...
						...
						netchan_t	netchan;
					} clientConnection_t;

you can see they are quite similar




######################################################################################################################
################## Sequence and Packet Structure when Server Sending Snapshot  #######################################
######################################################################################################################

1.	so for the server sending the snapshot down to the client happens in the SV_SendClientSnapshot(){}


					sv_snapshot.c 

					void SV_SendClientSnapshot( client_t *client ) 
					{

						...
						...

						MSG_WriteLong( &msg, client->lastClientCommand );

						SV_WriteSnapshotToClient( client, &msg );

						...
						...

						SV_SendMessageToClient( &msg, client );
					}


2.	you can see that in the first function SV_WriteSnapshotToClient, we populate msg with all the snapshot relevant information 

	you can see that the current frame and lastFrame are mainly determined by client->netchan.outgoingSequence and client->deltaMessage

	client->deltaMessage is the last server message that client got, (server will know this when the server parses the client input command, client 
		will send over his last server message sequence number along with his UserCmd)

	therefore, whenwe construct a snapshot, it will be doen from lastframe to netchan.outgoingSequence frame

					sv_snapshot.c 				

					static void SV_WriteSnapshotToClient( client_t *client, msg_t *msg ) 
					{

						// this is the snapshot we are creating

						frame = &client->frames[ client->netchan.outgoingSequence & PACKET_MASK ];

						...
						...

						oldframe = &client->frames[ client->deltaMessage & PACKET_MASK ];
						lastframe = client->netchan.outgoingSequence - client->deltaMessage;	// index of oldframe


						...
						...


						MSG_WriteByte (msg, svc_snapshot);

						// NOTE, MRE: now sent at the start of every message from server to client
						// let the client know which reliable clientCommands we have received
						//MSG_WriteLong( msg, client->lastClientCommand );

						// send over the current server time so the client can drift
						// its view of time to try to match
						MSG_WriteLong (msg, svs.time);

						// what we are delta'ing from
						MSG_WriteByte (msg, lastframe);


						SV_EmitPacketEntities (oldframe, frame, msg);

					}


3.	then in the 2nd function, we will call SV_Netchan_Transmit(){}, which will write the header as well as the outgoingSequence number

	another interesting is that since the server keeps a buffer of the recent frames, it will initialize some of the data for the client->frame

	you can see that messageAcked = -1; The reason why we set it to -1 is becuz the messageAcked number will be assigned properly 
	when the server reads the response from the client



					sv_snapshot.c

					void SV_SendMessageToClient( &msg, client )
					{

						// record information about the message
						client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSize = msg->cursize;
						client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSent = svs.time;
						client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageAcked = -1;

						// send the datagram
						SV_Netchan_Transmit( client, msg );	//msg->cursize, msg->data );

						...
						...
						...

						client->nextSnapshotTime = svs.time + rateMsec;

					}




So the packet structure is 

?	header																MSG_InitOOB (&send, send_buf, sizeof(send_buf));	(SV_Netchan_Transmit();)
4	sequence number (high bit set if an oversize fragment)				MSG_WriteLong( &send, chan->outgoingSequence );
4	last client reliable command 										MSG_WriteLong( &msg, client->lastClientCommand );	(SV_SendClientSnapshot();)
1	svc_snapshot														MSG_WriteByte (msg, svc_snapshot);					(SV_WriteSnapshotToClient();)
4	serverTime															MSG_WriteLong (msg, svs.time);						
1	lastframe for delta compression										MSG_WriteByte (msg, lastframe);
1	snapFlags
1	areaBytes
<areabytes>
<playerstate>
<packetentities>










So on the client side, when the client reads the snapshot message, this occurs in CL_PacketEvent(){}. The trick thing is that
some of the things happen in various order. For example, to capture the sequence number, which is at the head the msg->data 

we manually cast it into clc.serverMessageSequence

					clc.serverMessageSequence = LittleLong( *(int *)msg->data );

if we have called MSG_ReadXXXX, it would not have worked becuz msg byte offset has already passed outgoingSequence.



					
					cl_main.c

					CL_PacketEvent()
					{

						...
						...

						if (!CL_Netchan_Process( &clc.netchan, msg) ) {
							return;		// out of order, duplicated, etc
						}

						...
						...

						// track the last message received so it can be returned in 
						// client messages, allowing the server to detect a dropped
						// gamestate
						clc.serverMessageSequence = LittleLong( *(int *)msg->data );	// i believe this is server->outgoingSequence
																						// also client->incomingSequence
																						// becuz in CL_WritePacket(), 
																						// we will call "MSG_WriteLong( &buf, clc.serverMessageSequence );"

						clc.lastPacketTime = cls.realtime;
						CL_ParseServerMessage( msg );

						...
						...
					}



2.	as we look inside clc.reliableAcknowledge, 

		clc.reliableAcknowledge = MSG_ReadLong( msg ); 

	will read in "MSG_WriteLong( &msg, client->lastClientCommand );" sent down by the server


					cl_parse.c

					void CL_ParseServerMessage( msg_t *msg ) 
					{

						// get the reliable sequence acknowledge number
						clc.reliableAcknowledge = MSG_ReadLong( msg );

						...
						...
						...

						case svc_snapshot:
							printf("	### CL_ParseSnapshot\n");
							CL_ParseSnapshot( msg );
							break;
					
						...
						...
					}




3.	then in CL_ParseSnapshot(){} we carry on reading the rest of the message. Note that for these two

					newSnap.serverCommandNum = clc.serverCommandSequence;

					newSnap.messageNum = clc.serverMessageSequence;			 // reading in Server's call MSG_WriteLong( &send, chan->outgoingSequence );

	the values were already stored in clc.serverCommandSequence; and clc.serverMessageSequence respectively. So we do not call MSG_ReadXXXX on these two values



					cl_parse.c 

					void CL_ParseSnapshot( msg_t *msg ) 
					{

						...
						...

						// we will have read any new server commands in this
						// message before we got to svc_snapshot
						newSnap.serverCommandNum = clc.serverCommandSequence;

						newSnap.serverTime = MSG_ReadLong( msg );

						newSnap.messageNum = clc.serverMessageSequence;

						deltaNum = MSG_ReadByte( msg );
						newSnap.deltaNum = newSnap.messageNum - deltaNum;

						newSnap.snapFlags = MSG_ReadByte( msg );

						...
						...

					}












############################################################
################## Client To Server  #######################
############################################################

Now lets take a look at how Client to Server communication work. The most common operation is the client sending its usercmd


1.	so in the code, this happens in CL_SendCmd(){};

					cl_main.c

					CL_SendCmd(void)
					{

						...
						...

						// we create commands even if a demo is playing,
						CL_CreateNewCommands();

						...
						...

						CL_WritePacket();
					}






2.					

	- 	at the bottom of the function, you can see we assign some values on cl.outPackets,

					packetNum = clc.netchan.outgoingSequence & PACKET_MASK;
					cl.outPackets[ packetNum ].p_realtime = cls.realtime;
					cl.outPackets[ packetNum ].p_serverTime = oldcmd->serverTime;
					cl.outPackets[ packetNum ].p_cmdNumber = cl.cmdNumber;
					clc.lastPacketSentTime = cls.realtime;

	cl.outPackets is a circular array, keeping track of the packets we have sent out. This is actually also used to calculate
	the latency on the client side, and this is done at CL_ParseSnapshot(){}, we will talk about this later

	-	you can also see how quake3 implements reliability, essentially, any commands that is unacknowledged, we resent it



					cl_input.c

					CL_WritePacket()
					{

						MSG_Init( &buf, data, sizeof(data) );

						MSG_Bitstream( &buf );
						// write the current serverId so the server
						// can tell if this is from the current gameState
						MSG_WriteLong( &buf, cl.serverId );

						// write the last message we received, which can
						// be used for delta compression, and is also used
						// to tell if we dropped a gamestate
						MSG_WriteLong( &buf, clc.serverMessageSequence );

						// write the last reliable message we received
						MSG_WriteLong( &buf, clc.serverCommandSequence );


						// write any unacknowledged clientCommands
						for ( i = clc.reliableAcknowledge + 1 ; i <= clc.reliableSequence ; i++ ) 
						{
							MSG_WriteByte( &buf, clc_clientCommand );
							MSG_WriteLong( &buf, i );
							MSG_WriteString( &buf, clc.reliableCommands[ i & (MAX_RELIABLE_COMMANDS-1) ] );
						}




						// begin a client move command
						if ( cl_nodelta->integer || !cl.snap.valid || clc.demowaiting
							|| clc.serverMessageSequence != cl.snap.messageNum ) 
						{
							MSG_WriteByte (&buf, clc_moveNoDelta);
						} 
						else 
						{
							MSG_WriteByte (&buf, clc_move);
						}

						// write the command count
						MSG_WriteByte( &buf, count );



						...
						...
						...



						packetNum = clc.netchan.outgoingSequence & PACKET_MASK;
						cl.outPackets[ packetNum ].p_realtime = cls.realtime;
						cl.outPackets[ packetNum ].p_serverTime = oldcmd->serverTime;
						cl.outPackets[ packetNum ].p_cmdNumber = cl.cmdNumber;
						clc.lastPacketSentTime = cls.realtime;



						
						CL_Netchan_Transmit (&clc.netchan, &buf);	


					}








3.	As mentioned above, at the end of this function, cl.outPackets is introduced, and we talked about how 
	the latency/ping calculation are done at the end of the CL_ParseSnapshot(){} function


					packetNum = clc.netchan.outgoingSequence & PACKET_MASK;
					cl.outPackets[ packetNum ].p_realtime = cls.realtime;
					cl.outPackets[ packetNum ].p_serverTime = oldcmd->serverTime;
					cl.outPackets[ packetNum ].p_cmdNumber = cl.cmdNumber;
					clc.lastPacketSentTime = cls.realtime;



	so if you look inside CL_ParseSnapshot(){}, you will see how we calculate the ping time,
	we are finding the "correct" outPacket corresponding to the snapshot we just got from the server



					cl_parse.c

					void CL_ParseSnapshot( msg_t *msg ) 
					{

						...
						...
						...


						...
						...
						...

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

						...
						...
						
					}




Back to CL_WritePacket, the structure of the packet is 

During normal gameplay, a client packet will contain something like:

4	sequence number
2	qport
4	serverid
4	acknowledged sequence number
4	clc.serverCommandSequence
<optional reliable commands>
1	clc_move or clc_moveNoDelta
1	command count
<count * usercmds>




############################################################################
################## Server Reading the Client Command #######################
############################################################################



1.	so we read the 
					sv_client.c

					SV_ExecuteClientMessage()
					{
						MSG_Bitstream(msg);

						serverId = MSG_ReadLong( msg );
						cl->messageAcknowledge = MSG_ReadLong( msg );

						cl->reliableAcknowledge = MSG_ReadLong( msg );


						...
						...

						// read optional clientCommand strings
						do {
							c = MSG_ReadByte( msg );
							if ( c == clc_EOF ) {
								break;
							}
							if ( c != clc_clientCommand ) {
								break;
							}
							if ( !SV_ClientCommand( cl, msg ) ) {
								return;	// we couldn't execute it because of the flood protection
							}
							if (cl->state == CS_ZOMBIE) {
								return;	// disconnect command
							}
						} while ( 1 );




						// read the usercmd_t
						if ( c == clc_move ) {
							SV_UserMove( cl, msg, qtrue );
						} else if ( c == clc_moveNoDelta ) {
							SV_UserMove( cl, msg, qfalse );
						} else if ( c != clc_EOF ) {
							Com_Printf( "WARNING: bad command byte for client %i\n", cl - svs.clients );
						}


					}



2.	
	-	cl->deltaMessage gets assigned to the value of cl->messageAcknowledge
		something we have already read in SV_ExecuteClientMessage(){}

		that is the last server message the client got from the server, so therefore when
		we make our delta compression, we start from this state


	-	also when the server gets the message from the client, we prepare from ping calculation

			cl->frames[ cl->messageAcknowledge & PACKET_MASK ].messageAcked = svs.time;



					sv_client.c

					static void SV_UserMove( client_t *cl, msg_t *msg, qboolean delta ) 
					{


						if ( delta ) {
							cl->deltaMessage = cl->messageAcknowledge;
						} else {
							cl->deltaMessage = -1;
						}



						cmdCount = MSG_ReadByte( msg );

						...
						...

						// save time for ping calculation
						cl->frames[ cl->messageAcknowledge & PACKET_MASK ].messageAcked = svs.time;


					}


3.

	the server calculateing the ping actually happens in the SV_Frame(){} function


					SV_Frame()
					{

						// update ping based on the all received frames
						SV_CalcPings();
						
						...
						...

						// check timeouts
						SV_CheckTimeouts();

						// send messages back to the clients
						SV_SendClientMessages();
					
						...
						...
					}




	- 	lets see how we actually calculate ping

	-	we have two for loops here, the first one loops through the clients, since we want to calcualte ping for each client

		the nested for loop 	

		in this nested for loop 
			if ( cl->frames[j].messageAcked <= 0 ) 

					/*
					===================
					SV_CalcPings

					Updates the cl->ping variables
					===================
					*/

					sv_main.c

					// update ping based on the all received frames
					void SV_CalcPings()
					{
						for (i=0 ; i < sv_maxclients->integer ; i++) 
						{
							cl = &svs.clients[i];
							if ( cl->state != CS_ACTIVE ) {
								cl->ping = 999;
								continue;
							}
							if ( !cl->gentity ) {
								cl->ping = 999;
								continue;
							}
							if ( cl->gentity->r.svFlags & SVF_BOT ) {
								cl->ping = 0;
								continue;
							}

							total = 0;
							count = 0;
							for ( j = 0 ; j < PACKET_BACKUP ; j++ ) 
							{
								if ( cl->frames[j].messageAcked <= 0 ) 
								{
									continue;
								}
								delta = cl->frames[j].messageAcked - cl->frames[j].messageSent;
								count++;
								total += delta;
							}
							if (!count) 
							{
								cl->ping = 999;
							} 
							else 
							{
								cl->ping = total/count;
								if ( cl->ping > 999 ) 
								{
									cl->ping = 999;
								}
							}

							// let the game dll know about the ping
							ps = SV_GameClientNum( i );
							ps->ping = cl->ping;
						}

					}





#################################################################################
##################### Summary ###################################################
#################################################################################

In short, it is a bit like a chicken and egg, circular sequence

becuz what the server sends to the client depends on what he has received from the client, and vice versa,
what the client sends to the server depends on what he was gotten from the client


Server->Client snapshots:

				So the packet structure is 

				?	header																MSG_InitOOB (&send, send_buf, sizeof(send_buf));	(SV_Netchan_Transmit();)
				4	sequence number (high bit set if an oversize fragment)				MSG_WriteLong( &send, chan->outgoingSequence );
				4	last client reliable command 										MSG_WriteLong( &msg, client->lastClientCommand );	(SV_SendClientSnapshot();)
				1	svc_snapshot														MSG_WriteByte (msg, svc_snapshot);					(SV_WriteSnapshotToClient();)
				4	serverTime															MSG_WriteLong (msg, svs.time);						
				1	lastframe for delta compression										MSG_WriteByte (msg, lastframe);
				1	snapFlags
				1	areaBytes
				<areabytes>
				<playerstate>
				<packetentities>


1.	the only slightly confusing thing here is the client->lastClientCommand. That value depends on what the server has received from the client

	as explained below, the client will send his commands (reliable messages) along with his usercmd packet 

	and when the server reads the usercmd packet from the client in SV_ExecuteClientMessage(){}, the server will read it and store it in client->lastClientCommand


	

Client->Server usercmd:

				so the packet structure is 

				4	sequence number
				2	qport
				4	serverid
				4	acknowledged sequence number 										MSG_WriteLong( &buf, clc.serverMessageSequence );
				4	clc.serverCommandSequence 											MSG_WriteLong( &buf, clc.serverCommandSequence );
				<optional reliable commands>
				1	clc_move or clc_moveNoDelta
				1	command count
				<count * usercmds>


1.	so the "ackwnoledged sequence number" is the the last sequence number we got from server for the server to construct delta compression 
	
		client sends out: clc.serverMessageSequence 

	then when the server reads it, it gets put in to the variable:

		cl->messageAcknowledge, cl->deltaMessage



2.	the clc.serverCommandSequence is similar, we send the last ack we got for the reliable messages, so the server can know which reliable 
	messages to get back to u








Here is a brief timeline diagram about sequence numbers and the order of operations happening




Server																														client


#################################################
################## SetUp  #######################
#################################################


	// upon connection
	// server sets up Netchan for the new client
	// this actually happens in the main loop, but we are putting this here for simplicity

	SV_DirectConnect()
	{
	
		client_t	*cl, *newcl;

		...
		...
		

		Netchan_setup(&newcl->netchan)
		{
			...
			...

			chan->incomingSequence = 0;
			chan->outgoingSequence = 1;
		}
	}
	


	Note that netchan.incomingSequence does not seemed to used except in 


	Netchan_Process
	{


		// get sequence numbers		
		MSG_BeginReadingOOB( msg );
		sequence = MSG_ReadLong( msg );

	//
	// dropped packets don't keep the message from being used
	//
		chan->dropped = sequence - (chan->incomingSequence+1);
		if ( chan->dropped > 0 ) 
		{
			if ( showdrop->integer || showpackets->integer ) {
				Com_Printf( "%s:Dropped %i packets at %i\n"
				, NET_AdrToString( chan->remoteAddress )
				, chan->dropped
				, sequence );
			}
		}
	


		chan->incomingSequence = sequence;
	}



#################################################
################## Frame 0 ######################
#################################################


	SV_Frame()
	{

		// nothing happens cuz we dont receive anything on frame 0
		SV_ReadPacket()
		{

		}	

		...
		...
		...


		SV_SendClientSnapshot()
		{

			// note that client->d
			SV_WriteSnapshotToClient()
			{
				frame = &client->frames[ client->netchan.outgoingSequence & PACKET_MASK ];
				lastframe = client->netchan.outgoingSequence - client->deltaMessage;


				// construct snapshot with 
			}


			SV_SendMessageToClient()
			{

				// record information about the message

				int index = client->netchan.outgoingSequence & PACKET_MASK;

				client->frames[index].messageSize = msg->cursize;
				client->frames[index].messageSent = svs.time;
				client->frames[index].messageAcked = -1;

			}
		}


	}






















#################################################
################## Frame 1 ######################
#################################################



	SV_Frame()
	{

		SV_ReadPacket()
		{
			// Read Client packets from each connected clients
			SV_ExecuteClientMessage( cl, msg )
			{

				// read the sequence number that the client sent over
				cl->messageAcknowledge = MSG_ReadLong( msg );
				...
				cl->reliableAcknowledge = MSG_ReadLong( msg );


				SV_UserMove()
				{

					// save time for ping calculation
					cl->frames[ cl->messageAcknowledge & PACKET_MASK ].messageAcked = svs.time;


					// run SV_ClientThink (cl, &cmds[ i ]);
				}
			}
		}	




		SV_SendClientSnapshot()
		{
			SV_SendMessageToClient()
			{

				// record information about the message

				int index = client->netchan.outgoingSequence & PACKET_MASK;

				client->frames[index].messageSize = msg->cursize;
				client->frames[index].messageSent = svs.time;
				client->frames[index].messageAcked = -1;

			}
		}




	}










General thoughts


1.	is it just me that outgoingSequence, which is a variable in the netchan should stay in the netchannel layer?