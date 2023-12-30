import sys 
import copy
import grpc
import socket  # for gethostbyname()
import logging
import csci4220_hw4_pb2
import csci4220_hw4_pb2_grpc
from concurrent import futures

N = 4

def XOR_dist(local_id, remote_id):
    return local_id ^ remote_id

def BOOTSTRAP(remote_hostname, remote_port, local_obj):
    '''
    lets a node connect to another node by exchanging information so that both nodes know each
    other's ID, address, and port.  Both nodes should add each other to their k-buckets.
    
    :param remote_hostname: hostname that requires to connect to the current Node
    :param remote_poart: port that requires to connect to the current Node
    '''

    remote_host_addr = socket.gethostbyname(remote_hostname)
    #remote_host_addr = socket.gethostname()

    channel = grpc.insecure_channel('{}:{}'.format(remote_host_addr, remote_port))
    stub = csci4220_hw4_pb2_grpc.KadImplStub(channel)
    # FindNode() return k nearest id
    Idkey = csci4220_hw4_pb2.IDKey(node=local_obj.node, idkey=local_obj.node.id)
    R = stub.FindNode(Idkey)

    # Update k-buckets with node
    local_obj.Add_node(R.responding_node)

    # Update k-buckets with all nodes in R
    for temp_node in R.nodes:
        local_obj.Add_node(temp_node)

    sys.stdout.write("After BOOTSTRAP({}), k-buckets are:\n".format(R.responding_node.id))
    local_obj.print_buckets()

def FIND_NODE(node_id, local_obj):
    '''
    attempts to find a remote node, and has the side effect of updating the current node's k-buckets.
    '''
    sys.stdout.write("Before FIND_NODE command, k-buckets are:\n")
    local_obj.print_buckets()

    # If the node’s ID is <nodeID>, no search should be made and behave as though it found the node.
    find_target = False
    if (node_id == local_obj.node.id):
        find_target = True
    else:

        S = local_obj.get_k_closest(remote_id = node_id)
        S_prime = copy.copy(S)

        for temp_node in S_prime:
            # if contacted, skip this one
            if (temp_node not in S_prime):
                continue

            S_prime.remove(temp_node)

            idkey = csci4220_hw4_pb2.IDKey( node  = local_obj.node,	idkey = node_id )
            channel = grpc.insecure_channel('{}:{}'.format(temp_node.address, temp_node.port))
            stub = csci4220_hw4_pb2_grpc.KadImplStub(channel)
            R = stub.FindNode(idkey)

            # Update k-buckets with node
            local_obj.Add_node(R.responding_node)
            
            # Update k-buckets with all nodes in R
            for temp_R in R.nodes:
                local_obj.Add_node(temp_R)
                if (node_id == temp_R.id):
                    find_target = True
                    
            if (find_target):
                break
        
    # if found target node IDd, the program should then print: Found destination id <nodeID>
    if (find_target):
        sys.stdout.write("Found destination id {}\n".format(node_id))
    # Otherwise the program should print: Could not find destination id <nodeID>
    else:
        sys.stdout.write("Could not find destination id {}\n".format(node_id))
    
    sys.stdout.write("After FIND_NODE command, k-buckets are:\n")
    local_obj.print_buckets()

def FIND_VALUE(key, local_obj):
    '''
    behaves the same way that FIND_NODE does, 
    but uses the key instead of a node ID to determine which node to query next, 
    and the FindValue RPC instead of the FindNode RPC.
    '''
    sys.stdout.write("Before FIND_VALUE command, k-buckets are:\n")
    local_obj.print_buckets()

    find_target = False
    # If the target key stored on current node, print: Found data "<value>" for key <key>
    if (key in local_obj.keys_dict):
        find_target = True

        value = local_obj.keys_dict[key]
        sys.stdout.write("Found data \"{}\" for key {}\n".format(value, key))
    else:
        S = local_obj.get_k_closest(remote_id = key)
        S_prime = [local_obj.node]

        while (len(S) != 0):
            temp_node = S.pop(0)
   
            # if contacted, skip this one
            if (temp_node in S_prime):
                continue

            S_prime.append(temp_node)

            channel = grpc.insecure_channel('{}:{}'.format(temp_node.address, temp_node.port))
            stub = csci4220_hw4_pb2_grpc.KadImplStub(channel)
            idkey = csci4220_hw4_pb2.IDKey( node  = local_obj.node,	idkey = key )
            kv_node = stub.FindValue(idkey)

            # if mode_kv is true, then only read from kv
            local_obj.Add_node(kv_node.responding_node)
            if (kv_node.mode_kv):
                find_target = True
                
                break
            else:
                for temp_r in kv_node.nodes:
                    S.append(temp_r)
                    local_obj.Add_node(temp_r)

        # If found target key at another node, the program should then print: Found value "<value>" for key <key>
        if (find_target):
            sys.stdout.write("Found value \"{}\" for key {}\n".format(kv_node.kv.value, key))
        # Otherwise the program should print: Could not find key <key>
        else:
            sys.stdout.write("Could not find key {}\n".format(key))

    sys.stdout.write("After FIND_VALUE command, k-buckets are:\n")
    sys.stdout.flush()
    local_obj.print_buckets()

def STORE(key, value, local_obj):
    '''
    The node should send a Store RPC to the single node that has ID closest to the key.
    '''
    # get closest distance & closest node
    closest_dist = XOR_dist(local_obj.node.id, key)
    closest_node = local_obj.node
    for i in range(N):
        for temp_node in local_obj.buckets[i]:
            temp_dist = XOR_dist(temp_node.id, key)
            if (temp_dist < closest_dist):
                closest_dist = temp_dist
                closest_node = temp_node
    
    # When calling the Store RPC, the requester should print: Storing key <key> at node <remoteID>
    sys.stdout.write("Storing key {} at node {}\n".format(key, closest_node.id))
    sys.stdout.flush()

    keyValue = csci4220_hw4_pb2.KeyValue(node = local_obj.node, key = key, value = value)
    if (closest_node == local_obj.node):
         # current node is the closest node and need to store the key/value pair locally.
        local_obj.Store(keyValue, None)
    else:
        channel = grpc.insecure_channel('{}:{}'.format(closest_node.address, closest_node.port))
        stub = csci4220_hw4_pb2_grpc.KadImplStub(channel)   
        stub.Store(keyValue)

def QUIT(local_obj):
    '''
    quitting a node, letting every node in bucket know that I'm quitting
    '''
    for i in range(N):
        for temp_node in local_obj.buckets[i]:
            sys.stdout.write("Letting {} know I'm quitting.\n".format(temp_node.id))
            sys.stdout.flush()

            try:
                channel = grpc.insecure_channel('{}:{}'.format(temp_node.address, temp_node.port))
                stub = csci4220_hw4_pb2_grpc.KadImplStub(channel)  
                Idkey = csci4220_hw4_pb2.IDKey(node  = local_obj.node, idkey = local_obj.node.id)
                stub.Quit(Idkey)
            except:
                continue

    sys.stdout.write("Shut down node {}\n".format(local_obj.node.id))
    sys.stdout.flush()

class KadImplServicer(csci4220_hw4_pb2_grpc.KadImplServicer):
    def __init__(self, host_addr, port, nodeID, k):
        self.node = csci4220_hw4_pb2.Node(id = nodeID, port = int(port), address=host_addr)
        self.buckets = {i: [] for i in range(N)}
        self.keys_dict = {}
        self.k = k

    def print_buckets(self):
        """
        print out buckets based on param @ buckets passed in

        :param buckts: dictionary with keys (int) & value (<ID>:<port>) passed from KadImplServicer class
        """ 
        for i in range(len(self.buckets)):
            sys.stdout.write("{}:".format(i))
            for entry in self.buckets[i]:
                sys.stdout.write(" {}:{}".format(entry.id, entry.port))
            sys.stdout.write("\n")
        sys.stdout.flush()

    def get_hash(self, remote_node):
        '''
        get the hash value based on local node and remote node
        return the bucket index current remote node are suppose to locate
        '''
        remote_dist = XOR_dist(self.node.id, remote_node.id)

        for i in range(N):
            lower = 2 ** i
            upper = 2 ** (i+1)
            if (lower <= remote_dist and remote_dist < upper):
                return i

    def get_k_closest(self, remote_node = None, remote_id = None):
        '''
        get k closest nodes from current buckets to remote node, nodes including self.node
        
        :param remote_note: a node used to calculate distance to find the the k closest nodes
        '''
        dist_list = []
        remote_id = remote_node.id if remote_id == None else remote_id
        for keys in self.buckets:
            for value in self.buckets[keys]:
                temp_dist = XOR_dist(remote_id, value.id)
                dist_list.append((temp_dist, value))
        
        dist_list.sort(key=lambda x:x[0])
        return [node for (_, node) in dist_list[slice(self.k + 1)]]

    def Add_node(self, remote_node):
        '''
        add a node to current buckets list based on the distance for remote node and current node
        '''
        i = self.get_hash(remote_node)

        if (i == None):
            '''
            not appending node itself, i.e., if distance is 0
            '''
            return 
        
        if (remote_node in self.buckets[i]):
            self.buckets[i].remove(remote_node)
        self.buckets[i].append(remote_node)

        if (len(self.buckets[i]) > self.k):
            self.buckets[i].pop(0)
        
    def remove_node(self, remote_node):
        '''
        remove a node from current buckets list based on the distance for remote node and current node
        '''
        i = self.get_hash(remote_node)
        if (i == None):
            # not removing node itself, i.e., if distance is 0
            return 
        
        if (remote_node in self.buckets[i]):
            self.buckets[i].remove(remote_node)
        
    def FindNode(self, request, context):
        '''
        find a remote node, and has the side effect of updating the current node's k-buckets.
        '''
        sys.stdout.write("Serving FindNode({}) request for {}\n".format(request.idkey, request.node.id))
        sys.stdout.flush()

        k_closest = []
        if (request.idkey != self.node.id):
            self.Add_node(request.node)
        k_closest = self.get_k_closest(request.node)
        
        return csci4220_hw4_pb2.NodeList(responding_node = self.node, nodes = k_closest)

    def FindValue(self, request, context):
        sys.stdout.write("Serving FindKey({}) request for {}\n".format(request.idkey, request.node.id))

        if (request.idkey != self.node.id):
            self.Add_node(request.node)

        # If the remote node has stored the key, it responds with the key and the associated value.
        if (request.idkey in self.keys_dict):
            value = self.keys_dict[request.idkey]
            return csci4220_hw4_pb2.KV_Node_Wrapper(
                responding_node = self.node,
                mode_kv = True,
                kv = csci4220_hw4_pb2.KeyValue(
                    node = self.node,
                    key = request.idkey,
					value = value
                )
            )
        # If the remote node not stored the key, reply with the k closest nodes to the key.
        else:
            k_closest = self.get_k_closest(remote_id=request.idkey)
            return csci4220_hw4_pb2.KV_Node_Wrapper(
                responding_node = self.node, 
                mode_kv = False,                                     
				nodes = k_closest
            )

    def Store(self, request, context):
        '''
        When receiving a store RPC the node receiving the call should locally store the key/value pair. 
        It also update its own k-buckets by adding/updating the requester's ID to be the most recently used.
        '''
        # If a node receives a call to Store, it should print: Storing key <key> value "<value>"
        if (request.node != self.node):
            sys.stdout.write("Storing key {} value \"{}\"\n".format(request.key, request.value))
        
        # store locally
        self.keys_dict[request.key] = request.value

        # update k-bucket list
        self.Add_node(request.node)

        # just for keeping consistant with protocol file
        idkey = csci4220_hw4_pb2.IDKey(node = request.node, idkey = request.key)
        return idkey
        
    def Quit(self, request, context):
        i = self.get_hash(request.node)
        if (i is None):
            sys.stdout.write("No record of quitting node {} in k-buckets.\n".format(request.idkey))
            return
        
        sys.stdout.write("Evicting quitting node {} from bucket {}\n".format(request.idkey, i))
        self.remove_node(request.node)

        # does nothing, just added it here so that it satisfies the protocol
        return request

def server(host_addr, port, nodeID, k):
    local_obj = KadImplServicer(host_addr, port, nodeID, k)
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    csci4220_hw4_pb2_grpc.add_KadImplServicer_to_server(
       local_obj , server
    )
    server.add_insecure_port("{}:{}".format(host_addr, port))
    server.start()

    for line in sys.stdin:
        command_list = line.replace('\n','').split(' ')
        if ((command_list[0]).upper() == "BOOTSTRAP"):
            if (len(command_list) != 3):
                sys.stdout.write("ERROR: Wrong Input( BOOTSTRAP <remote hostname> <remote port> )\n")
                sys.stdout.flush()
                continue
            BOOTSTRAP(command_list[1], int(command_list[2]), local_obj)

        elif ((command_list[0]).upper() == "FIND_NODE"):
            if (len(command_list) != 2):
                sys.stdout.write("ERROR: Wrong Input( FIND_NODE <nodeID> )\n")
                sys.stdout.flush()
                continue
            FIND_NODE(int(command_list[1]), local_obj)

        elif ((command_list[0]).upper() == "FIND_VALUE"):
            if (len(command_list) != 2):
                sys.stdout.write("ERROR: Wrong Input( FIND_VALUE <key> )\n")
                sys.stdout.flush()
                continue
            FIND_VALUE(int(command_list[1]), local_obj)

        elif ((command_list[0]).upper() == "STORE"):
            if (len(command_list) != 3):
                sys.stdout.write("ERROR: Wrong Input( STORE <key> <value> )\n")
                sys.stdout.flush()
                continue
            STORE(int(command_list[1]), command_list[2], local_obj)
        
        elif ((command_list[0]).upper() == "QUIT"):
            QUIT(local_obj)
            break
        else:
            sys.stdout.write("ERROR: Wrong Input\n")
            sys.stdout.flush()
            continue

def run():
    if (len(sys.argv) != 4):
        sys.stdout.write("Input error: hw4.py <nodeID> <port> <k>")
        sys.stdout.flush()
        exit(1)

    local_nodeID = int(sys.argv[1])
    local_port = int(sys.argv[2])
    k = int(sys.argv[3])

    hostname = socket.gethostname() # Gets my host name
    local_host_addr = socket.gethostbyname(hostname) # Gets my IP address from my hostname   
    
    server(local_host_addr, local_port, local_nodeID, k)

if __name__ == "__main__":
    logging.basicConfig()
    run()
    
    

    