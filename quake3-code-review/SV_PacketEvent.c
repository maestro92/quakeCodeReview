



1.	There is actually quite a bit of trace before arriving at SV_PacketEvent(){}. 



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



2.	This whole chunk may be a bit confusing, but the imporant line of code here is 
		
		com_frameTime = Com_EventLoop();




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



3.	Let us look into what happens in there

	
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

							...
							...
							...

						}



					}









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





2.					

					sv_client.c

					void SV_ExecuteClientMessage()
					{

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







3.	

					sv_client.c

					SV_UserMove()
					{


						...
						...



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

						trap_GetUsercmd( clientNum, &ent->client->pers.cmd );

						...
						...


						if ( !(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer ) {
							ClientThink_real( ent );
						}
					}





					g_syscall.c

					void trap_GetUsercmd( int clientNum, usercmd_t *cmd ) 
					{
						syscall( G_GET_USERCMD, clientNum, cmd );
					}



					vm.c

					int QDECL VM_DllSyscall( int arg, ... ) 
					{
						#if ((defined __linux__) && (defined __powerpc__))
							...
							...
						#else // original id code
							return currentVM->systemCall( &arg );
						#endif
					}




					sv.game.c

					int SV_GameSystemCalls( int *args ) 
					{

						case G_GET_USERCMD:
							SV_GetUsercmd( args[1], VMA(2) );
							return 0;
					}




					vm.c
					void *VM_ArgPtr( int intValue ) 
					{
						if ( !intValue ) {
							return NULL;
						}
						// bk001220 - currentVM is missing on reconnect
						if ( currentVM==NULL )
						  return NULL;

						if ( currentVM->entryPoint ) {
							return (void *)(currentVM->dataBase + intValue);
						}
						else {
							return (void *)(currentVM->dataBase + (intValue & currentVM->dataMask));
						}
					}



					sv_game.c

					void SV_GetUsercmd( int clientNum, usercmd_t *cmd ) {
						if ( clientNum < 0 || clientNum >= sv_maxclients->integer ) {
							Com_Error( ERR_DROP, "SV_GetUsercmd: bad clientNum:%i", clientNum );
						}
						*cmd = svs.clients[clientNum].lastUsercmd;
					}
8.					
					

					g_active.c

					void ClientThink_real()
					{
						...
						...



						// set up for pmove
						oldEventSequence = client->ps.eventSequence;

						...
						...
						...


						// save results of pmove
						if ( ent->client->ps.eventSequence != oldEventSequence ) {
							ent->eventTime = level.time;
						}

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
									...
									...

								case EV_FIRE_WEAPON:
									FireWeapon( ent );
									break;

									...
									...

								default:
									break;
								}
							}


					}




10.					
					g_weapon.c

					void FireWeaon(gentity_t* ent)
					{

						// fire the specific weapon
						switch( ent->s.weapon ) {
						
						...
						...

						case WP_SHOTGUN:
							weapon_supershotgun_fire( ent );
							break;
						case WP_MACHINEGUN:
							if ( g_gametype.integer != GT_TEAM ) {
								Bullet_Fire( ent, MACHINEGUN_SPREAD, MACHINEGUN_DAMAGE );
							} else {
								Bullet_Fire( ent, MACHINEGUN_SPREAD, MACHINEGUN_TEAM_DAMAGE );
							}
							break;

						...
						...

						case WP_GRAPPLING_HOOK:
							Weapon_GrapplingHook_Fire( ent );
							break;
					}





11.
					


