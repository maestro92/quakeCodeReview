
Server Collision Detection 


	

http://www.kbs.twi.tudelft.nl/docs/MSc/2001/Waveren_Jean-Paul_van/thesis.pdf








1.	So coming from SV_ClientThink, 

					bg_pmove.c

					void PmoveSingle()
					{

						...
						...


						else if ( pml.walking ) {
							// walking on ground
							PM_WalkMove();
						}
					}




2.
					bg_pmove.c

					static void PM_WalkMove()
					{
						...
						...

						PM_StepSlideMove( qfalse );

						...
					}





3.					

					bg_slidemove.c

					void PM_StepSlideMove( qboolean gravity ) 
					{

						...
						...

						if ( PM_SlideMove( gravity ) == 0 ) {
							return;		// we got exactly where we wanted to go first try	
						}

						...
						...
					}




4.					

					bg_slidemove.c


					qboolean PM_SlideMove( qboolean gravity ) 
					{

						...
						...
						
						for ( bumpcount=0 ; bumpcount < numbumps ; bumpcount++ ) 
						{

							...
							...

							// see if we can make it there
							pm->trace ( &trace, pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->clientNum, pm->tracemask);
						
							...
							...
						}


					}



5.	then the pm->trace

					sv_game.c

					int SV_GameSystemCalls( int *args ) 
					{
						...
						...

						case G_TRACE:
							SV_Trace( VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], /*int capsule*/ qfalse );
							return 0;

						...
						...
					}



6.				
					sv_world.c

					void SV_Trace()
					{
						...
						...


						// clip to world
						CM_BoxTrace( &clip.trace, start, end, mins, maxs, 0, contentmask, capsule );

						// clip to other solid entities
						SV_ClipMoveToEntities ( &clip );
					
						...
					}




7.					

					sv_world.c

					void SV_ClipMoveToEntities()
					{

						...
						...
						int	touchlist[MAX_GENTITIES];

						...
						...

						num = SV_AreaEntities( clip->boxmins, clip->boxmaxs, touchlist, MAX_GENTITIES);

						...
						...

						for ( i=0 ; i<num ; i++ ) 
						{
							...
							...

							// might intersect, so do an exact clip
							clipHandle = SV_ClipHandleForEntity (touch);

							...
							...
						}
					}




8.	



					int SV_AreaEntities( const vec3_t mins, const vec3_t maxs, int *entityList, int maxcount ) {
						areaParms_t		ap;

						ap.mins = mins;
						ap.maxs = maxs;
						ap.list = entityList;
						ap.count = 0;
						ap.maxcount = maxcount;

						SV_AreaEntities_r( sv_worldSectors, &ap );

						return ap.count;
					}


	-	we recursively add entities to the entityList

		I presume BSP tree works like a BSP tree?, which you have a node that stores all the entries there 

		then you do a aabb check? with each of them to check for collision. 


					void SV_AreaEntities_r()
					{



						for ( check = node->entities  ; check ; check = next ) {
							next = check->nextEntityInWorldSector;

							gcheck = SV_GEntityForSvEntity( check );

							if ( gcheck->r.absmin[0] > ap->maxs[0]
							|| gcheck->r.absmin[1] > ap->maxs[1]
							|| gcheck->r.absmin[2] > ap->maxs[2]
							|| gcheck->r.absmax[0] < ap->mins[0]
							|| gcheck->r.absmax[1] < ap->mins[1]
							|| gcheck->r.absmax[2] < ap->mins[2]) {
								continue;
							}

							if ( ap->count == ap->maxcount ) {
								Com_Printf ("SV_AreaEntities: MAXCOUNT\n");
								return;
							}

							ap->list[ap->count] = check - sv.svEntities;
							ap->count++;
						}
					
						...
						...

						// recurse down both sides
						if ( ap->maxs[node->axis] > node->dist ) {
							SV_AreaEntities_r ( node->children[0], ap );
						}
						if ( ap->mins[node->axis] < node->dist ) {
							SV_AreaEntities_r ( node->children[1], ap );
						}

					}



#######################################################################
###################### SV_ClipHandleForEntity #########################
#######################################################################


1.	after getting all the entities, we go into the for loop


					sv_world.c

					clipHandle_t SV_ClipHandleForEntity( const sharedEntity_t *ent ) 
					{


					}




2.					
					cm_load.c

					clipHandle_t CM_TempBoxModel( const vec3_t mins, const vec3_t maxs, int capsule ) 
					{



					}














#######################################################################################################
################################# Client Side Collision in Prediction #################################
#######################################################################################################

So on the client side, the collision detection is done when we do player prediction. (recall that CG_PredictPlayerState is in SCR_UpdateScreen)

1. 

					cg_predict.c

					void CG_PredictPlayerState()
					{

						...
						...

						for ( cmdNum = current - CMD_BACKUP + 1 ; cmdNum <= current ; cmdNum++ ) 
						{

							...
							Pmove (&cg_pmove);
							...
						}
					}



2.	we look into Pmove(){};

					bg_pmove.c

					void Pmove()
					{


						...
						...

						// chop the move up if it is too long, to prevent framerate
						// dependent behavior
						while ( pmove->ps->commandTime != finalTime ) 
						{
						
							...
							...

							PmoveSingle( pmove );

							...
							...
						}

					}


3.	the PmoveSingle does a couple of interesting things, I will explain each one	


					PmoveSingle()
					{

						...
						...

						// set groundentity
						PM_GroundTrace();

						...
						...

						else if ( pml.walking ) {
							// walking on ground
							PM_WalkMove();
						} 

						...
						...

						// set groundentity, watertype, and waterlevel
						PM_GroundTrace();
						PM_SetWaterLevel();

						...
						...
					}



#############################################################################
############################# PM_GroundTrace(){} ############################
#############################################################################

1.	let us take a look at PM_GroundTrace(){} first.

	I presume this function is called ground trace is becuz it tries to shoot a ray underneath
	to trace the ground. and the comments also says, it is setting the ground entity. so it makes sense

	-	you can see that what we are doing is we first initalize the value of "point" with the position of the player

	-	we pass in the the variable "trace" to the pm->trace function, and that will give us the result of the tracing result

	-	you can see in the subsequent if check "if ( trace.fraction == 1.0 ) ", the comments says we are in free fall if we dont have anything

		here "trace.fraction" is the percentage that we can travel in that direciton. Since if we are traveling downwards 100% (trace.fraction == 1.0)
		that certainly means we are in free fall.



					bg_pmove.c

					PM_GroundTrace()
					{
						vec3_t		point;
						trace_t		trace;

						point[0] = pm->ps->origin[0];
						point[1] = pm->ps->origin[1];
						point[2] = pm->ps->origin[2] - 0.25;

						pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
						pml.groundTrace = trace;

						...
						...

						// if the trace didn't hit anything, we are in free fall
						if ( trace.fraction == 1.0 ) {
							PM_GroundTraceMissed();
							pml.groundPlane = qfalse;
							pml.walking = qfalse;
							return;
						}

						...
						...

					}





2.	let us step through and see what pm->trace(){} is doing

	recall the server calls SV_Trace(){}, and on the client prediction side, we call this CG_Trace(){} function

					cg_predict.c

					void CG_Trace()
					{
						...
						...

						trap_CM_BoxTrace ( &t, start, end, mins, maxs, 0, mask);
						t.entityNum = t.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
						// check all other solid models
						CG_ClipMoveToEntities (start, mins, maxs, end, skipNumber, mask, &t);
						
						...
					}
					


3.	anytime we see the word "trap", we know that this is going into system call mode

					


					trap_CM_BoxTrace ( &t, start, end, mins, maxs, 0, mask);

					# a bunch of quake system call stuff
					...
					...				cg_syscall.c 	trap_CM_BoxTrace(){}
					...				vm.c 			VM_DllSyscall(){}									
					...				
					...


					cl_cgame.c

					int CL_CgameSystemCalls()
					{
						...
						...

						case CG_CM_BOXTRACE:
							CM_BoxTrace( VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], /*int capsule*/ qfalse );
							return 0;
						...
						...
					}




					cm_trace.c

					void CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
											  vec3_t mins, vec3_t maxs,
											  clipHandle_t model, int brushmask, int capsule ) {
						CM_Trace( results, start, end, mins, maxs, model, vec3_origin, brushmask, capsule, NULL );
					}



4.	and we finally arrived at the CM_Trace function, and you can see since we are shooting a ray,
	we will go through the CM_TraceThroughTree(){} function

					cm_trace.c

					void CM_Trace()
					{


						...
						...


						if ( model ) 
						{
							...
							...
						}
						else 
						{
							CM_TraceThroughTree( &tw, 0, 0, 1, tw.start, tw.end );
						}


						*results = tw.trace;
					}


5.	so you can see we are just traversing through the tree recursively (a bit like our kd tree). 
	and we are storing our results in traceWork_t.trace

					cm_trace.c

					void CM_TraceThroughTree( traceWork_t *tw, int num, float p1f, float p2f, vec3_t p1, vec3_t p2) 
					{
						
						...
						...


						// if < 0, we are in a leaf node
						// base case
						if (num < 0) 
						{
							CM_TraceThroughLeaf( tw, &cm.leafs[-1-num] );
							return;
						}

						...
						...


						node = cm.nodes + num;
						plane = node->plane;


						...
						...

						// see which sides we need to consider
						if ( t1 >= offset + 1 && t2 >= offset + 1 ) {
							CM_TraceThroughTree( tw, node->children[0], p1f, p2f, p1, p2 );
							return;
						}
						if ( t1 < -offset - 1 && t2 < -offset - 1 ) {
							CM_TraceThroughTree( tw, node->children[1], p1f, p2f, p1, p2 );
							return;
						}

						...
						...

						CM_TraceThroughTree( tw, node->children[side], p1f, midf, p1, mid );

						...
						...

						CM_TraceThroughTree( tw, node->children[side^1], midf, p2f, mid, p2 );
					}





6.					
					cm_trace.c
					
					void CM_TraceThroughLeaf( traceWork_t *tw, cLeaf_t *leaf ) 
					{


					}



##############################################################################
############################### PM_WalkMove(){} ##############################
##############################################################################




1.	so the PM_GroundTrace checks if you are mid air or intersecting a ground 

	the actually collision for your walking command is done in PM_WalkMove(){}



					PM_WalkMove()
					{

						...
						...

						// don't do anything if standing still
						if (!pm->ps->velocity[0] && !pm->ps->velocity[1]) {
							return;
						}

						PM_StepSlideMove( qfalse );
					}





2.	

					bg_slidemove.c

					void PM_StepSlideMove()
					{
						...
						...

						if ( PM_SlideMove( gravity ) == 0 ) {
							return;		// we got exactly where we wanted to go first try	
						}

						...
						...
					}



3.					

					void PM_SlideMove()
					{


						for ( bumpcount=0 ; bumpcount < numbumps ; bumpcount++ ) 
						{

							...

							// see if we can make it there
							pm->trace ( &trace, pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->clientNum, pm->tracemask);
						
							...
							...
						}
					}



4.		

					void CG_Trace()
					{

						...
						trap_CM_BoxTrace ( &t, start, end, mins, maxs, 0, mask);
						...
					}


5.					


					int CL_CgameSystemCalls( int *args ) 
					{

						...

						case CG_CM_BOXTRACE:
							CM_BoxTrace( VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], /*int capsule*/ qfalse );
							return 0;
				

						...
						...
					}




6.					


					void CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
											  vec3_t mins, vec3_t maxs,
											  clipHandle_t model, int brushmask, int capsule ) 
					{
						CM_Trace( results, start, end, mins, maxs, model, vec3_origin, brushmask, capsule, NULL );
					}





7.					
					cm_trace.c

					void CM_Trace()
					{

						...
						...


						CM_TraceThroughTree( &tw, 0, 0, 1, tw.start, tw.end );
					}





8.					see above





###################################################################################
################################# clipMap_t nodes #################################
###################################################################################

1.	one thing I want to point out in the previous function is the few lines below


					node = cm.nodes + num;
					plane = node->plane;

	you might wonder what is cm, turns out it is declared in cm_load.c

					cm_load.c

					clipMap_t cm



	and actually ultimately that leads to how maps and bsps are loaded in quake3. please see loading-map.c for more details







#############################################################################################
################################# CG_ClipMoveToEntities(){} #################################
#############################################################################################



6.	back to step2, we call CG_ClipMoveToEntities(){}

	Note that the Server calls SV_ClipMoveToEntities(){}


					cg_predict.c 			

					CG_ClipMoveToEntities()
					{

						for ( i = 0 ; i < cg_numSolidEntities ; i++ ) 
						{
							cent = cg_solidEntities[ i ];
							ent = &cent->currentState;

							...
							...


						}

					}

























##############################################################################################
######################################### Comparison #########################################
##############################################################################################


so if you have noticed, there is some comparison between the two.


				SV_Trace
				{
					...
					CM_BoxTrace( &clip.trace, start, end, mins, maxs, 0, contentmask, capsule );
					{
						CM_Trace()
						{

							...
							...

							CM_TraceThroughTree( &tw, 0, 0, 1, tw.start, tw.end );
						}
					}

					// clip to other solid entities
					SV_ClipMoveToEntities ( &clip );
					{
						num = SV_AreaEntities( clip->boxmins, clip->boxmaxs, touchlist, MAX_GENTITIES);
					
						for ( i=0 ; i<num ; i++ ) 
						{
							touch = SV_GentityNum( touchlist[i] );

						}
					}
				}





				CG_Trace
				{
					trap_CM_BoxTrace ( &t, start, end, mins, maxs, 0, mask)
					{

					}

					t.entityNum = t.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
					// check all other solid models
					CG_ClipMoveToEntities (start, mins, maxs, end, skipNumber, mask, &t);
					{
						for ( i = 0 ; i < cg_numSolidEntities ; i++ ) 
						{
							cent = cg_solidEntities[ i ];

							...
							...
						}
					}
				}


the main difference here is that the client in CG_ClipMoveToEntities(){}, we iterate through cg_solidEntities[i] whereas the server
gets everything from SV_AreaEntities(){}

the reason why is that CG_ClipMoveToEntities(){} does so is that when the server send a snapshot to the client, the server only sends the ones 
that are close to the client, so the server does not send everything he has to the client. The entities in the client snapshot is only a 
subset of the entire world. 

Then in CG_BuildSolidList(){}, we copy what we have from the snapshot and assigned it to cg_solidEntities[];



1.				

				CG_DrawActiveFrame()
				{
					...
					...

					// set up cg.snap and possibly cg.nextSnap
					CG_ProcessSnapshots();

					...
					...
				}	




2.	

				cg_snapshot.c

				CG_ProcessSnapshots()
				{
					do {
				
						...
						...

						CG_SetNextSnap( snap );

						...
						...

					} while ( 1 );


				}


3.	

				CG_SetNextSnap()
				{

					...
					...

					// sort out solid entities
					CG_BuildSolidList();
				}


4.	
				CG_BuildSolidList()
				{

					...
					...
					...

					for ( i = 0 ; i < snap->numEntities ; i++ ) 
					{
						cent = &cg_entities[ snap->entities[ i ].number ];
						ent = &cent->currentState;

						if ( ent->eType == ET_ITEM || ent->eType == ET_PUSH_TRIGGER || ent->eType == ET_TELEPORT_TRIGGER ) {
							cg_triggerEntities[cg_numTriggerEntities] = cent;
							cg_numTriggerEntities++;
							continue;
						}

						if ( cent->nextState.solid ) {
							cg_solidEntities[cg_numSolidEntities] = cent;
							cg_numSolidEntities++;
							continue;
						}
					}

				}













############################################################################################
#################################### Item Pickup ###########################################
############################################################################################




1.	

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




					g_active.c

					void ClientEvents()
					{



					}





2.					

					cg_predict.c


					void CG_PredictPlayerState( void ) 
					{


						...
						...


						// fire events and other transition triggered things
						CG_TransitionPlayerState( &cg.predictedPlayerState, &oldPlayerState );


						...
						...
					}


3.					


					cg_playerstate.c

					void CG_TransitionPlayerState()
					{

						// run events
						CG_CheckPlayerstateEvents( ps, ops );
					}










					cg_playerstate.c

					void CG_CheckPlayerstateEvents( playerState_t *ps, playerState_t *ops ) {

						...
						...

						cent = &cg.predictedPlayerEntity; // cg_entities[ ps->clientNum ];
						// go through the predictable events buffer
						for ( i = ps->eventSequence - MAX_PS_EVENTS ; i < ps->eventSequence ; i++ ) 
						{

							...
							...

							CG_EntityEvent( cent, cent->lerpOrigin );

						}
					}





2.
					void CG_EntityEvent( centity_t *cent, vec3_t position ) 
					{

						...
						...

						case EV_ITEM_PICKUP:
							DEBUGNAME("EV_ITEM_PICKUP");
							{
								...
								...
								...

								// show icon and name on status bar
								if ( es->number == cg.snap->ps.clientNum ) {
									CG_ItemPickup( index );
								}
							}
							break;


						...
						...

					}





3.					

					CG_ItemPickup()
					{


					}





then we run on the server

1.					
					

					g_items.c

					Pickup_Weapon()
					{

					}
























2.					
					tr_world.c

					static mnode_t* R_PointInLeaf()
					{

							node = tr.world->nodes;
					}





















					cm_test.c

					int CM_PointContents( const vec3_t p, clipHandle_t model ) 
					{

						...
						...

						if ( model ) 
						{
							clipm = CM_ClipHandleToModel( model );
							leaf = &clipm->leaf;
						} 
						else 
						{
							leafnum = CM_PointLeafnum_r (p, 0);
							leaf = &cm.leafs[leafnum];
						}

						...
						...
					}



5.				

					cm_trace.c

					CM_PositionTest()
					{



						cm.checkcount++;
					
						CM_BoxLeafnums_r( &ll, 0 );

						cm.checkcount++;

						// test the contents of the leafs
						for (i=0 ; i < ll.count ; i++) {
							CM_TestInLeaf( tw, &cm.leafs[leafs[i]] );
							if ( tw->trace.allsolid ) {
								break;
							}
						}

					}





6.					
					cm_trace.c

					CM_TestInLeaf()
					{

						...
						CM_TestBoxInBrush( tw, b );
					}






7.				

					cm_test.c
				
					CM_BoxLeafnums_r()
					{



					}















###########################################################################################################
############################################### PM_WalkMove ###############################################
###########################################################################################################




1.	so on the client side, to actually check for collision detection for the player whenever he travels on the ground, 
	that is actually done in the PM_SlideMove(){} function


					bg_pmove.c

					PM_WalkMove()
					{


					}



					/*
					==================
					PM_SlideMove

					Returns qtrue if the velocity was clipped in some way
					==================
					*/

					bg_slidemove.c

					qboolean PM_SlideMove( qboolean gravity ) 
					{


						for ( bumpcount=0 ; bumpcount < numbumps ; bumpcount++ ) {
						{

							// see if we can make it there
							pm->trace ( &trace, pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->clientNum, pm->tracemask);
						}
					}











2.					

					static void PM_GroundTrace( void ) {
						vec3_t		point;
						trace_t		trace;

						point[0] = pm->ps->origin[0];
						point[1] = pm->ps->origin[1];
						point[2] = pm->ps->origin[2] - 0.25;

						pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);

						...
						...
					}


3.
					
					cg_predict.c

					void CG_Trace()
					{

						...

						trap_CM_BoxTrace ( &t, start, end, mins, maxs, 0, mask);

						...

						// check all other solid models
						CG_ClipMoveToEntities (start, mins, maxs, end, skipNumber, mask, &t);

						*result = t;
					}





4.					

	calling trap_CM_BoxTrace(){} will eventually call CM_Trace()

					

					cm_trace.c

					void CM_Trace()
					{



					}


















5.					

					cg_predict.c

					static void CG_ClipMoveToEntities ( ) 
					{
						...
						...

						for ( i = 0 ; i < cg_numSolidEntities ; i++ ) 
						{
							cent = cg_solidEntities[ i ];


							if ( ent->solid == SOLID_BMODEL ) 
							{
								...
								...
							} 
							else 
							{
								// encoded bbox
								
								...
								...

								bmins[0] = bmins[1] = -x;
								bmaxs[0] = bmaxs[1] = x;
								bmins[2] = -zd;
								bmaxs[2] = zu;

								cmodel = trap_CM_TempBoxModel( bmins, bmaxs );
								VectorCopy( vec3_origin, angles );
								VectorCopy( cent->lerpOrigin, origin );
							}
						}







					}





5.					

					cl_cgame.c

					int CL_CgameSystemCalls( int *args ) 
					{

						switch( args[0] ) 
						{
							...
							...

							case CG_CM_TEMPBOXMODEL:
								return CM_TempBoxModel( VMA(1), VMA(2), /*int capsule*/ qfalse );

							...
							...
						}

						...
						...
					}





1.	

					q_shared.h


					#include "surfaceflags.h"			// shared with the q3map utility

					// plane types are used to speed some tests
					// 0-2 are axial planes
					#define	PLANE_X			0
					#define	PLANE_Y			1
					#define	PLANE_Z			2
					#define	PLANE_NON_AXIAL	3



					#define PlaneTypeForNormal(x) (x[0] == 1.0 ? PLANE_X : (x[1] == 1.0 ? PLANE_Y : (x[2] == 1.0 ? PLANE_Z : PLANE_NON_AXIAL) ) )

					// plane_t structure
					// !!! if this is changed, it must be changed in asm code too !!!
					typedef struct cplane_s {
						vec3_t	normal;
						float	dist;
						byte	type;			// for fast side tests: 0,1,2 = axial, 3 = nonaxial
						byte	signbits;		// signx + (signy<<1) + (signz<<2), used as lookup during collision
						byte	pad[2];
					} cplane_t;


					// a trace is returned when a box is swept through the world
					typedef struct {
						qboolean	allsolid;	// if true, plane is not valid
						qboolean	startsolid;	// if true, the initial point was in a solid area
						float		fraction;	// time completed, 1.0 = didn't hit anything
						vec3_t		endpos;		// final position
						cplane_t	plane;		// surface normal at impact, transformed to world space
						int			surfaceFlags;	// surface hit
						int			contents;	// contents on other side of surface hit
						int			entityNum;	// entity the contacted sirface is a part of
					} trace_t;











2.					
					tr_public.h

					typedef struct 
					{
						
						...
						...

						// visualization for debugging collision detection
						void	(*CM_DrawDebugSurface)( void (*drawPoly)(int color, int numPoints, float *points) );


						...
						...
					} refimport_t;



3.	
					g_public.h



					#define SVF_CAPSULE				0x00000400	// use capsule for collision detection instead of bbox
					#define SVF_NOTSINGLECLIENT		0x00000800	// send entity to everyone but one client
																// (entityShared_t->singleClient)





					typedef enum 
					{





						G_TRACE,	// ( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
									// collision detection against all linked entities

						G_POINT_CONTENTS,	// ( const vec3_t point, int passEntityNum );
						// point contents against all linked entities

						...
						...

						G_LINKENTITY,		// ( gentity_t *ent );
						// an entity will never be sent to a client or used for collision
						// if it is not passed to linkentity.  If the size, position, or
						// solidity changes, it must be relinked.

						...
						...

					} gameImport_t;










4.

					cm_trace.c

					// always use bbox vs. bbox collision and never capsule vs. bbox or vice versa
					//#define ALWAYS_BBOX_VS_BBOX
					// always use capsule vs. capsule collision and never capsule vs. bbox or vice versa
					//#define ALWAYS_CAPSULE_VS_CAPSULE












##############################################################################################
############################## Narrow Phase Collision Detection ##############################
##############################################################################################




1.					

					cm_trace.c

					void CM_TestBoundingBoxInCapsule( traceWork_t *tw, clipHandle_t model ) 
					{



					}















2.					

					cm_trace.c

					/*
					================
					CM_TraceThroughSphere

					get the first intersection of the ray with the sphere
					================
					*/
					void CM_TraceThroughSphere( traceWork_t *tw, vec3_t origin, float radius, vec3_t start, vec3_t end ) 
					{



					}





CM_PatchCollideFromGrid(){}

CM_GeneratePachCollide(){}

CM_TracePointThroughPatchCollide(){}

CM_TraceThroughPatchCollide(){}

CM_PositionTestInPatchCollide(){}

CM_ClipHandleToModel(){}

CM_TestInLeaf(){}

CM_TraceThroughPatchCollide(){}

CM_TraceThroughVerticalCylinder(){}

CM_TraceThroughSphere(){}






1.	Server 

					
					int SV_GameSystemCalls()
					{
						switch( args[0] )
						{
							...
							...
							case G_ENTITY_CONTACT:
								return SV_EntityContact( VMA(1), VMA(2), VMA(3), /*int capsule*/ qfalse );

							...
							...
						}
					}




2.			
					sv_game.c

					SV_EntityContact()
					{
						...
						...
						ch = SV_ClipHandleForEntity( gEnt );

						...
						...
					}





3.					

					sv_world.c

					void SV_AreaEntities_r()
					{

						for ( check = node->entities  ; check ; check = next ) {
							next = check->nextEntityInWorldSector;

							gcheck = SV_GEntityForSvEntity( check );

							...
							...
						}

					}





4.					
					sv_world.c











2.	


					g_public.h


					typedef struct {
						entityState_t	s;				// communicated by server to clients

						qboolean	linked;				// qfalse if not in any good cluster
						int			linkcount;

						int			svFlags;			// SVF_NOCLIENT, SVF_BROADCAST, etc

						// only send to this client when SVF_SINGLECLIENT is set	
						// if SVF_CLIENTMASK is set, use bitmask for clients to send to (maxclients must be <= 32, up to the mod to enforce this)
						int			singleClient;		

						qboolean	bmodel;				// if false, assume an explicit mins / maxs bounding box
														// only set by trap_SetBrushModel
						vec3_t		mins, maxs;
						int			contents;			// CONTENTS_TRIGGER, CONTENTS_SOLID, CONTENTS_BODY, etc
														// a non-solid entity should set to 0

						vec3_t		absmin, absmax;		// derived from mins/maxs and origin + rotation

						// currentOrigin will be used for all collision detection and world linking.
						// it will not necessarily be the same as the trajectory evaluation for the current
						// time, because each entity must be moved one at a time after time is advanced
						// to avoid simultanious collision issues
						vec3_t		currentOrigin;
						vec3_t		currentAngles;

						// when a trace call is made and passEntityNum != ENTITYNUM_NONE,
						// an ent will be excluded from testing if:
						// ent->s.number == passEntityNum	(don't interact with self)
						// ent->s.ownerNum = passEntityNum	(don't interact with your own missiles)
						// entity[ent->s.ownerNum].ownerNum = passEntityNum	(don't interact with other missiles from owner)
						int			ownerNum;
					} entityShared_t;




