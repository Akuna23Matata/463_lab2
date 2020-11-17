#include "ne.h"
#include "router.h"
#include <stdbool.h>

/* ----- GLOBAL VARIABLES ----- */
struct route_entry routingTable[MAX_ROUTERS];
int NumRoutes;

//this is a helper function to copy path from newRoute[fromI] to routingTable[toI] with both path and path_len
//changed properly


////////////////////////////////////////////////////////////////
/* Routine Name    : InitRoutingTbl
 * INPUT ARGUMENTS : 1. (struct pkt_INIT_RESPONSE *) - The INIT_RESPONSE from Network Emulator
 *                   2. int - My router's id received from command line argument.
 * RETURN VALUE    : void
 * USAGE           : This routine is called after receiving the INIT_RESPONSE message from the Network Emulator. 
 *                   It initializes the routing table with the bootstrap neighbor information in INIT_RESPONSE.  
 *                   Also sets up a route to itself (self-route) with next_hop as itself and cost as 0.
 */
void InitRoutingTbl (struct pkt_INIT_RESPONSE *InitResponse, int myID){
	/* ----- YOUR CODE HERE ----- */
	// go thorough all connected neighbers
	int i;
	NumRoutes = InitResponse->no_nbr;
	while(i < NumRoutes){
		//assign info from init response
		routingTable[i].dest_id = InitResponse->nbrcost[i].nbr;
		routingTable[i].next_hop = InitResponse->nbrcost[i].nbr;
		routingTable[i].cost = InitResponse->nbrcost[i].cost;
		routingTable[i].path_len = 2;
		routingTable[i].path[0] = myID;
		routingTable[i].path[1] = routingTable[i].dest_id;
		i++;
	}
	//add self route
	routingTable[i].dest_id = myID;
	routingTable[i].next_hop = myID;
	routingTable[i].cost = 0;
	routingTable[i].path_len = 1;
	routingTable[i].path[0] = myID;
	NumRoutes += 1;
	return;
}


////////////////////////////////////////////////////////////////
/* Routine Name    : UpdateRoutes
 * INPUT ARGUMENTS : 1. (struct pkt_RT_UPDATE *) - The Route Update message from one of the neighbors of the router.
 *                   2. int - The direct cost to the neighbor who sent the update. 
 *                   3. int - My router's id received from command line argument.
 * RETURN VALUE    : int - Return 1 : if the routing table has changed on running the function.
 *                         Return 0 : Otherwise.
 * USAGE           : This routine is called after receiving the route update from any neighbor. 
 *                   The routing table is then updated after running the distance vector protocol. 
 *                   It installs any new route received, that is previously unknown. For known routes, 
 *                   it finds the shortest path using current cost and received cost. 
 *                   It also implements the forced update and split horizon rules. My router's id
 *                   that is passed as argument may be useful in applying split horizon rule.
 */
int UpdateRoutes(struct pkt_RT_UPDATE *RecvdUpdatePacket, int costToNbr, int myID){
	/* ----- YOUR CODE HERE ----- */
	//go thorough all routes in the updated packet
	int dest;	//destination router
	int to_dest; //index of the found path in routing table
	int to_nbr = -1; //index of the path to the neighbor
	bool forceUpdate = false;
	bool isNewRoute = true;
	int rtn = 0;
	//find the index of the path to neighbor
	for(int i = 0; i < NumRoutes; i++){
		if(routingTable[i].dest_id == RecvdUpdatePacket->sender_id){
			to_nbr = i;
		}
	}
	//shouldnt happen, just in case, no path found to this 'neighbor'
	if(to_nbr == -1){
		return 0;
	}
	//go thorough the neighbor's routingTable
	for(int i = 0; i < RecvdUpdatePacket->no_routes; i++){
		//Path vector rule: only update when R(myID) is not in path of R(i)
		//Force update rule: force update when R(myID) is in path of R(i), and cost is higher
		to_dest = -1;
		//first check if dest already in curret table
		dest = RecvdUpdatePacket->route[i].dest_id;
		for(int x = 0; x < NumRoutes; x++){
			if(routingTable[x].dest_id == dest){
				to_dest = x;
			}
		}
		//if no path found, use this path
		if(to_dest == -1){
			routingTable[NumRoutes].dest_id = dest;
			routingTable[NumRoutes].next_hop = RecvdUpdatePacket->sender_id;
			routingTable[NumRoutes].cost = costToNbr + RecvdUpdatePacket->route[i].cost;
			//path to neighbor first
			routingTable[NumRoutes].path_len = 1;
			routingTable[NumRoutes].path[0] = myID;
			//then cpy rest
			for(int x = 0; x < RecvdUpdatePacket->route[i].path_len; x++){
				routingTable[NumRoutes].path[1 + x] = RecvdUpdatePacket->route[i].path[x];
				routingTable[NumRoutes].path_len += 1;
			}
			NumRoutes += 1;
			rtn = 1;
		}
		//first check if force update rule suits
		//when new cost is higher
		else if((costToNbr + RecvdUpdatePacket->route[i].cost) > routingTable[to_dest].cost){
			//check if r(i) is in path of r(myID) to r(dest)
			//find the route to dest
			for(int x = 0; x < routingTable[to_dest].path_len; x++){
				if(routingTable[to_dest].path[x] == RecvdUpdatePacket->sender_id){
					forceUpdate = true;
				}
			}
			//update if forceUpdate flag is true
			if(forceUpdate){
				routingTable[to_dest].dest_id = dest;
				routingTable[to_dest].next_hop = RecvdUpdatePacket->sender_id;
				routingTable[to_dest].cost = costToNbr + RecvdUpdatePacket->route[i].cost;
				//cpy path to neighbor first
				routingTable[to_dest].path_len = 1;
				routingTable[to_dest].path[0] = myID;
				//then cpy rest
				for(int x = 0; x < RecvdUpdatePacket->route[to_dest].path_len; x++){
					routingTable[to_dest].path[1 + x] = RecvdUpdatePacket->route[i].path[x];
					routingTable[to_dest].path_len += 1;
				}
				rtn = 1;
			}
		}
		//try path vector rule
		//first check if the path is actually shorter
		//if longer, we dont have to do anything
		else if((costToNbr + RecvdUpdatePacket->route[i].cost) < routingTable[to_dest].cost){
			//check if is in path
			isNewRoute = true;
			//have to check every router in path
			for(int x = 0; x < RecvdUpdatePacket->route[i].path_len; x++){
				if(myID == RecvdUpdatePacket->route[i].path[x]){
					isNewRoute = false;
				}
			}
			if(isNewRoute){
				routingTable[to_dest].dest_id = dest;
				routingTable[to_dest].next_hop = RecvdUpdatePacket->sender_id;
				routingTable[to_dest].cost = costToNbr + RecvdUpdatePacket->route[i].cost;
				//cpy path to neighbor first
				routingTable[to_dest].path_len = 1;
				routingTable[to_dest].path[0] = myID;
				//then cpy rest
				for(int x = 0; x < RecvdUpdatePacket->route[i].path_len; x++){
					routingTable[to_dest].path[1 + x] = RecvdUpdatePacket->route[i].path[x];
					routingTable[to_dest].path_len += 1;
				}
				rtn = 1;
			}
		}
	}
	return rtn;
}

////////////////////////////////////////////////////////////////
/* Routine Name    : ConvertTabletoP kt
 * INPUT ARGUMENTS : 1. (struct pkt_RT_UPDATE *) - An empty pkt_RT_UPDATE structure
 *                   2. int - My router's id received from command line argument.
 * RETURN VALUE    : void
 * USAGE           : This routine fills the routing table into the empty struct pkt_RT_UPDATE. 
 *                   My router's id  is copied to the sender_id in pkt_RT_UPDATE. 
 *                   Note that the dest_id is not filled in this function. When this update message 
 *                   is sent to all neighbors of the router, the dest_id is filled.
 */
void ConvertTabletoPkt(struct pkt_RT_UPDATE *UpdatePacketToSend, int myID){
	/* ----- YOUR CODE HERE ----- */
	//just copt everything from routing table
	UpdatePacketToSend->sender_id = myID;
	UpdatePacketToSend->no_routes = NumRoutes;
	for(int i = 0; i < NumRoutes; i++){
		UpdatePacketToSend->route[i].dest_id = routingTable[i].dest_id;
		UpdatePacketToSend->route[i].next_hop = routingTable[i].next_hop;
		UpdatePacketToSend->route[i].cost = routingTable[i].cost;
		UpdatePacketToSend->route[i].path_len = routingTable[i].path_len;
		for(int x = 0; x < UpdatePacketToSend->route[i].path_len; x++){
			UpdatePacketToSend->route[i].path[x] = routingTable[i].path[x];
		}
	}
	return;
}


////////////////////////////////////////////////////////////////
//It is highly recommended that you do not change this function!
void PrintRoutes (FILE* Logfile, int myID){
	/* ----- PRINT ALL ROUTES TO LOG FILE ----- */
	int i;
	int j;
	for(i = 0; i < NumRoutes; i++){
		fprintf(Logfile, "<R%d -> R%d> Path: R%d", myID, routingTable[i].dest_id, myID);

		/* ----- PRINT PATH VECTOR ----- */
		for(j = 1; j < routingTable[i].path_len; j++){
			fprintf(Logfile, " -> R%d", routingTable[i].path[j]);	
		}
		fprintf(Logfile, ", Cost: %d\n", routingTable[i].cost);
	}
	fprintf(Logfile, "\n");
	fflush(Logfile);
}


////////////////////////////////////////////////////////////////
void UninstallRoutesOnNbrDeath(int DeadNbr){
	/* ----- YOUR CODE HERE ----- */
	for(int i = 0; i < NumRoutes; i++){
		for(int x = 0; x < routingTable[i].path_len; x++){
			if(DeadNbr == routingTable[i].path[x]){
				routingTable[i].cost = INFINITY;
			}
		}
	}
	return;
}