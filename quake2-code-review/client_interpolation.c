1.	recall in the client frame

			quake2/cl_main.c


			CL_Frame()
			{

				...
				...

				// deals with the mouse
				IN_Frame ();


				// fetch results from server
				CL_ReadPackets ();


				// send a new command message to the server
				CL_SendCommand ();


				// predict all unacknowledged movements
				CL_PredictMovement ();


				// allow rendering DLL change
				VID_CheckChanges ();
				if (!cl.refresh_prepped && cls.state == ca_active)
					CL_PrepRefresh ();


				SCR_UpdateScreen ();

				...
				...

			}



9.	we look into the SRC_UpdateScreen() function in quake2/cl_scrn.c


		// Structure containing functions exported from refresh DLL
		refexport_t	re;	

				quake2/cl_scrn.c

				SCR_UpdateScreen()
				{

					...
					...


					for ( i = 0; i < numframes; i++ )
					{
						re.BeginFrame( separation[i] );

						if (scr_draw_loading == 2)
						{	//  loading plaque over black screen
							int		w, h;

							re.CinematicSetPalette(NULL);
							scr_draw_loading = false;
							re.DrawGetPicSize (&w, &h, "loading");
							re.DrawPic ((viddef.width-w)/2, (viddef.height-h)/2, "loading");
				//			re.EndFrame();
				//			return;
						} 
						// if a cinematic is supposed to be running, handle menus
						// and console specially
						else if (cl.cinematictime > 0)
						{
							if (cls.key_dest == key_menu)
							{
								if (cl.cinematicpalette_active)
								{
									re.CinematicSetPalette(NULL);
									cl.cinematicpalette_active = false;
								}
								M_Draw ();
				//				re.EndFrame();
				//				return;
							}
							else if (cls.key_dest == key_console)
							{
								if (cl.cinematicpalette_active)
								{
									re.CinematicSetPalette(NULL);
									cl.cinematicpalette_active = false;
								}
								SCR_DrawConsole ();
				//				re.EndFrame();
				//				return;
							}
							else
							{
								SCR_DrawCinematic();
				//				re.EndFrame();
				//				return;
							}
						}
						else 
						{

							// make sure the game palette is active
							if (cl.cinematicpalette_active)
							{
								re.CinematicSetPalette(NULL);
								cl.cinematicpalette_active = false;
							}

							// do 3D refresh drawing, and then update the screen
							SCR_CalcVrect ();

							// clear any dirty part of the background
							SCR_TileClear ();

							V_RenderView ( separation[i] );

							SCR_DrawStats ();
							if (cl.frame.playerstate.stats[STAT_LAYOUTS] & 1)
								SCR_DrawLayout ();
							if (cl.frame.playerstate.stats[STAT_LAYOUTS] & 2)
								CL_DrawInventory ();

							SCR_DrawNet ();
							SCR_CheckDrawCenterString ();

							if (scr_timegraph->value)
								SCR_DebugGraph (cls.frametime*300, 0);

							if (scr_debuggraph->value || scr_timegraph->value || scr_netgraph->value)
								SCR_DrawDebugGraph ();

							SCR_DrawPause ();

							SCR_DrawConsole ();

							M_Draw ();

							SCR_DrawLoading ();
						}
					}
					re.EndFrame();
				}





10.	
		
				V_RenderView()
				{


				}




11.	we look into rendering part,
		
				ref_gl/gl_rmain.c

				R_BeginFrame()
				{

					R_RenderView( fd );
					R_SetLightLevel ();
					R_SetGL2D ();

				}




12.	

				/*
				================
				R_RenderView

				r_newrefdef must be set before the first call
				================
				*/
				void R_RenderView (refdef_t *fd)
				{

					...
					...
					
					R_PushDlights ();

					if (gl_finish->value)
						qglFinish ();

					R_SetupFrame ();

					R_SetFrustum ();

					R_SetupGL ();

					R_MarkLeaves ();	// done here so we know if we're in water

					R_DrawWorld ();

					R_DrawEntitiesOnList ();

					R_RenderDlights ();

					R_DrawParticles ();

					R_DrawAlphaSurfaces ();

					R_Flash();

					if (r_speeds->value)
					{
						ri.Con_Printf (PRINT_ALL, "%4i wpoly %4i epoly %i tex %i lmaps\n",
							c_brush_polys, 
							c_alias_polys, 
							c_visible_textures, 
							c_visible_lightmaps); 
					}
				}








				