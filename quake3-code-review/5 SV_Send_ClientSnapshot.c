Now let us look at how the Server compiles a snapshot to the Client

1.	so in the SV_SendClientMessages(){}, we essentially go through each client and call SV_SendClientSnapshot(){}

					sv_snapshot.c

					SV_SendClientMessages()
					{
						int			i;
						client_t	*c;

						// send a message to each connected client
						for (i=0, c = svs.clients ; i < sv_maxclients->integer ; i++, c++) 
						{
							 printf("		examining %d\n", i);
							
							if (!c->state) {
								continue;		// not connected
							}

							if ( svs.time < c->nextSnapshotTime ) 
							{
								printf("			XXX c->nextSnapshotTime %d\n", c->nextSnapshotTime);
								continue;		// not time yet
							}

							// send additional message fragments if the last message
							// was too large to send at once
							if ( c->netchan.unsentFragments ) {
								c->nextSnapshotTime = svs.time + 
									SV_RateMsec( c, c->netchan.unsentLength - c->netchan.unsentFragmentStart );
								SV_Netchan_TransmitNextFragment( c );
								continue;
							}

							// generate and send a new message
							printf("			sending snapshot to %d, svs.time %d, c->nextSnapshotTime %d\n", i, svs.time, c->nextSnapshotTime);
							SV_SendClientSnapshot( c );
						}

					}






2.	Just like in quake2, there are a few steps in building the snapshot

	there are two main steps in SV_SendClientSnapshot(){}, as illustrated below:

	-	SV_BuildClientSnapshot(){}
		add all the entities that are visible to the client to a list in SV_BuildClientSnapshot(){}


	-	SV_WriteSnapshotToClient(){}
		then in SV_WriteSnapshotToClient(){} we go through each one entity and check if we should include it in the snapshot




					sv_snapshot.c

					SV_SendClientSnapshot()
					{

						// build the snapshot
						SV_BuildClientSnapshot( client );


						...
						...

						// send over all the relevant entityState_t
						// and the playerState_t
						SV_WriteSnapshotToClient( client, &msg );


						SV_SendMessageToClient( &msg, client );
					}




#########################################################################################
#############################	SV_BuildClientSnapshot(){} ##############################
#########################################################################################


1.	So the SV_BuildClientSnapshot is gonna add all the visible entities to a temporary list 

	-	we add all the visible object states to a temporary array, 	"entityNumbers", (yes that is name of the array)
		this is done in SV_AddEntitiesVisibleFromPoint(){}

		then in our major for loop, we transfer each states from the entityNumbers to svs.snapshotEntities (our circular buffer)


		the reason we first put them in a temporary array, entityNumbers, becuz it is easier to sort 
	
					qsort( entityNumbers.snapshotEntities, entityNumbers.numSnapshotEntities, 
					sizeof( entityNumbers.snapshotEntities[0] ), SV_QsortEntityNumbers );

		the comments says, they need to be sorted becuz of portal objects.

		as a matter of fact, in quake2 where there is not any portal objects, they just directly copy visible object states to 
		the svs.snapshotEntities (the circular buffer);

		the transfer of these objects happens in the main for loop below


					SV_BuildClientSnapshot()
					{

						snapshotEntityNumbers_t		entityNumbers;

						// bump the counter used to prevent double adding
						sv.snapshotCounter++;



						// this is the frame we are creating
						frame = &client->frames[ client->netchan.outgoingSequence & PACKET_MASK ];


						// clear everything in this snapshot
						entityNumbers.numSnapshotEntities = 0;



						// add all the entities directly visible to the eye, which
						// may include portal entities that merge other viewpoints
						SV_AddEntitiesVisibleFromPoint( org, frame, &entityNumbers, qfalse );											




						qsort( entityNumbers.snapshotEntities, entityNumbers.numSnapshotEntities, 
						sizeof( entityNumbers.snapshotEntities[0] ), SV_QsortEntityNumbers );
						...

						// copy the entity states out
						frame->num_entities = 0;
						frame->first_entity = svs.nextSnapshotEntities;
						for ( i = 0 ; i < entityNumbers.numSnapshotEntities ; i++ ) 
						{
							ent = SV_GentityNum(entityNumbers.snapshotEntities[i]);
							state = &svs.snapshotEntities[svs.nextSnapshotEntities % svs.numSnapshotEntities];
							*state = ent->s;
							svs.nextSnapshotEntities++;
							// this should never hit, map should always be restarted first in SV_Frame
							if ( svs.nextSnapshotEntities >= 0x7FFFFFFE ) {
								Com_Error(ERR_FATAL, "svs.nextSnapshotEntities wrapped");
							}
							frame->num_entities++;
						}

					}





#########################################################################################
#############################	SV_AddEntitiesVisibleFromPoint(){} ######################
#########################################################################################


1.	let is look inside what happens in SV_AddEntitiesVisibleFromPoint(){}, which is a main portion of step 1
	just like quake 2, we iterate through all the entities and add the ones that are visible to the client to a
	temporary list entityNumbers




				// add all the entities directly visible to the eye, which
				// may include portal entities that merge other viewpoints
				SV_AddEntitiesVisibleFromPoint( org, frame, &entityNumbers, qfalse )
				{


					leafnum = CM_PointLeafnum (origin);
					clientarea = CM_LeafArea (leafnum);
					clientcluster = CM_LeafCluster (leafnum);


					for ( e = 0 ; e < sv.num_entities ; e++ ) 
					{
						ent = SV_GentityNum(e);

						...
						...

						svEnt = SV_SvEntityForGentity( ent );

						...
						...

						// add it
						SV_AddEntToSnapshot( svEnt, ent, eNums );


						...
						...
						// something to do with the portal
					}
				}



2.	the SV_AddEntToSnapshot is very straightforward, it just adds the entity to entityNumbers

				sv_snapshot.c

				SV_AddEntToSnapshot( svEntity_t *svEnt, sharedEntity_t *gEnt, snapshotEntityNumbers_t *eNums ) 
				{
					...
					...
					...


					eNums->snapshotEntities[ eNums->numSnapshotEntities ] = gEnt->s.number;
					eNums->numSnapshotEntities++;
				}





####################################################################################################
################################### SV_WriteSnapshotToClient(){} ###################################
####################################################################################################

back to SV_BuildClientSnapshot(){}, we look at what happens in the second step
			


1.	then we look into SV_WriteSnapshotToClient(){}




	... to be continued, [similar to quake2]










#########################################################################################################################
############################################### SNAPSHOT PACKET STRUCTURE ###############################################
#########################################################################################################################


On the server side when the server sends the snapshot

					Netchan_Transmit( &client->netchan, msg->cursize, msg->data );
					{
						// init header
						MSG_InitOOB (&send, send_buf, sizeof(send_buf));		//	HEADER

						MSG_WriteLong( &send, chan->outgoingSequence );			// 	sequence number

						MSG_WriteData( &send, data, length );
						{
							MSG_WriteByte (msg, svc_snapshot);					//	Start of snapshot signal

							MSG_WriteByte (msg, ~snapshot_information~);		//	information about every entity	

							MSG_WriteByte( msg, svc_EOF );						//	End of snapshot signal
						}

					}





on the client side, when the client reads the snapshot


					CL_PacketEvent
					{


						if (!CL_Netchan_Process( &clc.netchan, msg) ) {
							return;		// out of order, duplicated, etc
						}


					}





					Netchan_Process
					{
						...
						...

						// get sequence numbers		
						MSG_BeginReadingOOB( msg );
						sequence = MSG_ReadLong( msg );

					}





