









2.	so the general idea is pretty well understood already,
	we run every single commad we have sent since the most recent acknowledged one we have received
	from the server


			cl_pred.c

			CL_PredictMovement()
			{


				...
				...

				if (!cl_predict->value || (cl.frame.playerstate.pmove.pm_flags & PMF_NO_PREDICTION))
				{	// just set angles
					for (i=0 ; i<3 ; i++)
					{
						cl.predicted_angles[i] = cl.viewangles[i] + SHORT2ANGLE(cl.frame.playerstate.pmove.delta_angles[i]);
					}
					return;
				}


				ack = cls.netchan.incoming_acknowledged;
				current = cls.netchan.outgoing_sequence;


				// if we are too far out of date, just freeze
				if (current - ack >= CMD_BACKUP)
				{
					if (cl_showmiss->value)
						Com_Printf ("exceeded CMD_BACKUP\n");
					return;	
				}




					
				// run frames
				while (++ack < current)
				{
					frame = ack & (CMD_BACKUP-1);
					cmd = &cl.cmds[frame];

					pm.cmd = *cmd;
					Pmove (&pm);

					// save for debug checking
					VectorCopy (pm.s.origin, cl.predicted_origins[frame]);
				}

			}






				/*
				================
				Pmove

				Can be called by either the server or the client
				================
				*/
				pmove.c

				void Pmove (pmove_t *pmove)
				{


				}