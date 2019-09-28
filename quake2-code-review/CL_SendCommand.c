			cl_input.c

			void CL_SendCommand (void)
			{
				// get new key events
				Sys_SendKeyEvents ();

				// allow mice or other external controllers to add commands
				IN_Commands ();

				// process console commands
				Cbuf_Execute ();

				// fix any cheating cvars
				CL_FixCvarCheats ();

				// send intentions now
				CL_SendCmd ();

				// resend a connection request if necessary
				CL_CheckForResend ();
			}










				CL_FinishMove()
				{

					...
					...

					if (in_attack.state & 3)				
						cmd->buttons |= BUTTON_ATTACK;
					
					in_attack.state &= ~2;

					...
				}


		and you can see "cmd->buttons |= BUTTON_ATTACK" is where the player gets the Mouse Left Click










	-	one important thing here is that I want to point out Netchan_NeedReliable()

			cl_input.c

			CL_SendCmd()
			{

				...
				...



				send_reliable = Netchan_NeedReliable (chan);


				w1 = ( chan->outgoing_sequence & ~(1<<31) ) | (send_reliable<<31);
				w2 = ( chan->incoming_sequence & ~(1<<31) ) | (chan->incoming_reliable_sequence<<31);

				chan->outgoing_sequence++;
				chan->last_sent = curtime;

				MSG_WriteLong (&send, w1);
				MSG_WriteLong (&send, w2);

				...
				...



				// copy the reliable message to the packet first
				if (send_reliable)
				{
					SZ_Write (&send, chan->reliable_buf, chan->reliable_length);
					chan->last_reliable_sequence = chan->outgoing_sequence;
				}

			}




3.	let us look inside Netchan_NeedReliable(netchan_t *chan)

			net_chan.c

			Netchan_NeedReliable()
			{
				qboolean	send_reliable;

				// if the remote side dropped the last reliable message, resend it
				send_reliable = false;

				if (chan->incoming_acknowledged > chan->last_reliable_sequence
				&& chan->incoming_reliable_acknowledged != chan->reliable_sequence)
					send_reliable = true;

				// if the reliable transmit buffer is empty, copy the current message out
				if (!chan->reliable_length && chan->message.cursize)
				{
					send_reliable = true;
				}

				return send_reliable;



			}




