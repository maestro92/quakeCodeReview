

So in this article, we will look at how the server process events from the queue.


1.	Starting from the WinMain(){} as our entry point, we see it will call Com_Frame(){} in the main loop. This is already mentioned
	in the main-code.c



					WinMain()
					{
						while( 1 ) 
						{
							...
							...

							// run the game
							Com_Frame();

							...
							...
						}


					}



2.	Then if we go into Com_Frame(){}, we can see that it will pump the sysEvent_t queue and pump UDP incoming queue 
	before running the server frame. For details of the server frame logic, please refer to server-frame.c 
		

					common.c

					Com_Frame()
					{

						...
						...

						do {
							
							com_frameTime = Com_EventLoop();


							if ( lastTime > com_frameTime ) {
								lastTime = com_frameTime;		// possible on first frame
							}
							msec = com_frameTime - lastTime;
						} while ( msec < minMsec );
				

						...
						...

						SV_Frame(msec);

					}



3.	Of course, we have to look into what happens in com_frameTime = Com_EventLoop(); This function is shared by both client and server when they 
	want to retrieve events.

	-	this is where we process every event in the queue until there is none left. This is of course done by the 
		main while(1) loop.


	-	later on, once we get to Com_GetEvent(){} in detail, you will see that if we run out of events from the eventQue, 
		Com_GetEvent(){} will return an empty event, hence it will go to the "if ( ev.evType == SE_NONE )" path. Once it is in there

		it will check if we have any network messages from NS_CLIENT or NS_SERVER. Here we will obviously only get messages in the NS_SERVER port.
		we will all process packet events from the NS_SERVER in the while loop and calling Com_RunAndTimeServerPacket(){}. 


	-	a little note about NS_SERVER and NS_CLIENT. These are just two enums to mark wether a Netchan is from the server or client
		you can see how the code uses it to set up the network

					cl_main.c

					void CL_ConnectionlessPacket()
					{
						// server connection
						if()
						{
							...
							...

							Netchan_Setup (NS_CLIENT, &clc.netchan, from, Cvar_VariableValue( "net_qport" ) );

							...
							...
						}
					}


		and in sv_client.c

					sv_client.c

					void SV_DirectConnect( netadr_t from ) {
								
						...
						...

						// save the address
						Netchan_Setup (NS_SERVER, &newcl->netchan , from, qport);

						...
						...
					}



	Enought talk, now lets look at the real code


	-	you can see that whenever we get a valid event, it goes into the switch statement and depending on the event type,
		we go into different functions.

					common.c

					Com_EventLoop()
					{
						...
						...

						while ( 1 ) {
							ev = Com_GetEvent();

							// if no more events are available
							if ( ev.evType == SE_NONE ) {
								// manually send packet events for the loopback channel
								while ( NET_GetLoopPacket( NS_CLIENT, &evFrom, &buf ) ) {
									CL_PacketEvent( evFrom, &buf );
								}

								while ( NET_GetLoopPacket( NS_SERVER, &evFrom, &buf ) ) {
									// if the server just shut down, flush the events
									if ( com_sv_running->integer ) {
										Com_RunAndTimeServerPacket( &evFrom, &buf );
									}
								}

								return ev.evTime;
							}


							switch ( ev.evType ) {
							default:
							  // bk001129 - was ev.evTime
								Com_Error( ERR_FATAL, "Com_EventLoop: bad event type %i", ev.evType );
								break;
					        case SE_NONE:
					            break;
							case SE_KEY:
								CL_KeyEvent( ev.evValue, ev.evValue2, ev.evTime );
								break;
							case SE_CHAR:
								CL_CharEvent( ev.evValue );
								break;
							case SE_MOUSE:
								CL_MouseEvent( ev.evValue, ev.evValue2, ev.evTime );
								break;
							case SE_JOYSTICK_AXIS:
								CL_JoystickEvent( ev.evValue, ev.evValue2, ev.evTime );
								break;
							case SE_CONSOLE:
								Cbuf_AddText( (char *)ev.evPtr );
								Cbuf_AddText( "\n" );
								break;
							case SE_PACKET:
								// this cvar allows simulation of connections that
								// drop a lot of packets.  Note that loopback connections
								// don't go through here at all.
								if ( com_dropsim->value > 0 ) {
									static int seed;

									if ( Q_random( &seed ) < com_dropsim->value ) {
										break;		// drop this packet
									}
								}

								evFrom = *(netadr_t *)ev.evPtr;
								buf.cursize = ev.evPtrLength - sizeof( evFrom );

								// we must copy the contents of the message out, because
								// the event buffers are only large enough to hold the
								// exact payload, but channel messages need to be large
								// enough to hold fragment reassembly
								if ( (unsigned)buf.cursize > buf.maxsize ) {
									Com_Printf("Com_EventLoop: oversize packet\n");
									continue;
								}
								Com_Memcpy( buf.data, (byte *)((netadr_t *)ev.evPtr + 1), buf.cursize );
								if ( com_sv_running->integer ) {
									Com_RunAndTimeServerPacket( &evFrom, &buf );
								} else {
									CL_PacketEvent( evFrom, &buf );
								}
								break;
							}

							// free any block data
							if ( ev.evPtr ) {
								Z_Free( ev.evPtr );
							}


						}
					}







#######################################################################################
############################# Com_GetEvent(){} ########################################
#######################################################################################

1.	you can see that we first fetch the event by calling ev = Com_GetEvent(){}; the code for it is very simple

					common.c 

					sysEvent_t	Com_GetEvent( void ) {
						if ( com_pushedEventsHead > com_pushedEventsTail ) {
							com_pushedEventsTail++;
							return com_pushedEvents[ (com_pushedEventsTail-1) & (MAX_PUSHED_EVENTS-1) ];
						}
						return Com_GetRealEvent();
					}

		like what we mentioned in the main-code.c file, whenever we want to remove/pop an entry, we just increase the tail counter (com_pushedEventsTail){}



2.	In the case of com_pushedEventsHead <= com_pushedEventsTail, which means out com_pushedEvents queue is empty, we will call Com_GetRealEvent(){};



					common.c

					sysEvent_t Com_GetRealEvent( void )
					{

						int			r;
						sysEvent_t	ev;

						// either get an event from the system or the journal file
						if ( com_journal->integer == 2 ) 
						{

						}
						else
						{
							ev = Sys_GetEvent();

							...
							...
						}

						return ev;
					}		

		


6.	this function returns data if our eventQue has something. AND if you recall from main-code.c, when we process our mouse inputs
	in the IN_Frame(){}, mouse information is stored into eventQue by calling the Sys_QueEvent(){} function.


	-	in the case of 

					if ( Sys_GetPacket ( &adr, &netmsg ) ) {

						...
						Sys_QueEvent( 0, SE_PACKET, 0, 0, len, buf );
					}

		you can see that if we get a packet, we will call Sys_QueEvent(){}. So we are translating receving a network packet into 
		a system que event. SE_PACKET stands for System Event.


	-	in the case of 

						// return if we have data
					if ( eventHead > eventTail ) {
						eventTail++;
						return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
					}

		once again, if we are returning something, we increase the eventTail couter. 


	-	also on the last notice, we create an empty event to return


					win_main.c

					sysEvent_t Sys_GetEvent()
					{

						...
						...

						// return if we have data
						if ( eventHead > eventTail ) {
							eventTail++;
							return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
						}



						// check for network packets
						MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );
						if ( Sys_GetPacket ( &adr, &netmsg ) ) {
							netadr_t		*buf;
							int				len;

							// copy out to a seperate buffer for qeueing
							// the readcount stepahead is for SOCKS support
							len = sizeof( netadr_t ) + netmsg.cursize - netmsg.readcount;
							buf = Z_Malloc( len );
							*buf = adr;
							memcpy( buf+1, &netmsg.data[netmsg.readcount], netmsg.cursize - netmsg.readcount );
							Sys_QueEvent( 0, SE_PACKET, 0, 0, len, buf );
						}

						// return if we have data
						if ( eventHead > eventTail ) {
							eventTail++;
							return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
						}


						// create an empty event to return

						memset( &ev, 0, sizeof( ev ) );
						ev.evTime = timeGetTime();

						return ev;

					}












7.					
























4.					common.c
					
					void Com_RunAndTimeServerPacket()
					{
						...
						...

						SV_PacketEvent( *evFrom, buf );

						...
						...
					}


##########################################################################$###########
################################## SV_PacketEvent() ##################################					
##########################################################################$###########

1.	we finally arrived here at The SV_PacketEvent(){}, let us look at what is going on 


					sv_main.c

					SV_PacketEvent()
					{


						...
						...


						// find which client the message is from
						for (i=0, cl=svs.clients ; i < sv_maxclients->integer ; i++,cl++) {
							if (cl->state == CS_FREE) {
								continue;
							}

							...
							...
							...
							...

							// make sure it is a valid, in sequence packet
							if (SV_Netchan_Process(cl, msg)) {
								// zombie clients still need to do the Netchan_Process
								// to make sure they don't need to retransmit the final
								// reliable message, but they don't do any other processing
								if (cl->state != CS_ZOMBIE) {
									cl->lastPacketTime = svs.time;	// don't timeout
									SV_ExecuteClientMessage( cl, msg );
								}
							}
							return;
						}


						...
						...
					}





2.	you can see we update the cl->messageAcknowledge

					sv_client.c

					void SV_ExecuteClientMessage()
					{

						cl->messageAcknowledge = MSG_ReadLong( msg );

						...
						...

						// read the usercmd_t
						if ( c == clc_move ) {
							SV_UserMove( cl, msg, qtrue );
						} else if ( c == clc_moveNoDelta ) {
							SV_UserMove( cl, msg, qfalse );
						} else if ( c != clc_EOF ) {
							Com_Printf( "WARNING: bad command byte for client %i\n", cl - svs.clients );
						}

					}







3.	we execute the user command movement in the SV_UserMove(){} function
	if you look into the code, you might have noticed that we update the cl->deltaMessage = cl->messageAcknowledge.

	we will later use cl->deltaMessage when we try to delta-construct the snapshot to send down to the client. // Note that this is also done in quake2
	keeping track of cl->deltaMessage number becuz we are employing a delta-compression scheme, so we need to know
	which message the client has already seen



					sv_client.c

					SV_UserMove()
					{


						...
						...


						if ( delta ) 
						{
							cl->deltaMessage = cl->messageAcknowledge;
						} 
						else 
						{
							cl->deltaMessage = -1;
						}



						// usually, the first couple commands will be duplicates
						// of ones we have previously received, but the servertimes
						// in the commands will cause them to be immediately discarded
						for ( i =  0 ; i < cmdCount ; i++ ) {
							// if this is a cmd from before a map_restart ignore it
							if ( cmds[i].serverTime > cmds[cmdCount-1].serverTime ) {
								continue;
							}
							// extremely lagged or cmd from before a map_restart
							//if ( cmds[i].serverTime > svs.time + 3000 ) {
							//	continue;
							//}
							// don't execute if this is an old cmd which is already executed
							// these old cmds are included when cl_packetdup > 0
							if ( cmds[i].serverTime <= cl->lastUsercmd.serverTime ) {
								continue;
							}
							SV_ClientThink (cl, &cmds[ i ]);
						}
					}





4.	


					/*
					==================
					SV_ClientThink

					Also called by bot code
					==================
					*/
					void SV_ClientThink (client_t *cl, usercmd_t *cmd) {
						cl->lastUsercmd = *cmd;

						if ( cl->state != CS_ACTIVE ) {
							return;		// may have been kicked during the last usercmd
						}

						VM_Call( gvm, GAME_CLIENT_THINK, cl - svs.clients );
					}






5.					

					vm.c

					int	QDECL VM_Call( vm_t *vm, int callnum, ... ) 
					{

						...
						...
						...
						...


						// if we have a dll loaded, call it directly
						if ( vm->entryPoint ) {
							//rcg010207 -  see dissertation at top of VM_DllSyscall() in this file.
							va_start(ap, callnum);
							for (i = 0; i < sizeof (args) / sizeof (args[i]); i++) {
								args[i] = va_arg(ap, int);
							}
							va_end(ap);

							r = vm->entryPoint( callnum,  args[0],  args[1],  args[2], args[3],
					                            args[4],  args[5],  args[6], args[7],
					                            args[8],  args[9], args[10], args[11],
					                            args[12], args[13], args[14], args[15]);
						} else if ( vm->compiled ) {
							r = VM_CallCompiled( vm, &callnum );
						} else {
							r = VM_CallInterpreted( vm, &callnum );
						}

						...
						...
					}



6.					

					g_main.c


					int vmMain()
					{

						switch ( command ) {
						case GAME_INIT:
							G_InitGame( arg0, arg1, arg2 );
							return 0;
						...
						...

						case GAME_CLIENT_THINK:
							ClientThink( arg0 );
							return 0;
						
						...
						...
						...
						...

						case BOTAI_START_FRAME:
							return BotAIStartFrame( arg0 );
						}

						return -1;
					}




7.	as what the comments says, this is called by the server when a new command has arrived from the client

					/*
					==================
					ClientThink

					A new command has arrived from the client
					==================
					*/


					g_active.c

					void ClientThink()
					{

						...
						...

						if ( !(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer ) {
							ClientThink_real( ent );
						}
					}




8.					
					

					g_active.c

					void ClientThink_real()
					{
						...
						...

						// execute client events
						ClientEvents( ent, oldEventSequence );

						...
						...

					}





9.					

					g_active.c

					void ClientEvents()
					{

						...
						...

						for ( i = oldEventSequence ; i < client->ps.eventSequence ; i++ ) 
						{
								event = client->ps.events[ i & (MAX_PS_EVENTS-1) ];

								switch ( event ) {
								case EV_FALL_MEDIUM:
								case EV_FALL_FAR:
									if ( ent->s.eType != ET_PLAYER ) {
										break;		// not in the player model
									}
									if ( g_dmflags.integer & DF_NO_FALLING ) {
										break;
									}
									if ( event == EV_FALL_FAR ) {
										damage = 10;
									} else {
										damage = 5;
									}
									VectorSet (dir, 0, 0, 1);
									ent->pain_debounce_time = level.time + 200;	// no normal pain sound
									G_Damage (ent, NULL, NULL, NULL, NULL, damage, 0, MOD_FALLING);
									break;

								case EV_FIRE_WEAPON:
									FireWeapon( ent );
									break;

								case EV_USE_ITEM1:		// teleporter
									// drop flags in CTF
									item = NULL;
									j = 0;

									if ( ent->client->ps.powerups[ PW_REDFLAG ] ) {
										item = BG_FindItemForPowerup( PW_REDFLAG );
										j = PW_REDFLAG;
									} else if ( ent->client->ps.powerups[ PW_BLUEFLAG ] ) {
										item = BG_FindItemForPowerup( PW_BLUEFLAG );
										j = PW_BLUEFLAG;
									} else if ( ent->client->ps.powerups[ PW_NEUTRALFLAG ] ) {
										item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
										j = PW_NEUTRALFLAG;
									}

									if ( item ) {
										drop = Drop_Item( ent, item, 0 );
										// decide how many seconds it has left
										drop->count = ( ent->client->ps.powerups[ j ] - level.time ) / 1000;
										if ( drop->count < 1 ) {
											drop->count = 1;
										}

										ent->client->ps.powerups[ j ] = 0;
									}

								...
								...


								
									SelectSpawnPoint( ent->client->ps.origin, origin, angles );
									TeleportPlayer( ent, origin, angles );
									break;

								case EV_USE_ITEM2:		// medkit
									ent->health = ent->client->ps.stats[STAT_MAX_HEALTH] + 25;

									break;

								...
								...

								default:
									break;
								}
							}


					}







10.					

					vm.c

					int QDECL VM_Call()
					{

						...
						...

						// if we have a dll loaded, call it directly
						if ( vm->entryPoint ) {
							//rcg010207 -  see dissertation at top of VM_DllSyscall() in this file.
							va_start(ap, callnum);
							for (i = 0; i < sizeof (args) / sizeof (args[i]); i++) {
								args[i] = va_arg(ap, int);
							}
							va_end(ap);

							r = vm->entryPoint( callnum,  args[0],  args[1],  args[2], args[3],
					                            args[4],  args[5],  args[6], args[7],
					                            args[8],  args[9], args[10], args[11],
					                            args[12], args[13], args[14], args[15]);
						} else if ( ... ) {
							...
						} else {
							...
						}

						...
						...
					}








11.					


					g_main.c

					int vmMain()
					{


						switch(command)
						{

						...
						...

						case GAME_CLIENT_THINK:
							ClientThink( arg0 );
							return 0;
						}

						...
						...

					}








12.	






