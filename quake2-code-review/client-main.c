		

so the client has its own class to represent entities, obviously it is more geared towards the rendering side .
I am assuming centity means client entity

					client.h

					typedef struct
					{
						entity_state_t	baseline;		// delta from this if not from a previous frame
						entity_state_t	current;
						entity_state_t	prev;			// will always be valid, but might just be a copy of current

						int			serverframe;		// if not current, this ent isn't in the frame

						int			trailcount;			// for diminishing grenade trails
						vec3_t		lerp_origin;		// for trails (variable hz)

						int			fly_stoptime;
					} centity_t;






1.	main client code, you can see that the client part of Quake2 predicts movement inbetween network updates from the server. 
	Eventually, the engine updates the client with correct info.

			void CL_Frame (int msec)
			{



				// fetch results from server
				CL_ReadPackets ();

				// send a new command message to the server
				CL_SendCommand ();

				// predict all unacknowledged movements
				CL_PredictMovement ();

				...

				...
				SCR_UpdateScreen ();
				...

			}



2.			






The files involved in player movement and action are: p_client.c,  p_hud.c, p_trail.c, p_view.c, p_weapon.c and g_items.c




9.	we look into the SRC_UpdateScreen() function in quake2/cl_scrn.c. The most important part is the V_RenderView(){} function which will
	get into later


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
				{	
					//  loading plaque over black screen
					...
					...
				} 
				// if a cinematic is supposed to be running, handle menus
				// and console specially
				else if (cl.cinematictime > 0)
				{
					...
					...
				}
				else 
				{


					...
					...

					V_RenderView ( separation[i] );

					...
					...
				}
			}
			re.EndFrame();
		}





10.	So lets start looking at this function
		
			cl_view.c

			V_RenderView()
			{




				if ( cl.frame.valid && (cl.force_refdef || !cl_paused->value) )
				{

					// build a refresh entity list and calc cl.sim*
					// this also calls CL_CalcViewValues which loads
					// v_forward, etc.
					CL_AddEntities ();

				}

			}






11.	in the Client AddEntities Function we first calculate the interpolation factor cl.lerpfrac based on the time

	-	CL_CalcViewValues(){} does interpolation on the player itself


	-	For CL_AddPacketEntities(){} the of the function may be misleading 

				cl_ents.c

				CL_AddEntities()
				{

					if (cls.state != ca_active)
						return;

					if (cl.time > cl.frame.servertime)
					{
						if (cl_showclamp->value)
							Com_Printf ("high clamp %i\n", cl.time - cl.frame.servertime);
						cl.time = cl.frame.servertime;
						cl.lerpfrac = 1.0;
					}
					else if (cl.time < cl.frame.servertime - 100)
					{
						if (cl_showclamp->value)
							Com_Printf ("low clamp %i\n", cl.frame.servertime-100 - cl.time);
						cl.time = cl.frame.servertime - 100;
						cl.lerpfrac = 0;
					}
					else
						cl.lerpfrac = 1.0 - (cl.frame.servertime - cl.time) * 0.01;

					if (cl_timedemo->value)
						cl.lerpfrac = 1.0;


					CL_CalcViewValues();

					CL_AddPacketEntities(&cl.frame);


					...
					...

				}




13.	now lets look at what CL_AddPacketEntities(){} does. You can see that it is going through cl_parse_entities and cl_entites 
	and interpolating every one of them. Recall that on the client side, the client does not access entities through g_edicts,
	instead, it does so through cl_entites.


	-	the actual interpolation of position happens in the line 								


				ent.origin[i] = ent.oldorigin[i] = cent->prev.origin[i] + cl.lerpfrac * 
								(cent->current.origin[i] - cent->prev.origin[i]);

		if you are curious, you can try to replace that line with 

				ent.origin[i] = ent.oldorigin[i] = (cent->current.origin[i]);
		(which is what I commented out), and try to run the game and see how are entities rendered.



	-	similarly, the actual interpolation of orientation of entities happens in the line 


				for (i=0 ; i<3 ; i++)
				{
					a1 = cent->current.angles[i];
					a2 = cent->prev.angles[i];
					ent.angles[i] = LerpAngle (a2, a1, cl.lerpfrac);
					// ent.angles[i] = cent->current.angles[i];
				}

		and once again, you can test the differences by replacing the LerpAngle line with the commented line
		the visual differences is actually quite obvious, so we definitely want lerping


	-	towards the end you can see we actually call V_AddEntity(){}. I will get to that in the next step


				cl_ents.c

				CL_AddPacketEntities()
				{



					for (pnum = 0 ; pnum<frame->num_entities ; pnum++)
					{
						s1 = &cl_parse_entities[(frame->parse_entities+pnum)&(MAX_PARSE_ENTITIES-1)];

						cent = &cl_entities[s1->number];


						...
						...


						ent.oldframe = cent->prev.frame;
						ent.backlerp = 1.0 - cl.lerpfrac;

						if (renderfx & (RF_FRAMELERP|RF_BEAM))
						{	
							// step origin discretely, because the frames
							// do the animation properly
							VectorCopy (cent->current.origin, ent.origin);
							VectorCopy (cent->current.old_origin, ent.oldorigin);
						}
						else
						{	// interpolate origin
							for (i=0 ; i<3 ; i++)
							{
							//	ent.origin[i] = ent.oldorigin[i] = (cent->current.origin[i]);

								ent.origin[i] = ent.oldorigin[i] = cent->prev.origin[i] + cl.lerpfrac * 
									(cent->current.origin[i] - cent->prev.origin[i]);
							}
						}






						// calculate angles
						if (effects & EF_ROTATE)
						{	// some bonus items auto-rotate
							ent.angles[0] = 0;
							ent.angles[1] = autorotate;
							ent.angles[2] = 0;
						}
						// RAFAEL
						else if (effects & EF_SPINNINGLIGHTS)
						{
							ent.angles[0] = 0;
							ent.angles[1] = anglemod(cl.time/2) + s1->angles[1];
							ent.angles[2] = 180;
							{
								vec3_t forward;
								vec3_t start;

								AngleVectors (ent.angles, forward, NULL, NULL);
								VectorMA (ent.origin, 64, forward, start);
								V_AddLight (start, 100, 1, 0, 0);
							}
						}
						else
						{	// interpolate angles
							float	a1, a2;

							for (i=0 ; i<3 ; i++)
							{
								a1 = cent->current.angles[i];
								a2 = cent->prev.angles[i];
								ent.angles[i] = LerpAngle (a2, a1, cl.lerpfrac);

								// ent.angles[i] = cent->current.angles[i];
							}
						}




						...
						...

						V_AddEntity (&ent);

						...
						...

					}
				}






14.	now let us take a look at V_AddEntity(){}. You can see that the entity is added to r_entities

				cl_view.c

				void V_AddEntity (entity_t *ent)
				{
					if (r_numentities >= MAX_ENTITIES)
						return;
					r_entities[r_numentities++] = *ent;
				}





	if you do a search you might notice that in 


				cl_view.c

				V_RenderView()
				{


						...
						...

						cl.refdef.entities = r_entities;

						...
						...


					re.RenderFrame(&cl.refdef)

				}



	-	if you trace through what re.RenderFrame does, you will find out that passing in &cl.refdef initializes
		r_newrefdef. 


				gl_rmain.c
				
				refexport_t GetRefAPI (refimport_t rimp )
				{
					refexport_t	re;

					...
					...

					re.RenderFrame = R_RenderFrame;

					...
					...
				}



				gl_rmain.c
				void R_RenderFrame (refdef_t *fd)
				{
					R_RenderView( fd );
					R_SetLightLevel ();
					R_SetGL2D ();
				}




				gl_rmain.c

				/*
				================
				R_RenderView

				r_newrefdef must be set before the first call
				================
				*/
				void R_RenderView (refdef_t *fd)
				{
					if (r_norefresh->value)
						return;

					r_newrefdef = *fd;


					...
					...
				}




	and initalizing r_newrefdef matters becuz in R_DrawEntitesOnList, you can see our game iterates through the list of entities and actually renders them


				gl_main.c

				R_DrawEntitesOnList()
				{

					// draw non-transparent first
					for (i=0 ; i<r_newrefdef.num_entities ; i++)
					{
						currententity = &r_newrefdef.entities[i];
						if (currententity->flags & RF_TRANSLUCENT)
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

					...
					...

				}





