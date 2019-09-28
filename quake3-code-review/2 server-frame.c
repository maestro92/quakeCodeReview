

so I assume u just came from main-code.c and want to delve into the server code. This is the right place. 

1.	back to square one, which is looking at the entry point of the server code from Com_Frame(){}



					quake3/common.c

					void Com_Frame(void)
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
						Cbuf_Execute ();

						...
						...


						//
						// server side
						//
						if ( com_speeds->integer ) {
							timeBeforeServer = Sys_Milliseconds ();
						}

						SV_Frame( msec );
						// if "dedicated" has been modified, start up
						// or shut down the client system.
						// Do this after the server may have started,
						// but before the client tries to auto-connect
						if ( com_dedicated->modified ) {
							// get the latched value
							Cvar_Get( "dedicated", "0", 0 );
							com_dedicated->modified = qfalse;
							if ( !com_dedicated->integer ) {
								CL_Init();
								Sys_ShowConsole( com_viewlog->integer, qfalse );
							} else {
								CL_Shutdown();
								Sys_ShowConsole( 1, qtrue );
							}
						}



						...
						...
						...

					}




2.	so let us look at what is going on in SV_Frame(){}


			
					quake3/sv_main.c

					void SV_Frame()
					{





						// run the game simulation in chunks
						while ( sv.timeResidual >= frameMsec ) {
							sv.timeResidual -= frameMsec;
							svs.time += frameMsec;

							// let everything in the world think and move
							VM_Call( gvm, GAME_RUN_FRAME, svs.time );
						}


						...
						...


						// update ping based on the all received frames
						SV_CalcPings();


						...
						...

						// check timeouts
						SV_CheckTimeouts();

						// send messages back to the clients
						SV_SendClientMessages();

						// send a heartbeat to the master if needed
						SV_MasterHeartbeat();
						...
						...

					}





3.	here we look at how VM_CALL works

					quake3/vm.c

					int QDCL VM_CALL(vm_t* vm, int callnum, ...)
					{

						...
						...

						if ( vm_debugLevel ) {
						  Com_Printf( "VM_Call( %i )\n", callnum );
						}

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





4.	here you can see the entire switch case of commands in the vmMain(){} function

	-	all the command enums are actually listed in the g_public.h file
					//
					// functions exported by the game subsystem
					//
					typedef enum {
						GAME_INIT,	// ( int levelTime, int randomSeed, int restart );
						// init and shutdown will be called every single level
						// The game should call G_GET_ENTITY_TOKEN to parse through all the
						// entity configuration text and spawn gentities.

						GAME_SHUTDOWN,	// (void);

						GAME_CLIENT_CONNECT,	// ( int clientNum, qboolean firstTime, qboolean isBot );
						// return NULL if the client is allowed to connect, otherwise return
						// a text string with the reason for denial

						GAME_CLIENT_BEGIN,				// ( int clientNum );

						GAME_CLIENT_USERINFO_CHANGED,	// ( int clientNum );

						GAME_CLIENT_DISCONNECT,			// ( int clientNum );

						GAME_CLIENT_COMMAND,			// ( int clientNum );

						GAME_CLIENT_THINK,				// ( int clientNum );

						GAME_RUN_FRAME,					// ( int levelTime );

						GAME_CONSOLE_COMMAND,			// ( void );
						// ConsoleCommand will be called when a command has been issued
						// that is not recognized as a builtin function.
						// The game can issue trap_argc() / trap_argv() commands to get the command
						// and parameters.  Return qfalse if the game doesn't recognize it as a command.

						BOTAI_START_FRAME				// ( int time );
					} gameExport_t;



	-	when the server runs the game from in SV_Frame(){}, it will run the GAME_RUN_FRAME command 

		so let us look into the case:GAME_RUN_FRAME


					game/g_main.c

					int vmMain()
					{

						switch ( command )
						{
						case GAME_INIT:
							G_InitGame( arg0, arg1, arg2 );
							return 0;

						...
						...	// more cases
						...

						case GAME_RUN_FRAME:
							G_RunFrame( arg0 );
							return 0;

						...
						...	// more cases
						...

						case GAME_CONSOLE_COMMAND:
							return ConsoleCommand();
						}

						return -1;


					}




5.	

	-	for the case of

				if ( i < MAX_CLIENTS ) {
					G_RunClient( ent );
					continue;
				}

		recall in the quake and quake2, it was designed that the first few entities were reserved for clients
		hence when for these indexes, we run G_RunClient(){}



					g_main.c

					/*
					================
					G_RunFrame

					Advances the non-player objects in the world
					================
					*/
					void G_RunFrame()
					{

						...
						...

						ent = &g_entities[0];
						for (i=0 ; i<level.num_entities ; i++, ent++) 
						{
							if ( !ent->inuse ) 
							{
								continue;
							}

							// clear events that are too old
							if ( level.time - ent->eventTime > EVENT_VALID_MSEC ) 
							{
								if ( ent->s.event ) {
									ent->s.event = 0;	// &= EV_EVENT_BITS;
									if ( ent->client ) {
										ent->client->ps.externalEvent = 0;
										// predicted events should never be set to zero
										//ent->client->ps.events[0] = 0;
										//ent->client->ps.events[1] = 0;
									}
								}
								if ( ent->freeAfterEvent ) {
									// tempEntities or dropped items completely go away after their event
									G_FreeEntity( ent );
									continue;
								} else if ( ent->unlinkAfterEvent ) {
									// items that will respawn will hide themselves after their pickup event
									ent->unlinkAfterEvent = qfalse;
									trap_UnlinkEntity( ent );
								}
							}

							// temporary entities don't think
							if ( ent->freeAfterEvent ) {
								continue;
							}

							if ( !ent->r.linked && ent->neverFree ) {
								continue;
							}

							if ( ent->s.eType == ET_MISSILE ) {
								G_RunMissile( ent );
								continue;
							}

							if ( ent->s.eType == ET_ITEM || ent->physicsObject ) {
								G_RunItem( ent );
								continue;
							}

							if ( ent->s.eType == ET_MOVER ) {
								G_RunMover( ent );
								continue;
							}

							if ( i < MAX_CLIENTS ) {
								G_RunClient( ent );
								continue;
							}

							G_RunThink( ent );
						}


					}






6.	as mentioned in the previous step, lets look at the 


					g_active.c

					void G_RunClient(gentity_t *ent)
					{
						if ( !(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer ) {
							return;
						}
						ent->client->pers.cmd.serverTime = level.time;
						ClientThink_real( ent );

					}



					g_active.c

					void ClientThink_real(gentity_t *ent)
					{

						...
						...

						Pmove(&pm);


						...
						...
						SendPendingPredictableEvents( &ent->client->ps );


						// execute client events
						ClientEvents( ent, oldEventSequence );


						...
						...

						// touch other objects
						ClientImpacts( ent, &pm );


						...
						...

						// perform once-a-second actions
						ClientTimerActions( ent, msec );


					}






6.	Just like in quake2, each entity will get called G_RunThink(ent){}


	-	what is interesting is that ent->think(ent){} is dynamic. Meaning it is different all the time

					g_main.c

					void G_RunThink(genti)
					{
						float	thinktime;

						thinktime = ent->nextthink;
						if (thinktime <= 0) {
							return;
						}
						if (thinktime > level.time) {
							return;
						}
						
						ent->nextthink = 0;
						if (!ent->think) {
							G_Error ( "NULL ent->think");
						}
						ent->think (ent);

					}








