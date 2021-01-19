

				sys_win.c

				int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
				{

					...
					...

					Qcommon_Frame (time);
					...
				}



2.	


				void Qcommon_Frame (int msec)
				{
					...
					...

					CL_Frame (msec);

					...
					...
				}



3.			
				void CL_Frame (int msec)
				{

					// fetch results from server
					CL_ReadPackets ();

					// send a new command message to the server
					CL_SendCommand ();

					// predict all unacknowledged movements
					CL_PredictMovement ();
				
					// update the screen
	----------->	SCR_UpdateScreen ();

				}


4.	
				void SCR_UpdateScreen (void)
				{
					...
					...

					V_RenderView ( separation[i] );

					...
					...
				}




5.	

				void V_RenderView( float stereo_separation )
				{
					...
					...
					re.RenderFrame (&cl.refdef);
					...
					...
				}



6.		
				gl_rmain.c

				/*
				@@@@@@@@@@@@@@@@@@@@@
				R_RenderFrame

				@@@@@@@@@@@@@@@@@@@@@
				*/
				void R_RenderFrame (refdef_t *fd)
				{
					R_RenderView( fd );
					R_SetLightLevel ();
					R_SetGL2D ();
				}



7.	here we are drawing the entities and the world 
	we see two big functions:


				R_DrawWorld ();
				R_DrawEntitiesOnList ();

	-	functiosn below:

				/*
				================
				R_RenderView

				r_newrefdef must be set before the first call
				================
				*/
				void R_RenderView (refdef_t *fd)
				{


					R_DrawWorld ();

					R_DrawEntitiesOnList ();

					...
					...
				}




####################################################################
###################### R_DrawEntitiesOnList ########################
####################################################################


8.	

				D:\Projects\quake2-vs2015-master\ref_gl\gl_rmain.c


				/*
				=============
				R_DrawEntitiesOnList
				=============
				*/
				void R_DrawEntitiesOnList (void)
				{
					int		i;

					if (!r_drawentities->value)
						return;

					// draw non-transparent first
					for (i=0 ; i<r_newrefdef.num_entities ; i++)
					{
						currententity = &r_newrefdef.entities[i];

						currentmodel = currententity->model;
						if (!currentmodel)
						{
							R_DrawNullModel ();
							continue;
						}
						switch (currentmodel->type)
						{
						case mod_alias:
							R_DrawAliasModel (currententity);
							break;
						case mod_brush:
							R_DrawBrushModel (currententity);
							break;
						case mod_sprite:
							R_DrawSpriteModel (currententity);
							break;
						default:
							ri.Sys_Error (ERR_DROP, "Bad modeltype");
							break;
						}
						
					}

					// draw transparent entities
					// we could sort these if it ever becomes a problem...
					qglDepthMask (0);		// no z writes
					for (i=0 ; i<r_newrefdef.num_entities ; i++)
					{
						currententity = &r_newrefdef.entities[i];
						if (!(currententity->flags & RF_TRANSLUCENT))
							continue;	// solid

						if ( currententity->flags & RF_BEAM )
						{
							R_DrawBeam( currententity );
						}
						else
						{
							currentmodel = currententity->model;

							if (!currentmodel)
							{
								R_DrawNullModel ();
								continue;
							}
							switch (currentmodel->type)
							{
							case mod_alias:
								R_DrawAliasModel (currententity);
								break;
							case mod_brush:
								R_DrawBrushModel (currententity);
								break;
							case mod_sprite:
								R_DrawSpriteModel (currententity);
								break;
							default:
								ri.Sys_Error (ERR_DROP, "Bad modeltype");
								break;
							}
						}
					}
					qglDepthMask (1);		// back to writing

				}
