
So this article will talk about how the interpolation is done in the code base

before we go deep into the functions and stack calls, i want to first talk about the the client entity,

so the client has its own struct to represent entities, obviously this struct is more geared towards rendering, so they will have different 
stuff in contrast to the entity struct that the server has 

lets take a look at it. I presume centity means client entity 


					cg_local.h
					
					typedef struct centity_s {
						entityState_t	currentState;	// from cg.frame
						entityState_t	nextState;		// from cg.nextFrame, if available
						qboolean		interpolate;	// true if next is valid to interpolate to
						qboolean		currentValid;	// true if cg.frame holds this entity

						...
						...
						...

						// exact interpolated position of entity on this frame
						vec3_t			lerpOrigin;
						vec3_t			lerpAngles;
					} centity_t;


I have included the major flags that are related to interpolations. Specifically, the lerping will happen between currentState and nextState

interestingly in quake2, these two are referred as prev and current 






all the interpolation for all the entities for the current frame is done when we build the render list in CG_DrawActiveFrame(){}

1.	if we look inside the function, you can see we build the render list in CG_AddPacketEntities(){};

					cg_view.c

					void CG_DrawActiveFrame()
					{


						// set up cg.snap and possibly cg.nextSnap
						CG_ProcessSnapshots();


						CG_PredictPlayerState();
						...
						...

						// build the render lists
						if ( !cg.hyperspace ) {
							CG_AddPacketEntities();			// adter calcViewValues, so predicted player state is correct
							CG_AddMarks();
							CG_AddParticles ();
							CG_AddLocalEntities();
						}
						CG_AddViewWeapon( &cg.predictedPlayerState );

					}




2.	let us look at what does CG_ProcessSnapshots(){} does					

	according to words of http://www.ra.is/unlagged/network.html
	their description is 

	"
		Almost immediately, the client game checks for a new snapshot. 
		If there is a new one, it sets cg.snap to the old one and cg.nextSnap to the new one. 
		If there is not one at all, it sets cg.snap to the old one and cg.nextSnap to NULL. (See CG_ProcessSnapshots() in cg_snapshot.c);
	"

	also, on their next page 

	"
		Quake 3's “built-in 50ms lag” is actually only 50ms when the server is running at 20 FPS (sv_fps 20) – which most do. 
		It's caused by the need to interpolate rather than extrapolate the states of other players. 
		To interpolate, you need two known values, so in this case you need two snapshots. 
		The client engine waits, then, for one extra snapshot before releasing the most current snapshot to the client game. 
		What you see is smooth and mostly like what actually happened on the server, but 50ms behind what your ping tells you it should be.
	"


	so essentially, we are looking to assign values for cg.snap and cg.nextSnap. once we have these two, we have enought to interpolate
	This is totally different from how quake2 does it. Quake2 does not wait in the current frame to get a new frame, it just takes the last snapshot obtained
	and lerp from there

					cg_snapshot.c

					void CG_ProcessSnapshots()
					{


						...
						...

						while ( !cg.snap ) {
							snap = CG_ReadNextSnapshot();
							if ( !snap ) {
								// we can't continue until we get a snapshot
								return;
							}

							// set our weapon selection to what
							// the playerstate is currently using
							if ( !( snap->snapFlags & SNAPFLAG_NOT_ACTIVE ) ) {
								CG_SetInitialSnapshot( snap );
							}
						}


						...
						...
						...


						do {
							// if we don't have a nextframe, try and read a new one in
							if ( !cg.nextSnap ) {
								snap = CG_ReadNextSnapshot();

								// if we still don't have a nextframe, we will just have to
								// extrapolate
								if ( !snap ) {
									break;
								}

								CG_SetNextSnap( snap );

								// if time went backwards, we have a level restart
								if ( cg.nextSnap->serverTime < cg.snap->serverTime ) {
									CG_Error( "CG_ProcessSnapshots: Server time went backwards" );
								}
							}

							// if our time is < nextFrame's, we have a nice interpolating state
							if ( cg.time >= cg.snap->serverTime && cg.time < cg.nextSnap->serverTime ) {
								break;
							}

							// we have passed the transition from nextFrame to frame
							CG_TransitionSnapshot();
						} while ( 1 );

						...
						...
						...
					}









3.	you can see that the value dest is initialized with the address of cg.activeSnapshots[0] / [1], so when we 
	call trap_GetSnapshot(){} on dest, that will also populate the value in cg.activeSnapshots[0] / [1]

					cg_snapshot.c


					static snapshot_t *CG_ReadNextSnapshot( void ) 
					{

						...
						snapshot_t	*dest;

						...
						...

						while ( cgs.processedSnapshotNum < cg.latestSnapshotNum ) {
							// decide which of the two slots to load it into
							if ( cg.snap == &cg.activeSnapshots[0] ) {
								dest = &cg.activeSnapshots[1];
							} else {
								dest = &cg.activeSnapshots[0];
							}

							// try to read the snapshot from the client system
							cgs.processedSnapshotNum++;
							r = trap_GetSnapshot( cgs.processedSnapshotNum, dest );

						}

						...
						...
					}




4.	the trap_GetSnapshot(){} will call CL_GetSnapshot(){} eventually, after going through a bunch of quake systemcall. 
					

					cg_syscalls.c

					qboolean	trap_GetSnapshot( int snapshotNumber, snapshot_t *snapshot ) {
						return syscall( CG_GETSNAPSHOT, snapshotNumber, snapshot );
					}


					cl_cgame.c

					int CL_CgameSystemCalls( int *args ) 
					{
						...
						...

						case CG_GETSNAPSHOT:
							return CL_GetSnapshot( args[1], VMA(2) );
					
						...
						...
					}




5.	Now lets see how CL_GetSnapshot(){} works, you might have noticed that it is checking if cl.snap is valid or not 
	and if you recall, in the CL_ReadPackets(){}, CL_ParseServerSnapshot(){}, cl.snapshots is excatly where we put our new snapshot 



					cl_cgame.c

					qboolean CL_GetSnapshot()
					{

						// if the frame has fallen out of the circular buffer, we can't return it
						if ( cl.snap.messageNum - snapshotNumber >= PACKET_BACKUP ) {
							return qfalse;
						}

						// if the frame is not valid, we can't return it
						clSnap = &cl.snapshots[snapshotNumber & PACKET_MASK];
						if ( !clSnap->valid ) {
							return qfalse;
						}

						// if the entities in the frame have fallen out of their
						// circular buffer, we can't return it
						if ( cl.parseEntitiesNum - clSnap->parseEntitiesNum >= MAX_PARSE_ENTITIES ) {
							return qfalse;
						}

						...
						...
						...
						...

						snapshot->numEntities = count;
						for ( i = 0 ; i < count ; i++ ) {
							snapshot->entities[i] = 
								cl.parseEntities[ ( clSnap->parseEntitiesNum + i ) & (MAX_PARSE_ENTITIES-1) ];
						}

						// FIXME: configstring changes and server commands!!!

						return qtrue;

					}


6.	So now looking back at CG_ProcessSnapshots(){}, it is not surprising where we call CG_ReadNextSnapshot(){} for both cl.snap and 
cl.nextSnap. so essentially, if cl.snap and cl.nextSnap is valid, we interpolate, if cl.nextSnap is invalid, we extrpolate. 




####################################################################################################
##################################### CG_AddPacketEntities(){} #####################################
####################################################################################################




2.	there are a lot of interesting happening in the CG_AddPacketEntities(){} function

	-	recall in the CG_DrawActiveFrame(){} function, we called CG_PredictPlayerState(){} for the local player prediction

		so in the CG_AddPacketEntities(){}, the first thing we did is 

					ps = &cg.predictedPlayerState;
					BG_PlayerStateToEntityState( ps, &cg.predictedPlayerEntity.currentState, qfalse );
					CG_AddCEntity( &cg.predictedPlayerEntity );

		so we add our player to the entity list. We will talk about each function in great detail later.


	-	you can also see how the cg.frameInterpolation is calcualted. this is lerp fraction that will be used when try to 
		interpolate entities. you can see that with the 

					if( cg.nextSnap )

		check, we will give a different cg.frameInterpolation value if we have received the next snapshot. If we did not get a snapshot, it 
		will just be cg.frameInterpolation = 0;

		
					




					cg_ents.c

					/*
					===============
					CG_AddPacketEntities

					===============
					*/
					void CG_AddPacketEntities( void ) 
					{
						int					num;
						centity_t			*cent;
						playerState_t		*ps;

						// set cg.frameInterpolation
						if ( cg.nextSnap ) 
						{
							int		delta;

							delta = (cg.nextSnap->serverTime - cg.snap->serverTime);
							if ( delta == 0 ) 
							{
								cg.frameInterpolation = 0;
							} 
							else 
							{
								cg.frameInterpolation = (float)( cg.time - cg.snap->serverTime ) / delta;
							}
						} 
						else 
						{
							cg.frameInterpolation = 0;	// actually, it should never be used, because 
														// no entities should be marked as interpolating
						}

						// the auto-rotating items will all have the same axis
						cg.autoAngles[0] = 0;
						cg.autoAngles[1] = ( cg.time & 2047 ) * 360 / 2048.0;
						cg.autoAngles[2] = 0;

						cg.autoAnglesFast[0] = 0;
						cg.autoAnglesFast[1] = ( cg.time & 1023 ) * 360 / 1024.0f;
						cg.autoAnglesFast[2] = 0;

						AnglesToAxis( cg.autoAngles, cg.autoAxis );
						AnglesToAxis( cg.autoAnglesFast, cg.autoAxisFast );

						// generate and add the entity from the playerstate
						ps = &cg.predictedPlayerState;
						BG_PlayerStateToEntityState( ps, &cg.predictedPlayerEntity.currentState, qfalse );
						CG_AddCEntity( &cg.predictedPlayerEntity );

						// lerp the non-predicted value for lightning gun origins
						CG_CalcEntityLerpPositions( &cg_entities[ cg.snap->ps.clientNum ] );

						// add each entity sent over by the server
						for ( num = 0 ; num < cg.snap->numEntities ; num++ ) {
							cent = &cg_entities[ cg.snap->entities[ num ].number ];
							CG_AddCEntity( cent );
						}
					}








3.	recall that playerState is like a superset of entityState, we can derive entityState from playerState,
	so that is what we are doing in th is function. we called this function by passing in the two arguments 

					BG_PlayerStateToEntityState( ps, &cg.predictedPlayerEntity.currentState, qfalse );

	and that will populate the correct value into cg.predictedPlayerState.currentState.


					/*
					========================
					BG_PlayerStateToEntityState

					This is done after each set of usercmd_t on the server,
					and after local prediction on the client
					========================
					*/

					bg_misc.c

					void BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qboolean snap ) 
					{
						// if you look inside the code, it's prety much all s->xxxx  assignments

						for example: 

							...

						s->angles2[YAW] = ps->movementDir;
						s->legsAnim = ps->legsAnim;
						s->torsoAnim = ps->torsoAnim;
						s->clientNum = ps->clientNum;		// ET_PLAYER looks here instead of at number

						...
						...

						// u get the idea...
					}	



4.	back to the CG_AddPacketEntities(){} function, we look at CG_AddCEntity( &cg.predictedPlayerEntity );, which adds our player to the renderlist
	you can see that this function will be used for all entities, and the switch case conveniently calls the CG_ function depending on the type

	I have listed only two examples here

					cg_ents.c

					void CG_AddCEntity( centity_t* cent)
					{

						...

						// calculate the current origin
						CG_CalcEntityLerpPositions( cent );

						// add automatic effects
						CG_EntityEffects( cent );

						...
						...
						switch ( cent->currentState.eType ) 
						{
							...
							...

							case ET_PLAYER:
								printf("ET_PLAYER");
								CG_Player( cent );

							case ET_ITEM:
								CG_Item( cent );
								break;

							...
							...
						}

					}







5.	like what the comments said: this function "calculate the current origin". 
	-	so there are multiple scenarios, some of them we want to interpolate, others we want to linear extrpolate

		so depending on the currentState.pos.trType, we do things differently


	-	you can see that if the client "does not want to see extrapolated players", we have the cg_smoothClients flag on

		and we manually set the TR_INTERPOLATE flag to currentState and nextState. these two states are the two we will be interpolating between


					cg_ents.c

					void CG_CalcEntityLerpPositions()
					{

						// if this player does not want to see extrapolated players
						if ( !cg_smoothClients.integer ) 
						{
							// make sure the clients use TR_INTERPOLATE
							if ( cent->currentState.number < MAX_CLIENTS ) 
							{
								cent->currentState.pos.trType = TR_INTERPOLATE;
								cent->nextState.pos.trType = TR_INTERPOLATE;
							}
						}


						if ( cent->interpolate && cent->currentState.pos.trType == TR_INTERPOLATE ) 
						{
							CG_InterpolateEntityPosition( cent );
							return;
						}

						// first see if we can interpolate between two snaps for
						// linear extrapolated clients
						if ( cent->interpolate && cent->currentState.pos.trType == TR_LINEAR_STOP &&
																cent->currentState.number < MAX_CLIENTS) {
							CG_InterpolateEntityPosition( cent );
							return;
						}

						// just use the current frame and evaluate as best we can
						BG_EvaluateTrajectory( &cent->currentState.pos, cg.time, cent->lerpOrigin );
						BG_EvaluateTrajectory( &cent->currentState.apos, cg.time, cent->lerpAngles );

						...
						...
					}




6.	the interpolate function is very straightforward. Recall the important here is the "f = cg.frameInterpolation"
	
	that is calculated in the CG_AddPacketEntities(){} function call	

	-	you can also see that we do a BG_EvaluateTrajectory(){} call, which gives us "vec3_t curret" and "vec3_t next", 
		and that is the two values we are interpolating between 


	-	you might also wonder how is cent->nextState calcualted


					cg_ents.c

					static void CG_InterpolateEntityPosition( centity_t *cent ) 
					{
						vec3_t		current, next;
						float		f;

						// it would be an internal error to find an entity that interpolates without
						// a snapshot ahead of the current one
						if ( cg.nextSnap == NULL ) {
							CG_Error( "CG_InterpoateEntityPosition: cg.nextSnap == NULL" );
						}

						f = cg.frameInterpolation;

						// this will linearize a sine or parabolic curve, but it is important
						// to not extrapolate player positions if more recent data is available
						BG_EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, current );
						BG_EvaluateTrajectory( &cent->nextState.pos, cg.nextSnap->serverTime, next );

						cent->lerpOrigin[0] = current[0] + f * ( next[0] - current[0] );
						cent->lerpOrigin[1] = current[1] + f * ( next[1] - current[1] );
						cent->lerpOrigin[2] = current[2] + f * ( next[2] - current[2] );

						BG_EvaluateTrajectory( &cent->currentState.apos, cg.snap->serverTime, current );
						BG_EvaluateTrajectory( &cent->nextState.apos, cg.nextSnap->serverTime, next );

						cent->lerpAngles[0] = LerpAngle( current[0], next[0], f );
						cent->lerpAngles[1] = LerpAngle( current[1], next[1], f );
						cent->lerpAngles[2] = LerpAngle( current[2], next[2], f );


					}






7.	Now if you go back to CG_AddPacketEntities(){}, you can see that after dealing with the players, we start looping through the entities
	and call CG_AddEntity(){} on all of them, so it is more or less the same thing :)


					void CG_AddPacketEntities( void ) 
					{

						...
						...

						// add each entity sent over by the server
						for ( num = 0 ; num < cg.snap->numEntities ; num++ ) 
						{
							cent = &cg_entities[ cg.snap->entities[ num ].number ];
							CG_AddCEntity( cent );
						}

						...
						...
					}




						// add the gun / barrel / flash
						CG_AddPlayerWeapon( &torso, NULL, cent, ci->team );


						CG_PlayerPowerups( cent, &torso );









4.					

					cg_snapshot.c

					void CG_SentInitialSnapshot( snapshot_t *snap )
					{

						...
						BG_PlayerStateToEntityState( &snap->ps, &cg_entities[ snap->ps.clientNum ].currentState, qfalse );	

						...
						...

						for ( i = 0 ; i < cg.snap->numEntities ; i++ ) 
						{
							state = &cg.snap->entities[ i ];
							cent = &cg_entities[ state->number ];

							memcpy(&cent->currentState, state, sizeof(entityState_t));
							//cent->currentState = *state;
							cent->interpolate = qfalse;
							cent->currentValid = qtrue;

							CG_ResetEntity( cent );

							// check for events
							CG_CheckEvents( cent );
						}
					}

