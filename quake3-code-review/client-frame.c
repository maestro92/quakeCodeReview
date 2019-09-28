
Client Frame





					void CL_Frame ( int msec ) 
					{

						if ( !com_cl_running->integer ) {
							return;
						}

						if ( cls.cddialog ) {
							// bring up the cd error dialog if needed
							cls.cddialog = qfalse;
							VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_NEED_CD );
						} else	if ( cls.state == CA_DISCONNECTED && !( cls.keyCatchers & KEYCATCH_UI )
							&& !com_sv_running->integer ) {
							// if disconnected, bring up the menu
							S_StopAllSounds();
							VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN );
						}

						// if recording an avi, lock to a fixed fps
						if ( cl_avidemo->integer && msec) {
							// save the current screen
							if ( cls.state == CA_ACTIVE || cl_forceavidemo->integer) {
								Cbuf_ExecuteText( EXEC_NOW, "screenshot silent\n" );
							}
							// fixed time for next frame'
							msec = (1000 / cl_avidemo->integer) * com_timescale->value;
							if (msec == 0) {
								msec = 1;
							}
						}
						
						printf("	CL_Frame %d\n", cls.framecount);

						// save the msec before checking pause
						cls.realFrametime = msec;

						// decide the simulation time
						cls.frametime = msec;

						cls.realtime += cls.frametime;

						if ( cl_timegraph->integer ) {
							SCR_DebugGraph ( cls.realFrametime * 0.25, 0 );
						}

						printf("		cls.frametime %d, and cls.realtime %d\n", cls.frametime, cls.realtime);


						// see if we need to update any userinfo
						CL_CheckUserinfo();

						// if we haven't gotten a packet in a long time,
						// drop the connection
						CL_CheckTimeout();

						// send intentions now
						CL_SendCmd();

						// resend a connection request if necessary
						CL_CheckForResend();

						// decide on the serverTime to render
						CL_SetCGameTime();

						// update the screen
						SCR_UpdateScreen();

						// update audio
						S_Update();

						// advance local effects for next frame
						SCR_RunCinematic();

						Con_RunConsole();

						cls.framecount++;
					}









					