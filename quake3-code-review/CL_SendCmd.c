





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








2.	the client generates a command for this frame

					cl_input.c

					usercmd_t CL_CreateNewCommands()
					{

						...
						...

						// generate a command for this frame
						cl.cmdNumber++;
						cmdNum = cl.cmdNumber & CMD_MASK;
						cl.cmds[cmdNum] = CL_CreateCmd ();
						cmd = &cl.cmds[cmdNum];
					}

					



######################################################################
########################### CL_CreateCmd() ###########################
######################################################################

1.	Now we look at how we create actual creat the command

	-	the middle chunk with a bunch of if statement is just prevent wrap around [0 ~ 360] or [-90 ~ 90] depending how u set your ranges
		if u we recall, we do the same in our games 

	everything else should be pretty straight forward

					cl_input.c

					usercmd_t CL_CreateCmd()
					{

						...
						...

						CL_CmdButtons( &cmd );

						// get basic movement from keyboard
						CL_KeyMove( &cmd );


						CL_MouseMove( &cmd );


						...

						// check to make sure the angles haven't wrapped
						if ( cl.viewangles[PITCH] - oldAngles[PITCH] > 90 ) {
							cl.viewangles[PITCH] = oldAngles[PITCH] + 90;
						} else if ( oldAngles[PITCH] - cl.viewangles[PITCH] > 90 ) {
							cl.viewangles[PITCH] = oldAngles[PITCH] - 90;
						} 

						// store out the final values
						CL_FinishMove( &cmd );


						...
						...
					}


2.	one thing I want to point out in CL_CmdButtons is that there is for loop for setting flags of the cmd->buttons
	using the |= operator. Obviously on the server side, the server will also check these bit flags when they parse the commands

	lots of the buttons are listed in q_shared.h. So if you try to do a global search on "BUTTON_ATTACK",  you will not actually find 
	anything occurence of the client code explicitly doing a "cmd->buttons != BUTTON_ATTACK" since it is handled in this 
	for loop. I certainly wasted so much doing so


					#define	BUTTON_ATTACK		1
					#define	BUTTON_TALK			2			// displays talk balloon and disables actions
					#define	BUTTON_USE_HOLDABLE	4
					#define	BUTTON_GESTURE		8
					#define	BUTTON_WALKING		16			// walking can't just be infered from MOVE_RUN
															// because a key pressed late in the frame will
															// only generate a small move value for that frame
															// walking will use different animations and
															// won't generate footsteps
					#define BUTTON_AFFIRMATIVE	32
					#define	BUTTON_NEGATIVE		64

					#define BUTTON_GETFLAG		128
					#define BUTTON_GUARDBASE	256
					#define BUTTON_PATROL		512
					#define BUTTON_FOLLOWME		1024

					#define	BUTTON_ANY			2048			// any key whatsoever

					#define	MOVE_RUN			120			// if forwardmove or rightmove are >= MOVE_RUN,


	now the real code 

					cl_input.c

					void CL_CmdButtons( usercmd_t *cmd )
					{
						int		i;

						//
						// figure button bits
						// send a button bit even if the key was pressed and released in
						// less than a frame
						//	
						for (i = 0 ; i < 15 ; i++) 
						{
							if ( in_buttons[i].active || in_buttons[i].wasPressed ) 
							{
								cmd->buttons |= 1 << i;
							}
							in_buttons[i].wasPressed = qfalse;
						}


						if ( cls.keyCatchers ) {
							cmd->buttons |= BUTTON_TALK;
						}

						// allow the game to know if any key at all is
						// currently pressed, even if it isn't bound to anything
						if ( anykeydown && !cls.keyCatchers ) {
							cmd->buttons |= BUTTON_ANY;
						}

					}



3.	another thing in CL_MouseMove(){} is that we see a m_filter, as well as cl.mouseDx[0], cl.mouseDx[1]

					cl_input.c

					CL_MouseMove( usercmd_t *cmd ) {
					{
						...
						...

						// allow mouse smoothing
						if ( m_filter->integer ) 
						{		
							mx = ( cl.mouseDx[0] + cl.mouseDx[1] ) * 0.5;
							my = ( cl.mouseDy[0] + cl.mouseDy[1] ) * 0.5;
						} 
						else 
						{
							mx = cl.mouseDx[cl.mouseIndex];
							my = cl.mouseDy[cl.mouseIndex];
						}

					}


	and if you check the declaration in clientActive_t, u start to wonder why do we have two here. Also if you refer to the code
	above, it seem slike the keeping two copies of mouseDx, mouseDy seems to only work in the case of m_filter.

					typedef struct {

						...

						int			mouseDx[2], mouseDy[2];	// added to by mouse events
						int			mouseIndex;

						...

					} clientActive_t;

					extern	clientActive_t		cl;



	Interestingly, if you do a search on the variable "m_filter", you will see in CL_Init(){}, we initalize the parameter 
	of m_filter. And the comments explains it all as to why we have this filter flag.


					cl_main.c

					void CL_Init( void ) {

						...
						...

						#ifdef MACOS_X
					        // Input is jittery on OS X w/o this
							m_filter = Cvar_Get ("m_filter", "1", CVAR_ARCHIVE);
						#else
							m_filter = Cvar_Get ("m_filter", "0", CVAR_ARCHIVE);

						...
						...
					}




4.	one thing I want to mention in CL_FinishMove(){} is that it sends the weapon index in the cmd as well.
	the usercmd_t has a member variable "cmd->weapon", which is the index of the weapons

	you can refer to the image in this link

		http://the-wipeout.de/wp-content/uploads/2012/09/weapons.gif

	the weapons index starts off 1, so if you print the values, you will see that the rocket launcher has cmd value 5, 
	the plasma gun hascmd value 8, etc, etc. (for some reason the BFG is left out)


					cl_input.c 

					void CL_FinishMove()
					{
						// copy the state that the cgame is currently sending
						cmd->weapon = cl.cgameUserCmdValue;

						// send the current server time so the amount of movement
						// can be determined without allowing cheating
						cmd->serverTime = cl.serverTime;

						for (i=0 ; i<3 ; i++) {
							cmd->angles[i] = ANGLE2SHORT(cl.viewangles[i]);
						}
					}











########################################################################################################################
############################################### UserCmd PACKET STRUCTURE ###############################################
########################################################################################################################


					CL_WritePacket()
					{



						CL_Netchan_Transmit (&clc.netchan, &buf);	
						{

							Netchan_Transmit( chan, msg->cursize, msg->data )
							{

								// write the packet header
								MSG_InitOOB (&send, send_buf, sizeof(send_buf));

								MSG_WriteLong( &send, chan->outgoingSequence );
								chan->outgoingSequence++;





								MSG_Init( &buf, data, sizeof(data) );

								MSG_Bitstream( &buf );
								// write the current serverId so the server
								// can tell if this is from the current gameState
								MSG_WriteLong( &buf, cl.serverId );

								// write the last message we received, which can
								// be used for delta compression, and is also used
								// to tell if we dropped a gamestate
								MSG_WriteLong( &buf, clc.serverMessageSequence );



								MSG_WriteByte (&buf, clc_move);

								MSG_WriteByte( &buf, count );

								MSG_WriteDeltaUsercmdKey (&buf, key, oldcmd, cmd);


							}


						}


					}



