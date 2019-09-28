
The code for the netchan_t is below

				typedef struct
				{
					qboolean	fatal_error;

					netsrc_t	sock;

					int			dropped;			// between last packet and previous

					int			last_received;		// for timeouts
					int			last_sent;			// for retransmits

					netadr_t	remote_address;
					int			qport;				// qport value to write when transmitting

				// sequencing variables
					int			incoming_sequence;
					int			incoming_acknowledged;
					int			incoming_reliable_acknowledged;	// single bit

					int			incoming_reliable_sequence;		// single bit, maintained local

					int			outgoing_sequence;
					int			reliable_sequence;			// single bit
					int			last_reliable_sequence;		// sequence number of last send

				// reliable staging and holding areas
					sizebuf_t	message;		// writing buffer to send to server
					byte		message_buf[MAX_MSGLEN-16];		// leave space for header

				// message is copied to this buffer when it is first transfered
					int			reliable_length;
					byte		reliable_buf[MAX_MSGLEN-16];	// unacked reliable message
				} netchan_t;


	Note that the client and the server keeps different counters.


	outgoing_sequence:
		the local counter for the sequence number that it will send to the remote target

	incoming_acknowledged:
		this is the for acknowledged sequence number for the local counter
		

	incoming_sequence:
		the sequence number for the remote counter



	below shows an example of what I mean. These are all printed in the Netchan_Process function
	Assuming I am the server, you can see that "inc_ack"(incoming_acknowledged) is in "local counter scope",
	whereas "inc_seq"(incoming_sequence) is in "remote counter scope"




Client																				Server
	

Example						

client
	out_seq 180
	inc_seq 69
	inc_ack 179
	inc_rel_ack 1
	inc_rel_seq 1
	last_rel_seq 16

																					server
																						out_seq 70
																						inc_seq 180
																						inc_ack 69
																						inc_rel_ack 1
																						inc_rel_seq 1
																						last_rel_seq 16


																					server
																						out_seq 70
																						inc_seq 181
																						inc_ack 69
																						inc_rel_ack 1
																						inc_rel_seq 1
																						last_rel_seq 16


																					server
																						out_seq 70
																						inc_seq 182
																						inc_ack 69
																						inc_rel_ack 1
																						inc_rel_seq 1
																						last_rel_seq 16


																					server
																						out_seq 70
																						inc_seq 183
																						inc_ack 69
																						inc_rel_ack 1
																						inc_rel_seq 1
																						last_rel_seq 16


																					server
																						out_seq 70
																						inc_seq 184
																						inc_ack 69
																						inc_rel_ack 1
																						inc_rel_seq 1
																						last_rel_seq 16


																					server
																						out_seq 70
																						inc_seq 185
																						inc_ack 69
																						inc_rel_ack 1
																						inc_rel_seq 1
																						last_rel_seq 16
client
	out_seq 186
	inc_seq 70
	inc_ack 185
	inc_rel_ack 1
	inc_rel_seq 1
	last_rel_seq 16



so even when I only have one player in the multiplayer game that I just set up, server seems to be running at a faster pace


