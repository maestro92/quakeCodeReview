









		-	you can see that even the client side weapon firing animation is dictated by the server.
			you can just manually set the state->gunframe to a random number (6 in my case), and you can see that your animation
			will be fixed


					cl_ents.c

					void CL_ParsePlayerstate()
					{



						if (flags & PS_WEAPONFRAME)
						{
							
							state->gunframe = MSG_ReadByte (&net_message);		
							state->gunoffset[0] = MSG_ReadChar (&net_message)*0.25;
							state->gunoffset[1] = MSG_ReadChar (&net_message)*0.25;
							state->gunoffset[2] = MSG_ReadChar (&net_message)*0.25;
							state->gunangles[0] = MSG_ReadChar (&net_message)*0.25;
							state->gunangles[1] = MSG_ReadChar (&net_message)*0.25;
							state->gunangles[2] = MSG_ReadChar (&net_message)*0.25;
							

							// ADDED By Martin
							// you can see that even the client side animation is sent by the server
							state->gunframe = 6;
							printf("gunframe %d\n", state->gunframe);
							// 


						}

					}







3.	now lets take a look at how the client Parse the PacketEntities

	before delving deeper, I want to point out the one thing about "cl_entities" and "cl_parse_entities"
	it is declared in client.h

					client.h		

					extern	centity_t	cl_entities[MAX_EDICTS];
					extern	cdlight_t	cl_dlights[MAX_DLIGHTS];

					// the cl_parse_entities must be large enough to hold UPDATE_BACKUP frames of
					// entities, so that when a delta compressed message arives from the server
					// it can be un-deltad from the original 
					#define	MAX_PARSE_ENTITIES	1024
					extern	entity_state_t	cl_parse_entities[MAX_PARSE_ENTITIES];


	and somehow it is declared again in cl_main.c


					cl_main.c 

					centity_t		cl_entities[MAX_EDICTS];

					entity_state_t	cl_parse_entities[MAX_PARSE_ENTITIES];


	regardless, I want to mention these two to mainly point out that when the client parses the entity information from the server, it will use these two arrays

	it also seems to me that on the client side, the client frame does not actually use g_edicts, instead it uses cl_entites to iterate and refernece
	edicts in the game

	as a matter of fact, the client can not even access g_edicts, or ge->edicts. That is only used on the server side.
	Therefore on the client side, it uses cl_entites to access all the entities.

	recall that the server only sends down a subset of the entities that is visible to the client

	for example in cl_main.c, we see
					
					cl_main.c

					void CL_ClearState (void)
					{
						S_StopAllSounds ();
						CL_ClearEffects ();
						CL_ClearTEnts ();

					// wipe the entire cl structure
						memset (&cl, 0, sizeof(cl));
						memset (&cl_entities, 0, sizeof(cl_entities));		<-------

						SZ_Clear (&cls.netchan.message);
					}

	when it clears, it only clears cl_entities






######################################################################################################################
################################################# CL_ParsePacketEntities #############################################
######################################################################################################################

Let us take a look at how the client parses the snapshots from the server

1.	we start from CL_ParseServerMessage(){}, which calls CL_ParseFrame(){};

					cl_parse.c

					CL_ParseServerMessage
					{
						...
						...

						case svc_frame:
							// printf("svc_frame\n\n");
							CL_ParseFrame ();
							break;
						
						...
						...
					}



2.	now let see what is going on in here
	
	note that later on, CL_ParseFrame will call CL_ParsePacketEntities(){}, and for the CL_ParsePacketEntities(){} to work


	-	in order to parse the snapshot


					cl_ents.c

					void CL_ParseFrame (void)
					{
						frame_t		*old;


						memset (&cl.frame, 0, sizeof(cl.frame));

						cl.frame.serverframe = MSG_ReadLong (&net_message);
						cl.frame.deltaframe = MSG_ReadLong (&net_message);
						cl.frame.servertime = cl.frame.serverframe*100;


						...
						...

						if (cl.frame.deltaframe <= 0)
						{
							cl.frame.valid = true;		// uncompressed frame
							old = NULL;
							cls.demowaiting = false;	// we can start recording now
						}
						else
						{
							old = &cl.frames[cl.frame.deltaframe & UPDATE_MASK];
						

						}





						// read packet entities
						cmd = MSG_ReadByte (&net_message);
						SHOWNET(svc_strings[cmd]);
						if (cmd != svc_packetentities)
							Com_Error (ERR_DROP, "CL_ParseFrame: not packetentities");
						CL_ParsePacketEntities (old, &cl.frame);



						...
						...

						// save the frame off in the backup array for later delta comparisons
						cl.frames[cl.frame.serverframe & UPDATE_MASK] = cl.frame;

					}











2.	Finally, now lets look into CL_ParsePacketEntities(){};

	-	just inside the while(1) condition, you can see that we have the code

					if (!newnum)
						break;

		if we have read how the server sends the PacketEntities, you can recall the server adds a MSG_WriteShort(msg, 0){} at the end
		to mark the end of the packetEntities. This is how the client knows when it reaches the end 


	-	what is going on here is that in the main while loop, we try to read the newnum. Then we need to compare this newnum with a old-object index

		which is why before the main while loop, and every time before a continue statement, we have to find the next oldnum to setup for the next iteration of the while loop


					cl_ents.c

					void CL_ParsePacketEntities (frame_t *oldframe, frame_t *newframe)
					{

						...
						...

						newframe->parse_entities = cl.parse_entities;
						newframe->num_entities = 0;


						// delta from the entities present in oldframe
						oldindex = 0;
						if (!oldframe)
							oldnum = 99999;
						else
						{
							if (oldindex >= oldframe->num_entities)
								oldnum = 99999;
							else
							{
								oldstate = &cl_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
								oldnum = oldstate->number;
							}
						}




						while (1)
						{
							newnum = CL_ParseEntityBits (&bits);


							if (!newnum)
								break;


							while (oldnum < newnum)
							{	
								// one or more entities from the old packet are unchanged
								...
								...
								CL_DeltaEntity (newframe, oldnum, oldstate, 0);
								
								oldindex++;

								//###### changed by Martin ######
								*RESET_oldnum();
								//###### end change ######
							}


							// removing entities
							if (bits & U_REMOVE)
							{	
								
								// the entity present in oldframe is not in the current frame
								...
								...

								oldindex++;

								//###### changed by Martin ######
								*RESET_oldnum();
								//###### end change ######
								continue;
							}

							// existing entities
							if (oldnum == newnum)
							{	
								...
								...								

								CL_DeltaEntity (newframe, newnum, oldstate, bits);
								oldindex++;


								//###### changed by Martin ######
								*RESET_oldnum();
								//###### end change ######
								continue;
							}

							// adding entities
							if (oldnum > newnum)
							{				
								...
								...
								CL_DeltaEntity (newframe, newnum, &cl_entities[newnum].baseline, bits);
								continue;
							}
						}

						// any remaining entities in the old frame are copied over
						while (oldnum != 99999)
						{	
							// one or more entities from the old packet are unchanged
							CL_DeltaEntity (newframe, oldnum, oldstate, 0);
							
							oldindex++;

							//###### changed by Martin ######
							*RESET_oldnum();
							//###### end change ######
						}
					}


		*	here I have abbreciated the RESET_oldnum(){} code to below to make the code more readable
			essentially what the code is doing it resets the oldnum for the next iteration

					RESET_oldnum()
					{
						if (oldindex >= oldframe->num_entities)
							oldnum = 99999;
						else
						{
							oldstate = &cl_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
							oldnum = oldstate->number;
						}
					}




	-	the CL_ParseEntityBits(); function, like what the comments says just "Returns the entity number and the header bits"

					cl_ents.c

					int CL_ParseEntityBits()
					{
						...
						...
					}



	-	you can see that we have a nested while loop "while (oldnum < newnum)" in the main while(1) loop. Recall that in the server side,
		it does not actually send information about an entity if it does not have updates. Below are some examples of why we have this nested while loop

###################################################
################# Adding an Entity ################
###################################################					

1.	in the case where the server is adding an entity


			Client 												Server

																WriteFrameToClient
																{
																	...
																	...

																	oldnum 1
																	newnum 1
																		existing entity 		<-- forced to send cuz it is a player

																	oldnum 23
																	newnum 23
																	existing entity
																		nothing to send 		<-- entity did not change

																	oldnum 40
																	newnum 40
																	existing entity
																		nothing to send 		<-- entity did not change

																	oldnum 98
																	newnum 98
																	existing entity
																		nothing to send 		<-- entity did not change

																	oldnum 109
																	newnum 109
																	existing entity
																		nothing to send 		<-- entity did not change																		

																	oldnum 9999
																	newnum 128
																	existing entity
																		adding new entity 		
																	...
																	...
																}


			CL_ReadPackets
			{

				oldnum 1
				newnum 1
					existing entity

				oldnum 23
				newnum 128
					reading ADDING new entity

				looping in nested while loop

					oldnum 23
					oldnum 40
					oldnum 98
					oldnum 109
					oldnum 99999		<---- becuz we have reached the end of our previous list

					then run the if condition 

					if (oldnum > newnum)
					{				
						// delta from baseline
						if (cl_shownet->value == 3)
							Com_Printf ("   baseline: %i\n", newnum);

						printf("		adding entity \n");

						CL_DeltaEntity (newframe, newnum, &cl_entities[newnum].baseline, bits);
						continue;
					}

				...
				...

			}




	-	you might have wondered why do we have an extra while loop even after the main while(1){} loop. The reason why is becuz if you recall 
		how the server sends information in MSG_WriteDeltaEntity(){}, he actually does not send anything when the entities have no updates.
		so therefore, like what the comments says, we need to make sure that "any remaining (unchanged) entities in the old frame are copied over"


		below is an typical example
		as we iterate through tne entities on the Server side, we eventually only decide to send udpates for the player (entity 1).
		Then when the client tries the parse the data and tries to construct its list for the current frame in CL_ReadPackets(), he only sees that 
		bits for entity1. However, the client is not done constructing the entities for this frame, so he still has copied over the unchanged ones from 
		the previous frame



			Client 												Server

																WriteFrameToClient
																{
																	...
																	...

																	oldnum 1
																	newnum 1
																		existing entity 		<-- forced to send cuz it is a player

																	oldnum 23
																	newnum 23
																	existing entity
																		nothing to send 		<-- entity did not change

																	oldnum 40
																	newnum 40
																	existing entity
																		nothing to send 		<-- entity did not change

																	oldnum 50
																	newnum 50
																	existing entity
																		nothing to send 		<-- entity did not change
																	...
																	...
																}


			CL_ReadPackets
			{

				oldnum 1
				newnum 1
					existing entity

				oldnum 23
				newnum 0
					the end of entityPackets, but we still need to copy over the remainder entities

				HERE COPYING REMAINDER unchanged entities

				oldnum 23
				oldnum 40
				oldnum 50
				...
				...

			}








4.	recall that the server uses svs.client_entities as the temporary array when sending information to the client
	i guess the client also uses a temporary array in cl_entities and cl_parse_entities.

	-	the cl.parse_entities functions similarly to svs.next_client_entities. which is a ever increasing number used in a circular array


	-	notice in the line 

					ent->prev = ent->current;
					...
					...
					ent->current = *state;

		the reason this is happening is that ent->prev and ent->current will be used for lerping, so we first assing prev with the current
		then assign ent->current with the new one



					cl_ents.c


					CL_DeltaEntity()
					{
						...
						...
						
						ent = &cl_entities[newnum];

						state = &cl_parse_entities[cl.parse_entities & (MAX_PARSE_ENTITIES-1)];
						cl.parse_entities++;
						frame->num_entities++;

						CL_ParseDelta (old, state, newnum, bits);

						...
						...


						if (ent->serverframe != cl.frame.serverframe - 1)
						{	// wasn't in last update, so initialize some things
							ent->trailcount = 1024;		// for diminishing rocket / grenade trails
							// duplicate the current state so lerping doesn't hurt anything
							ent->prev = *state;
							if (state->event == EV_OTHER_TELEPORT)
							{
								VectorCopy (state->origin, ent->prev.origin);
								VectorCopy (state->origin, ent->lerp_origin);
							}
							else
							{
								VectorCopy (state->old_origin, ent->prev.origin);
								VectorCopy (state->old_origin, ent->lerp_origin);
							}
						}
						else
						{	// shuffle the last state to previous
							ent->prev = ent->current;
						}

						ent->serverframe = cl.frame.serverframe;
						ent->current = *state;
					}




					CL_ParseDelta()
					{


					}