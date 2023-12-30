#!/usr/bin/env python3
import sys 
import socket
import selectors
import linecache
import math
reachable={}
if len(sys.argv)!= 7:
    print("Wrong Number of Arguments")
    sys.exit(-1)
host=sys.argv[1]
port= int(sys.argv[2])
id=sys.argv[3]
srange=int(sys.argv[4])
xpos=int(sys.argv[5])
ypos=int(sys.argv[6])
#parsing arguments 
def Process_Command(sock,mask,sock2): 
    global reachable
    global id
    Data=sock2.recv(1024)
    Data=Data.decode().split()
    #recieving and processing input from server
    if(len(Data)==0):
        selector.unregister(sock)
        sock.close()
        return
    #Just to help with the server disconecting before the clients are given the quit command
    if Data[0]=="REACHABLE":
        reachable.clear()
        for i in range(2,len(Data),3):
            reachable[Data[i]]=(int(Data[i+1]),int(Data[i+2]))
        message="{}: After reading REACHABLE message, I can see: {}".format(id,list(reachable.keys()))
        print(message)
        return reachable
    #Parsing of the reachable packet 
    if Data[0]=="THERE":
        return (Data[1], int(Data[2]), int(Data[3]))
    #Parsing of the there packet
    if  Data[0]=="DATAMESSAGE":
        Dest= Data[3]
        Current_Place= Data[2]
        Origin=Data[1]
        Data[4]=int(Data[4])
        if id == Dest:
            message="{}: Message from {} to {} successfully received.".format(Current_Place, Origin, Current_Place)
            print(message)
            return
        #output the right message if the packet has reached it destination
        sock2.send(genUpdate().encode())
        reachable=Process_Command(sock, mask, sock2)
        #send an updat command and wait for response
        where="WHERE "+ Dest
        sock.send(where.encode())
        id2,xpos2,ypos2=Process_Command(sock, mask, sock)
        #send where command and wait for responce
        getpos=lambda node: math.sqrt((reachable[node][0]-xpos2)**2 + (reachable[node][1]-ypos2)**2)
        Reach=set(reachable.keys())
        HopSet=set(Data[5::])
        Option=list(Reach-HopSet)
        Option.sort(key=getpos)
        #calculate next place
        if len(Option)==0:
            message="{}: Message from {} to {} could not be delivered".format(Current_Place,Origin,Dest)
            print(message)
            return
        #if there is none output the right message 
        else:
            Forward=Option[0]
            message="{}: Message from {} to {} being forwarded through {}".format(id,Origin,Dest,id)
            print(message)
            Data.append(Current_Place)
            Data[4]+=1
            Data[2]=Forward
            for i in range(len(Data)):
                Data[i]=str(Data[i])
            sock.send(" ".join(Data).encode())
        #otherwise keep the packet moving 
def Process_STDIN(stdin,mask,sock):
    global KeepRunning
    global id
    global xpos 
    global ypos 
    global reachable
    #python way to refrence global variables
    Data=stdin.readline().strip().split()
    #Reading the command from standered input
    if Data[0]=="QUIT":
       KeepRunning=False
       #Quit if its a quit command
    if Data[0]=="MOVE":
        xpos=int(Data[1])
        ypos=int(Data[2])
        sock.send(genUpdate().encode())
        Process_Command(sock, mask, sock)
        #When a move command is entered we send an update command to the serve and await the results
    if Data[0]=="WHERE":
        sock.send(" ".join(Data).encode())
        Process_Command(sock, mask, sock)
        #When a where command is entered we send it to the server and wait
    if Data[0]=="SENDDATA":
        where="WHERE "+Data[1]
        sock.send(where.encode())
        id2,xpos2,ypos2=Process_Command(sock, mask, sock)
        getpos=lambda node: math.sqrt((reachable[node][0]-xpos2)**2 + (reachable[node][1]-ypos2)**2)
        sock.send(genUpdate().encode())
        Process_Command(sock, mask, sock) 
        #Sending the where and positions update before generating the data message
        Reach=list(reachable.keys())
        Reach.sort(key=getpos)
        #Sorting the list by distance with lexograph as the tie breaker
        if len(Reach)==0:
            message="{}: Message from {} to {} could not be delivered.".format(id,id,id2)
            print(message)
            return
        #if we cant send it anywere than we output the proper message
        message="DATAMESSAGE {} {} {} {} {}".format(id,Reach[0],id2,1,id)
        sock.send(message.encode())
        #Sending the data message to the right place
        if getpos(Reach[0])==0:
            output="{}: Sent a new message directly to {}.".format(id,id2)
            print(output)
        else:
            output="{}: Sent a new message bound for {}.".format(id,id2)
            print(output) 
        #Printing the right message depending on how many steps are left
def genUpdate():
    global id
    global srange
    global xpos
    global ypos
    initupdate="UPDATEPOSITION {} {} {} {}".format(id,srange,xpos,ypos)
    return initupdate
    #Just a function to generate update commands

KeepRunning=True
selector= selectors.DefaultSelector()
#initialize the slectors and the variable that keeps the code running 
if __name__ == '__main__': 
    remote_addr= socket.gethostbyaddr(host)
    my_hostname = socket.gethostname() # Gets my host name
    my_address = socket.gethostbyname(my_hostname) # Gets my IP address from my hostname
    InitSocket=socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    InitSocket.connect((remote_addr[0],port))
    selector.register(InitSocket, selectors.EVENT_READ, Process_Command)
    selector.register(sys.stdin, selectors.EVENT_READ, Process_STDIN)
    InitSocket.send(genUpdate().encode())
    #sending initial update and getting the sockets connected and registering them with select
    while KeepRunning:
        for key,mask in selector.select():
            callback=key.data
            callback(key.fileobj,mask,InitSocket)
    InitSocket.close()
    #Clean up after quit
    

