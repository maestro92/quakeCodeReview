



1.	

	Notice in this function, we have a major for loop where it checks packets from each connected client

	and notice that we pass in &cl->netchan, and &net_message to Netchan_Process function
	the reason why we are doing this is because net_message has information about this new message client has, yet
	we store all the past information from this client in &cl->netchan (such as last asknkowledged sequence)


				sv_main.c

				SV_ReadPackets (void)
				{


					while (NET_GetPacket (NS_SERVER, &net_from, &net_message))
					{

						...
						...
						// check for packets from connected clients
						for (i=0, cl=svs.clients ; i<maxclients->value ; i++,cl++)
						{
							if (Netchan_Process(&cl->netchan, &net_message))
							{	
								// this is a valid, sequenced packet, so process it
								if (cl->state != cs_zombie)
								{
									cl->lastmessage = svs.realtime;	// don't timeout
									SV_ExecuteClientMessage (cl);
								}
							}
						}
					}
				}



2.	so we look into Netchan_Process

	-	we first retrieve the information from the new message

		sequence: 
			new sequence sent by the client
		
		reliable_message: 
			bit to see if it is a reliable message

		sequence_ack:


		reliable_ack:
			even or odd bit for the reliable message

			// so the 



	-	we first check if it is a duplicated packets, by comparing sequence and chan->incoming_sequence
		recall that chan->incoming_sequence stores the lastest message from the client,
		so if the new arriving sequence number is less than what we stored, than it is an "Out of order", duplicated packet


	-	to check if we dropped a message, we compare sequence and chan->incoming_sequence + 1
		since we want sequence to equal exaclty chan->incoming_sequence + 1, 

		Example: 

		if sequence = chan->incoming_sequence + 2, that means chan->incoming_sequence + 1 was dropped, which is no good



	-	after all the "error checking" for the messages, we update the information on chan
		


				qboolean Netchan_ProcessSV(netchan_t *chan, sizebuf_t *msg)
				{


					reliable_message = sequence >> 31;
					reliable_ack = sequence_ack >> 31;

					sequence &= ~(1 << 31);
					sequence_ack &= ~(1 << 31);




					if (sequence <= chan->incoming_sequence)	
					{

					}


					//
					// dropped packets don't keep the message from being used
					//
					chan->dropped = sequence - (chan->incoming_sequence + 1);
					if (chan->dropped > 0)
					{

					}



					//
					// if this message contains a reliable message, bump incoming_reliable_sequence 
					//
					chan->incoming_sequence = sequence;
					chan->incoming_acknowledged = sequence_ack;
					chan->incoming_reliable_acknowledged = reliable_ack;
					if (reliable_message)
					{
						chan->incoming_reliable_sequence ^= 1;
					}

				}









	-	note this is called SV_ClientThink because this is the server running the client think function
		As the server just received the command inputs sent by the client, so it will then run the ClientThink function

				sv_user.c

				void SV_ClientThink()
				{


					ge->ClientThink(cl->edict, cmd)
				}





				/*
				==============
				ClientThink

				This will be called once for each client frame, which will
				usually be a couple times for each server frame.
				==============
				*/
				void ClientThink (edict_t *ent, usercmd_t *ucmd)
				{

				}



In ClientThink, which is called more frequently, 
the player has a chance to move and his weapon gets a chance to animate. 
The touch functions for entities touched by the player are called from ClientThink.

