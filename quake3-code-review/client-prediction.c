
We will talk about where prediction is done in this article

All the prediction related varaibles are actually stored in the rendering information,
so if you look inside cg_t 

					cg_local.h

					typedef struct {

						...
						...

						// prediction state
						qboolean	hyperspace;				// true if prediction has hit a trigger_teleport
						playerState_t	predictedPlayerState;
						centity_t		predictedPlayerEntity;
						qboolean	validPPS;				// clear until the first call to CG_PredictPlayerState
						int			predictedErrorTime;
						vec3_t		predictedError;

						int			eventSequence;
						int			predictableEvents[MAX_PREDICTED_EVENTS];

						...
						...

					} cg_t;


so you can imagine the prediction code is gonna touch a lot of these very often






1. so the the prediction are all completed in the SCR_UpdateScreen(){} function

					cl_scrn.c

					SCR_UpdateScreen()
					{

						SCR_DrawScreenField( STEREO_CENTER );
					}



					cl_scrn.c

					SCR_DrawScreenField()
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



2.	that goes on to make a function call to CG_DRAW_ACTIVE_FRAME

					cl_cgame.c

					void CL_CGameRendering( stereoFrame_t stereo ) 
					{
						VM_Call( cgvm, CG_DRAW_ACTIVE_FRAME, cl.serverTime, stereo, clc.demoplaying );
						VM_Debug( 0 );
					}


					#############################
					standard quake3 VM call precedure
					#############################


					vm.c

					int vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  ) {

						switch ( command ) {
						
						...
						...

						case CG_DRAW_ACTIVE_FRAME:
							CG_DrawActiveFrame( arg0, arg1, arg2 );
							return 0;
						
						...
						...

						default:
							CG_Error( "vmMain: unknown command %i", command );
							break;
						}
						return -1;
					}



3.	as we look inside CG_DrawActiveFrame(){}, we see the few main functions here 

					cg_view.c


					void CG_DrawActiveFrame( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback ) 
					{

						...
						...

						// set up cg.snap and possibly cg.nextSnap
						CG_ProcessSnapshots();

						// this counter will be bumped for every valid scene we generate
						cg.clientFrame++;

						// update cg.predictedPlayerState
						CG_PredictPlayerState();


						// build cg.refdef
						inwater = CG_CalcViewValues();


						CG_AddViewWeapon( &cg.predictedPlayerState );
					}





4.	finally we come to the step where we loop through our commands for client-side prediction
	for this part, please also refer to "Player Prediction Optimization" section from http://www.ra.is/unlagged/walkthroughs.html
	the link is very very very helpful



	so the sole purpose of this function is to popualte [cg.predictedPlayerState] with the correct values


	from the link it says 

			The server only makes one call to Pmove(){} per command, but the client can make up to 63 calls to Pmove(){} per command, 
			depending on latency.


	63? Let us take a look at what is going on

	-				oldPlayerState = cg.predictedPlayerState
		we first save the state before we the execute the pmove code

	-	we then retrieve the oldestCmd and the latestCmd. oldestCmd is just the current command going back CMD_BACKUP (which is 63 in this case)

	-	we then "assign cg.predictedPlayerState" to a player state from the most recent valid snapshot acknowledge from the server
		this is where the prediction will start

		either 
				cg.predictedPlayerState = cg.nextSnap->ps;
				cg.physicsTime = cg.nextSnap->serverTime;
		or
				cg.predictedPlayerState = cg.snap->ps;
				cg.physicsTime = cg.snap->serverTime;

		depending if we teleported or not




	-	then we go into the main loop 
		so we start looping from 63 commands ago. 

		in the main loop we call a virtual function to grab the command according to the cmdNum 
		and we assign it to cg_pmove, which is a static variable declared in cg_predict.c


					// get the command
					trap_GetUserCmd( cmdNum, &cg_pmove.cmd );


		also before the entire for loop, we have initalized some values for the cg_pmove variable

					// prepare for pmove
					cg_pmove.ps = &cg.predictedPlayerState;
					cg_pmove.trace = CG_Trace;
					cg_pmove.pointcontents = CG_PointContents;

		you can see that cg_pmove.ps is initalized with the address of cg.predictedPlayerState.
		so when the cg.predictedPlayerState was set to cg.snap->ps, cg_pmove.ps is also updated.




	-	for the if check

					if ( cg_pmove.cmd.serverTime <= cg.predictedPlayerState.commandTime ) {
						continue;
					}

		we do not do anything if the command is too old (before the the snapshot)


	-	for the if check

					// don't do anything if the command was from a previous map_restart
					if ( cg_pmove.cmd.serverTime > latestCmd.serverTime ) 
					{
						continue;
					}

		we skip the command if it from a previous map_restart



	-	also notice when we are processing every cmd, we will call Pmove, and we will pass in cg_pmove
		to it. This will be discussed in detail later


	-	so when we finally loop till we are present


					cg_predict.c 


					static	pmove_t		cg_pmove;	
									

					void CG_PredictPlayerState()
					{

						...
						...
						...


						// prepare for pmove
						cg_pmove.ps = &cg.predictedPlayerState;
						cg_pmove.trace = CG_Trace;
						cg_pmove.pointcontents = CG_PointContents;



						// save the state before the pmove so we can detect transitions
						oldPlayerState = cg.predictedPlayerState;

						current = trap_GetCurrentCmdNumber();




						// if we don't have the commands right after the snapshot, we
						// can't accurately predict a current position, so just freeze at
						// the last good position we had
						cmdNum = current - CMD_BACKUP + 1;
						trap_GetUserCmd( cmdNum, &oldestCmd );



						trap_GetUserCmd( current, &latestCmd );



						// get the most recent information we have, even if
						// the server time is beyond our current cg.time,
						// because predicted player positions are going to 
						// be ahead of everything else anyway
						if ( cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport ) 
						{
							cg.predictedPlayerState = cg.nextSnap->ps;
							cg.physicsTime = cg.nextSnap->serverTime;
						} 
						else 
						{
							cg.predictedPlayerState = cg.snap->ps;
							cg.physicsTime = cg.snap->serverTime;
						}





						for ( cmdNum = current - CMD_BACKUP + 1 ; cmdNum <= current ; cmdNum++ ) 
						{

							// get the command
							trap_GetUserCmd( cmdNum, &cg_pmove.cmd );


							if ( cg_pmove.pmove_fixed ) 
							{
								PM_UpdateViewAngles( cg_pmove.ps, &cg_pmove.cmd );
							}

							// don't do anything if the time is before the snapshot player time
							if ( cg_pmove.cmd.serverTime <= cg.predictedPlayerState.commandTime ) {
								continue;
							}

							// don't do anything if the command was from a previous map_restart
							if ( cg_pmove.cmd.serverTime > latestCmd.serverTime ) 
							{
								continue;
							}


							if ( cg.predictedPlayerState.commandTime == oldPlayerState.commandTime ) 
							{

								...
								...




								Pmove (&cg_pmove);

								// add push trigger movement effects
								CG_TouchTriggerPrediction();

							}

						}


					}









#######################################################################################
####################################### Pmove #########################################
#######################################################################################

so the prediction calls Pmove which will do a bunch of things including collision detection.
i am not gonna talk about the details of collision detection done here, for that, please refer to collision-detection.c


1.	one important thing in pmove_t is the argument I passed into Pmove(){}. If you look at pmove_t struct

					

					typedef struct {
						// state (in / out)
						playerState_t	*ps;

						// command (in)
						usercmd_t	cmd;
						
						...
						...

					} pmove_t;


	it keeps a record of the playerState and cmd. These will be used heavily



					bg_move.c

					void Pmove(pmove_t *pmove)
					{

						...
						...

						// chop the move up if it is too long, to prevent framerate
						// dependent behavior
						while ( pmove->ps->commandTime != finalTime ) 
						{
							...
							...

							pmove->cmd.serverTime = pmove->ps->commandTime + msec;
							PmoveSingle( pmove );

							...
						}

					}



2.	if we look inside PmoveSingle, we can see some of the things that are done

	-	we will discuss PM_UpdateViewAngles(){} and AngleVectors(){} later

					bg_pmove.c

					viod PmoveSingle(pmove_t* pmove)
					{
						pm = pmove;
						...
						...

						// update the viewangles
						PM_UpdateViewAngles( pm->ps, &pm->cmd );
				
						AngleVectors (pm->ps->viewangles, pml.forward, pml.right, pml.up);

						...
						...


					}


3.	this is where the angles get updated on the client side. if you comment this out, the user will get stuck viewing in one direction
	meaning no matter how violently you shake your mouse, nothing will happen	


					bg_pmove.c

					void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd ) 
					{

						...
						...

						// circularly clamp the angles with deltas
						for (i=0 ; i<3 ; i++) 
						{
							temp = cmd->angles[i] + ps->delta_angles[i];
							if ( i == PITCH ) 
							{
								// don't let the player look up or down more than 90 degrees
								if ( temp > 16000 ) 
								{
									ps->delta_angles[i] = 16000 - cmd->angles[i];
									temp = 16000;
								} 
								else if ( temp < -16000 ) 
								{
									ps->delta_angles[i] = -16000 - cmd->angles[i];
									temp = -16000;
								}
							}
							ps->viewangles[i] = SHORT2ANGLE(temp);
						}
					}




4.	if you look at what AngleVectors is doing, u can see we are constructing the three axis vectors

					q_math.c 

					void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up) 
					{
						float		angle;
						static float		sr, sp, sy, cr, cp, cy;
						// static to help MS compiler fp bugs

						angle = angles[YAW] * (M_PI*2 / 360);
						sy = sin(angle);
						cy = cos(angle);
						angle = angles[PITCH] * (M_PI*2 / 360);
						sp = sin(angle);
						cp = cos(angle);
						angle = angles[ROLL] * (M_PI*2 / 360);
						sr = sin(angle);
						cr = cos(angle);

						if (forward)
						{
							forward[0] = cp*cy;
							forward[1] = cp*sy;
							forward[2] = -sp;
						}
						if (right)
						{
							right[0] = (-1*sr*sp*cy+-1*cr*-sy);
							right[1] = (-1*sr*sp*sy+-1*cr*cy);
							right[2] = -1*sr*cp;
						}
						if (up)
						{
							up[0] = (cr*sp*cy+-sr*-sy);
							up[1] = (cr*sp*sy+-sr*cy);
							up[2] = cr*cp;
						}
					}




#######################################################################################
############################### CG_TouchTriggerPrediction() ###########################
#######################################################################################


1.	so after finishing Pmove(){}, you can see that we are calling CG_TouchTriggerPrediction(){}

					cg_predict.c

					void CG_TouchTriggerPrediction()
					{

						for ( i = 0 ; i < cg_numTriggerEntities ; i++ ) {
							cent = cg_triggerEntities[ i ];
							ent = &cent->currentState;

							if ( ent->eType == ET_ITEM && !spectator ) {
								CG_TouchItem( cent );
								continue;
							}

							...
							...

							trap_CM_BoxTrace( &trace, cg.predictedPlayerState.origin, cg.predictedPlayerState.origin, 
								cg_pmove.mins, cg_pmove.maxs, cmodel, -1 );

							...
							...

						}
					}




2.	here it goes through all the ccg_triggerEntities(){}; and process them
	take the example of CG_TouchItem(){}; one thing I want to point out in here is that this function calls 
	BG_AddPredictableEventToPlayerstate(){}, and inside the collision detection between player and the ItemPickUp 
	is using cg.predictedPlayerState. which goes to show you that on the client side, everything is done with cg.predictedPlayerState

					cg_predict.c

					void CG_TouchItem()
					{
						...
						...

						// grab it
						BG_AddPredictableEventToPlayerstate( EV_ITEM_PICKUP, cent->currentState.modelindex , &cg.predictedPlayerState);

						...
						...
					}




2.	

					bg_misc.c

					void BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps ) 
					{
						ps->events[ps->eventSequence & (MAX_PS_EVENTS-1)] = newEvent;
						ps->eventParms[ps->eventSequence & (MAX_PS_EVENTS-1)] = eventParm;
						ps->eventSequence++;
					}







#####################################################################################################
####################################### CG_CalcViewValues() #########################################
#####################################################################################################

1.	after completing CG_PredictPlayerState(){}, we run CG_CalcViewValues(){}				
	
	-	you can see that we are transfering lots of information from playerState to cg.refdef, which is the graphics pipeline


	-	here you can see that we are ofcourse doing CG_OffsetFirstPersonView(){}

					cg_view.c	

					CG_CalcViewValues()
					{
						...
						ps = &cg.predictedPlayerState;

						...
						...

						VectorCopy( ps->origin, cg.refdef.vieworg );
						VectorCopy( ps->viewangles, cg.refdefViewAngles );



						if ( cg.renderingThirdPerson ) 
						{
							// back away from character
							CG_OffsetThirdPersonView();
						} else {
							// offset for local bobbing and kicks
							CG_OffsetFirstPersonView();
						}


						// position eye reletive to origin
						AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );

						...
						...

					}





					static void CG_OffsetFirstPersonView( void ) 
					{



					}











#####################################################################################################
####################################### CG_AddViewWeapon() ##########################################
#####################################################################################################

					cg_weapons.c


					CG_AddViewWeapon( &cg.predictedPlayerState )
					{

						...
						CG_CalculateWeaponPosition( hand.origin, angles );

						...
						...


						// add everything onto the hand
						CG_AddPlayerWeapon( &hand, ps, &cg.predictedPlayerEntity, ps->persistant[PERS_TEAM] );
					}














#################################################################################################
################################# Prediction Error ##############################################
#################################################################################################



Prediction Error: when the prediction is wrong. It must be correct for, which introduces some of the most severe
visual problems when it happens on the client - depending on how far ahead the client was predicting. 


At this point, the client will replace the predicted snapshot by the actual snapshot and recalculate the interpolation. 
This difference between the two is the prediction error. 
However at this point, you will not see a huge jerk due to the change from predicted to actual 
data as the prediction error is decayed away to ensure the gameplay remains as smooth as possible,
even if it is slightly inaccurate.


read this and look for the term "prediction error". It will direct you to the section CL_TIMENUDEG
that will mention what does decay mean over there
http://www.mikemartin.co/gaming_guides/quake3_smoothness_guide#TOC-CG_PREDICT





					cg_predict.c

					void CG_InterpolatePlayerState()
					{





						// if we are still allowing local input, short circuit the view angles
						if ( grabAngles ) {
							usercmd_t	cmd;
							int			cmdNum;

							cmdNum = trap_GetCurrentCmdNumber();
							trap_GetUserCmd( cmdNum, &cmd );

							PM_UpdateViewAngles( out, &cmd );
						}



					}











#######################################################################################################
################################## Stack Traces from trap commands ####################################
#######################################################################################################

I have included stack traces for some of the "quake function calls " that are used in client-prediction

1.	trap_GetUserCmd stack trace: 

					cg_syscalls.c

					qboolean	trap_GetUserCmd( int cmdNumber, usercmd_t *ucmd ) 
					{
						return syscall( CG_GETUSERCMD, cmdNumber, ucmd );
					}



					vm.c

					int QDECL VM_DllSyscall( int arg, ... ) 
					{
						...
						...

						return currentVM->systemCall( &arg );
					}



					cl_cgame.c

					int CL_CgameSystemCalls( int *args ) 
					{

						...
						...

						case CG_GETUSERCMD:
							return CL_GetUserCmd( args[1], VMA(2) );
				

					}




					vm.c

					void *VM_ArgPtr( int intValue ) {
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




					cl_cgame.c

					qboolean CL_GetUserCmd()
					{

						// can't return anything that we haven't created yet
						if ( cmdNumber > cl.cmdNumber ) {
							Com_Error( ERR_DROP, "CL_GetUserCmd: %i >= %i", cmdNumber, cl.cmdNumber );
						}

						// the usercmd has been overwritten in the wrapping
						// buffer because it is too far out of date
						if ( cmdNumber <= cl.cmdNumber - CMD_BACKUP ) 
						{
							return qfalse;
						}

						*ucmd = cl.cmds[ cmdNumber & CMD_MASK ];

						return qtrue;
					}



2.	trap_GetCurrentCmdNumber stack trace: 

					cg_syscalls.c

					int			trap_GetCurrentCmdNumber( void ) {
						return syscall( CG_GETCURRENTCMDNUMBER );
					}


					...
					...
					...


					cl_cgame.c

					int CL_CgameSystemCalls( int *args ) 
					{
						...
						...

						case CG_GETCURRENTCMDNUMBER:
							return CL_GetCurrentCmdNumber();

					}



					int CL_GetCurrentCmdNumber( void ) 
					{
						return cl.cmdNumber;
					}
