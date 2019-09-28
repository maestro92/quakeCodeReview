
#############################################################################################
################################# Server Loading the Map ####################################
#############################################################################################

1.	so when the server startsup the map this is what happens



					void Com_Frame( void ) 
					{

						...
						...
						Cbuf_Execute ();
						...
						...



					}



2.					

					cmd.c

					void Cbuf_Execute (void)
					{
						...
						...

						Cmd_ExecuteString (line);	
					}


3.	we go on an execute the command 

					void	Cmd_ExecuteString( const char *text ) 
					{	
						cmd_function_t	*cmd, **prev;

						for ( prev = &cmd_functions ; *prev ; prev = &cmd->next ) 
						{


							cmd->function ();
						}
					}


4.	and we load up the map

					sv_ccmds.c

					static void SV_Map_f( void ) 
					{

						...
						...

						// start up the map
						SV_SpawnServer( mapname, killBots );

						...
						...
					}




5.					
					change the server to a new map, taking all connected clients along with it 
					this is not called for map_restart

					sv_init.c

					void SV_SpawnServer()
					{

						...
						...

						CM_LoadMap( va("maps/%s.bsp", server), qfalse, &checksum );

						...
						...
					}


6.					

					cm_load.c

					void CM_LoadMap()
					{


						CMod_LoadNodes (&header.lumps[LUMP_NODES]);
					}






7.	and you can see here how we initalize all the nodes

					cm_load.c

					void CMod_LoadNodes()
					{

						...
						...

						cm.nodes = Hunk_Alloc( count * sizeof( *cm.nodes ), h_high );
						cm.numNodes = count;

						out = cm.nodes;
				

						for (i=0 ; i<count ; i++, out++, in++)
						{
							out->plane = cm.planes + LittleLong( in->planeNum );
							for (j=0 ; j<2 ; j++)
							{
								child = LittleLong (in->children[j]);
								out->children[j] = child;
							}
						}

					}













#############################################################################################
################################# Client Loading the Map ####################################
#############################################################################################



1. 	So if you are a client connecting to a remote server, 
	the map is loaded when the server sends you a gamestate message
	

					CL_PacketEvent()
					{

						...
						CL_ParseServerMessage( msg );

						...
						...
					}





2.	
					cl_parse.c

					void CL_ParseServerMessage( msg_t *msg ) 
					{
						...
						...

						case svc_gamestate:
							printf("	### svc_gamestate\n");
							CL_ParseGamestate( msg );
							break;

						...
					}




3.					cl_parse.c

					void CL_ParseGamestate( msg_t *msg ) 
					{
						...

						// This used to call CL_StartHunkUsers, but now we enter the download state before loading the
						// cgame
						CL_InitDownloads();


					}




4.					

					cl_main.c

					void CL_InitDownloads()
					{


						...
						...

						CL_DownloadsComplete();
					}




5.					
					cl_main.c

					void CL_DownloadsComplete(void)
					{
						...
						...

						CL_InitCGame();
					
						...
						...
					}


6.					

					cl_cgame.c

					void CL_InitCGame( void ) 
					{
						...
						...

						// init for this gamestate
						// use the lastExecutedServerCommand instead of the serverCommandSequence
						// otherwise server commands sent just before a gamestate are dropped
						VM_Call( cgvm, CG_INIT, clc.serverMessageSequence, clc.lastExecutedServerCommand, clc.clientNum );

						...
						...
					}



7.					
					cg_main.c

					int vmMain(  ) 
					{
						switch ( command ) 
						{
							case CG_INIT:
								CG_Init( arg0, arg1, arg2 );
								return 0;

							...
							...
							...
						}
					}


7.					
					cg_main.c

					void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum ) 
					{
						...
						...

						trap_CM_LoadMap( cgs.mapname );

						...
						...

						CG_RegisterGraphics();
					
						...
						...
					}



8.					
					cl_cgame.c

					int CL_CgameSystemCalls( int *args ) 
					{
						...
						...

						case CG_CM_LOADMAP:
							CL_CM_LoadMap( VMA(1) );
							return 0;				
						...
						...
					}



9.	
					cl_cgame.c

					void CL_CM_LoadMap( const char *mapname ) 
					{
						int		checksum;

						CM_LoadMap( mapname, qtrue, &checksum );
					}


10.
					cm_load.c

					void CM_LoadMap( const char *name, qboolean clientload, int *checksum ) 
					{
						...
						...


						CMod_LoadLeafBrushes (&header.lumps[LUMP_LEAFBRUSHES]);
						CMod_LoadLeafSurfaces (&header.lumps[LUMP_LEAFSURFACES]);
						CMod_LoadPlanes (&header.lumps[LUMP_PLANES]);

						...
						...

						CMod_LoadNodes (&header.lumps[LUMP_NODES]);
					
						...
						...
					}



11.					

#############################################################################################
################################# Client Rendering ##########################################
#############################################################################################



On the rendering side, the nodes are loaded again


8.						
					cg_main.c

					static void CG_RegisterGraphics(void)
					{
						...
						...

						trap_R_LoadWorldMap( cgs.mapname );		// example: cgs.mapname = maps/Q3DM1.bsp

						...
						...
					}


					cg_syscalls.c

					void	trap_R_LoadWorldMap( const char *mapname ) 
					{
						syscall( CG_R_LOADWORLDMAP, mapname );
					}



9.					
					cl_cgame.c

					int CL_CgameSystemCalls( int *args ) 
					{
						...
						...

						case CG_R_LOADWORLDMAP:
							re.LoadWorld( VMA(1) );
							return 0; 

						...
						...
					}



10.					

					tr_bsp.c

					RE_LoadWorldMap()
					{


						...
						...
						R_LoadNodesAndLeafs (&header->lumps[LUMP_NODES], &header->lumps[LUMP_LEAFS]);
						...
						...
					}


11.	

					tr_bsp.c

					static	void R_LoadNodesAndLeafs (lump_t *nodeLump, lump_t *leafLump) 
					{



					}









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
						...
					}


