


1.	


					void trap_LinkEntity( gentity_t *ent ) {
						syscall( G_LINKENTITY, ent );
					}

					void trap_UnlinkEntity( gentity_t *ent ) {
						syscall( G_UNLINKENTITY, ent );
					}






					int SV_GameSystemCalls( int *args ) 
					{
						...
						...


						case G_LINKENTITY:
							SV_LinkEntity( VMA(1) );
							return 0;
						case G_UNLINKENTITY:
							SV_UnlinkEntity( VMA(1) );
							return 0;

						...
						...

					}



2.					


					void SV_LinkEntity()
					{

						...
						...

						ent = SV_SvEntityForGentity( gEnt );

					}






3.					
					void SV_SvEntityForGentity()
					{


					}








#####################################################################################################
################################### SV_UnlinkEntity(){} #############################################
#####################################################################################################





1.					


					g_main.c

				
					void G_RunFrame( int levelTime ) 
					{

						...
						...


						for (i=0 ; i<level.num_entities ; i++, ent++) 
						{		

							...
							...

							if ( ent->freeAfterEvent ) 
							{
								// tempEntities or dropped items completely go away after their event
								
								printf("	G_FreeEntity \n");
								G_FreeEntity( ent );
								continue;
							}

							...
							...





							// temporary entities don't think
							if ( ent->freeAfterEvent ) {
								continue;
							}

							if ( !ent->r.linked && ent->neverFree ) {
								continue;
							}

							if ( ent->s.eType == ET_MISSILE ) {
								G_RunMissile( ent );
								continue;
							}

							if ( ent->s.eType == ET_ITEM || ent->physicsObject ) {
								G_RunItem( ent );
								continue;
							}

							if ( ent->s.eType == ET_MOVER ) {
								G_RunMover( ent );
								continue;
							}


							...
							...

						}

					}


					




2.	

					void G_FreeEntity( gentity_t *ed ) 
					{
						trap_UnlinkEntity (ed);		// unlink from world

						if ( ed->neverFree ) {
							return;
						}

						memset (ed, 0, sizeof(*ed));
						ed->classname = "freed";
						ed->freetime = level.time;
						ed->inuse = qfalse;
					}




3.	

					case G_UNLINKENTITY:
						SV_UnlinkEntity( VMA(1) );
						return 0;











4.	

					void SV_UnlinkEntity( sharedEntity_t *gEnt ) 
					{
						ent = SV_SvEntityForGentity( gEnt );



					}
