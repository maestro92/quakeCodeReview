



1.
					cl_scm.c

					SCR_DrawScreenField()
					{
						re.BeginFrame(steroFrame);


						...
						...

						if ( !VM_Call( uivm, UI_IS_FULLSCREEN )) 
						{
							switch( cls.state ) 
							{

								...
								...

								case CA_ACTIVE:
									CL_CGameRendering( stereoFrame );
									SCR_DrawDemoRecording();
									break;

								...
								...

							}
						}
					}








					tr_cmds.c

					void RE_BeginFrame()
					{


					}






3.					


					cl_cgame.c


					void CL_CGameRendering( stereoFrame_t stereo ) 
					{
						VM_Call( cgvm, CG_DRAW_ACTIVE_FRAME, cl.serverTime, stereo, clc.demoplaying );
						VM_Debug( 0 );
					}






4.					

					cg_main.c

					int vmMain()
					{
						switch(command)
						{


							...
							...

							case CG_DRAW_ACTIVE_FRAME:
								CG_DrawActiveFrame( arg0, arg1, arg2 );
								return 0;

							...
							...
						}


					}



5.					

					cg_view.c

					void CG_DrawActiveFrame()
					{


						...
						...

						// set up cg.snap and possibly cg.nextSnap
						CG_ProcessSnapshots();



						// this counter will be bumped for every valid scene we generate
						cg.clientFrame++;

						// update cg.predictedPlayerState
						CG_PredictPlayerState();


						...
						...

						// actually issue the rendering calls
						CG_DrawActive( stereoView );

					}






5.					

					cg_snapshot.c

					void CG_ProcessSnapshots()
					{


					}


















6.					

					cg_draw.c

					void CG_DrawActive()
					{

						// draw 3D view
						trap_R_RenderScene( &cg.refdef );


						...
						...

						// draw status bar and other floating elements
					 	CG_Draw2D();

					}





7.					
					cg_syscalls.c

					void	trap_R_RenderScene( const refdef_t *fd ) {
						syscall( CG_R_RENDERSCENE, fd );
					}




					vm.c

					int QDECL VM_DllSyscall( int arg, ... ) 
					{
						...
						...

						return currentVM->systemCall( &arg );
					}





8.					

					cl_cgame.c

					int CL_CgameSystemCalls()
					{

						...
						...

						case CG_R_RENDERSCENE:
							re.RenderScene( VMA(1) );
							return 0;

						...
						...
					}



9.		
					
					tr_scene.c

					void RE_RenderScene()
					{

						...
						...

						tr.refdef.num_entities = r_numentities - r_firstSceneEntity;
						tr.refdef.entities = &backEndData[tr.smpFrame]->entities[r_firstSceneEntity];

						...
						...

						R_RenderView( &parms );

						...
						...

					}




10.

					tr_main.c

					void R_RenderView()
					{

						...
						...

						// set viewParms.world
						R_RotateForViewer ();

						R_SetupFrustum ();

						R_GenerateDrawSurfs();

						R_SortDrawSurfs( tr.refdef.drawSurfs + firstDrawSurf, tr.refdef.numDrawSurfs - firstDrawSurf );

						// draw main system development information (surface outlines, etc)
						R_DebugGraphics();

					}







11.					

					tr_main.c

					void R_GenerateDrawSurfs()
					{
						R_AddWorldSurfaces ();

						R_AddPolygonSurfaces();

						// set the projection matrix with the minimum zfar
						// now that we have the world bounded
						// this needs to be done before entities are
						// added, because they use the projection
						// matrix for lod calculation
						R_SetupProjection ();

						R_AddEntitySurfaces ();					
					}




12.					

					tr_main.c

					void R_AddEntitySurfaces()
					{

						...
						...

						for ( tr.currentEntityNum = 0; 
					      tr.currentEntityNum < tr.refdef.num_entities; 
						  tr.currentEntityNum++ ) 
						{
							ent = tr.currentEntity = &tr.refdef.entities[tr.currentEntityNum];


							...
							...



						}
					}












13.					



					void RE_AddRefEntityToScene( const refEntity_t *ent ) {

						...
						...

						backEndData[tr.smpFrame]->entities[r_numentities].e = *ent;
						backEndData[tr.smpFrame]->entities[r_numentities].lightingCalculated = qfalse;

						r_numentities++;
					}




14

					cg_player.c

					CG_AddRefEntityWithPowerups()
					{


						trap_R_AddRefEntityToScene( ent );

					}




					cl_cgame.c

					int CL_CgameSystemCalls( int *args ) 
					{

						case CG_R_ADDREFENTITYTOSCENE:
							re.AddRefEntityToScene( VMA(1) );
							return 0;

					}



					tr_scene.c

					void RE_AddRefEntityToScene(const refEntity_t* ent)
					{
						...
						...


						backEndData[tr.smpFrame]->entities[r_numentities].e = *ent;
						backEndData[tr.smpFrame]->entities[r_numentities].lightingCalculated = qfalse;

						r_numentities++;
					}







#########################################################################################
############################ Add Packte Entity For Rendering ############################
#########################################################################################

1.					

					cg_ents.c

					CG_AddPacketENtities()
					{

						...
						...

						// generate and add the entity from the playerstate
						ps = &cg.predictedPlayerState;
						BG_PlayerStateToEntityState( ps, &cg.predictedPlayerEntity.currentState, qfalse );
						CG_AddCEntity( &cg.predictedPlayerEntity );

						// lerp the non-predicted value for lightning gun origins
						CG_CalcEntityLerpPositions( &cg_entities[ cg.snap->ps.clientNum ] );

						// add each entity sent over by the server
						for ( num = 0 ; num < cg.snap->numEntities ; num++ ) 
						{
							cent = &cg_entities[ cg.snap->entities[ num ].number ];
							CG_AddCEntity( cent );
						}
					}



2.					
					cg_ents.c

					CG_AddCEntity()
					{
						...
						...
						case ET_PLAYER:
							CG_Player( cent );
							break;
					
						...
						...
					}



3.

					cg_players.c

					void CG_Player( centity_t *cent ) 
					{

						...
						...
						CG_AddRefEntityWithPowerups( &legs, &cent->currentState, ci->team );

						...
						...
					}



4.

					cg_player.c

					CG_AddRefEntityWithPowerups()
					{
						...
						...

						trap_R_AddRefEntityToScene( ent );

						...
						...
					}


5.

					cl_cgame.c

					int CL_CgameSystemCalls( int *args ) 
					{
						...
						...

						case CG_R_ADDREFENTITYTOSCENE:
							re.AddRefEntityToScene( VMA(1) );
							return 0;

						...
						...
					}


6.
					tr_scene.c

					void RE_AddRefEntityToScene(const refEntity_t* ent)
					{
						...
						...


						backEndData[tr.smpFrame]->entities[r_numentities].e = *ent;
						backEndData[tr.smpFrame]->entities[r_numentities].lightingCalculated = qfalse;

						r_numentities++;
					}








7.					

					tr_cmds.c

					void R_IssueRenderCommands()
					{
						renderCommandList_t	*cmdList;

						cmdList = &backEndData[tr.smpFrame]->commands;

						
						...
						...		

				
						// actually start the commands going
						if ( !r_skipBackEnd->integer ) {
							// let it start on the new batch
							if ( !glConfig.smpActive ) {
								RB_ExecuteRenderCommands( cmdList->cmds );
							} else {
								GLimp_WakeRenderer( cmdList );
							}
						}
					}



