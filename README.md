# cpp_tcp_local_chatrooms
C++ implementation for TCP and LAN based Multi-Client Chat Server with Group Support for Selective Broadcasts.

Handles max 5 clients.
	Source Files: server.c client.c



Compilation:
	gcc server.c -o s.out
	gcc client.c -o c.out

	Or use the makefile provided.



Running:
	Server:
		./s.out
		Provide port number to use. Application will use localhost as Hostname.
	CLient: 
		./c.out
		Provide port number to use. Application will use localhost as Hostname.

	Follow subsequent instructrions carefully.



Functionalities Implemented and commands to use on Client/Server:

	1. /active : To display all the available active clients that are connected to the server.
	2. /send <dest client id> <Message> : To send message to the client corresponding to its unique id.
	3. /broadcast <Message> : Message should be broadcasted to all the active clients.
	4. /makegroup <client id1> <client id2> ... <client idn> : A group with unique id will be made including 
			all the mentioned clients along with the admin client.
	5. /sendgroup <group id> <Message>: The sender should be in the group to transfer the message to all his 
			peers of that group. The message should be send to all the peers along with group info.
	6. /activegroups : To display all the groups that are currently active on server and the sender is a part of.
	7. /makegroupreq <client id1> <client id2> ... <client idn> : A group having unique id should be made with 
			currently only the admin client. The request message for joining the group should be notified 
			to all the specified clients. Clients can respond to join that group.
	8. /joingroup <group id> : If this message is sent by a client having the request for joining the group, then 
				he will be added to group immediately.
	9. /declinegroup <group id> : If this message is sent by a client having the request for joining the group, then 
					client will not be added to the group.
	10. /quit : The client will be removed from the server. This client will be removed from all the active groups.
	11. /activeallgroups : To display all the groups which are active on the server.
	12. /joingroup <group id>: If this message is sent by a client having the request for
				joining the group, then he will be added to group immediately. Otherwise a request
				should be passed to the admin of that group and if admin responds to the request
				positively then he should be joined to that group.
