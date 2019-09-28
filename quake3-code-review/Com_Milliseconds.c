

Two places Com_Milliseconds() get used

1.



					void S_AddLoopSounds (void) 
					{

						time = Com_Milliseconds();	


					}


					int Com_Milliseconds (void) 
					{



					}





2.					

					cl_main.c

					CL_Frame()
					{

						...

						// update audio
						S_Update();
				
						...
						...
					}







3.	S_Update() stands for sound update. 

					snd_dma.c	


					void S_Update( void ) 
					{




						// mix some sound
						S_Update_();
					}




4. 

					snd_dma.c	

					S_Update_()
					{
						...
						...

						thisTime = Com_Milliseconds();

						...
						...
					}





5.	
					
					common.c			

					int Com_Milliseconds (void) {
						sysEvent_t	ev;

						printf("Com_Milliseconds\n");

						// get events and push them until we get a null event with the current time
						do {

							ev = Com_GetRealEvent();
							if ( ev.evType != SE_NONE ) {
								Com_PushEvent( &ev );
							}
						} while ( ev.evType != SE_NONE );
						
						return ev.evTime;
					}

