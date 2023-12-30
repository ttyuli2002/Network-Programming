#!/usr/bin/env python3
import sys
import socket
import linecache
import selectors
import math
from queue import PriorityQueue

class Station:
    def __init__(self,BaseId,XPos,YPos, NumLink):
        self.BaseID=BaseId
        self.XPos=XPos
        self.YPos=YPos
        self.NumLink=NumLink
        self.LinkDict={}
    #constructor for station
    def getConnectNumber(self):
        return self.NumLink
    #gets the number of nodes connected 
    def getBaseID(self):
        return str(self.BaseID)
    #gets the base id 
    def addLink(self,station):
        self.NumLink+=1
        self.LinkDict[station.getBaseID()]=station
    #adds a link
    def setX(self,X):
        self.XPos=X
    #lets the x position be set
    def setY(self,Y):
        self.YPos=Y
    #lets the y position be set
    def isSensor(self):
        return False
    #tells that it its not a sensor
    def getX(self):
        return self.XPos
    #returns x
    def getY(self):
        return self.YPos
    #returns y
class Sensor(Station):
    def __init__(self,BaseId,XPos,YPos, NumLink, sock):
        self.BaseID=BaseId
        self.XPos=XPos
        self.YPos=YPos
        self.NumLink=NumLink
        self.LinkDict={}
        self.Sock=sock
        self.reach=0
        #constructor for sensor
    def setReach(self,val):
        self.reach=val
        #lets the reach be set 
    def getBaseID(self):
        return str(self.BaseID)
        #returns the base id
    def getReach(self):
        return self.reach
        #returns the reach
    def isSensor(self):
        return True
        #returns that its a senso
    def clearConnenction(self): 
        self.LinkDict={}
        #clear the connections
class Graph:
    def __init__(self):
        self.Dictonary={}
        #construcs the graph
    def addNode(self,Node):
        self.Dictonary[Node]=self.Dictonary.get(Node,Station(Node, 0, 0, 0))
        return self.Dictonary[Node]
        #adds a node if it dosent exsist and returns it 
    def addSensor(self,sensor,Sock):
        self.Dictonary[sensor]=self.Dictonary.get(Node,Sensor(sensor, 0, 0, 0,Sock))
        return self.Dictonary[sensor]
        #adds a sensor if it dosent exsist at returns it 
    def get(self,Node):
        return self.Dictonary[Node]
        #gets a node
    def getReachable(self,Sensor):
        sensor=self.Dictonary[Sensor]
        reachable=[i.getBaseID()+" "+ str(i.getX())+ " "+ str(i.getY()) for i in self.Dictonary.values() if  math.sqrt((sensor.getX()-i.getX())**2 + (sensor.getY()- i.getY())**2)<=sensor.getReach() and i.getBaseID() != Sensor] 
        #makes an array of reachable nodes using distance formula
        for i in sensor.LinkDict.values():
            del i.LinkDict[Sensor]
        #clears the sensor from all links if it has any
        sensor.LinkDict=dict([(i.getBaseID(),i) for i in self.Dictonary.values() if  math.sqrt((sensor.getX()-i.getX())**2 + (sensor.getY()- i.getY())**2)<=sensor.getReach() and i.getBaseID() != Sensor]) 
        #updates the sensors link dictionary
        for i in self.Dictonary.values():
            if  math.sqrt((sensor.getX()-i.getX())**2 + (sensor.getY()- i.getY())**2)<=sensor.getReach() and i.getBaseID() != Sensor:
                i.addLink(sensor)
            #Adds links to each reachable node
        return reachable
    def getNode(self,Node):
        return self.Dictonary[Node]
    #returns the node
    def send(self,DATAMESSAGE):
        HopSet=set(DATAMESSAGE[5::])
        Current_Place=set([i.getBaseID() for i in self.Dictonary[DATAMESSAGE[2]].LinkDict.values()]) 
        Option=list(Current_Place-HopSet)
        Dest= DATAMESSAGE[3]
        Current_Place=DATAMESSAGE[2]
        #parsing data packet
        tmp=self.getNode(Dest)
        #gets the node
        xpos2,ypos2=tmp.getX(),tmp.getY()
        getpos=lambda node: (math.sqrt((self.Dictonary[node].getX()-xpos2)**2 + (self.Dictonary[node].getY()-ypos2)**2),node)
        Option.sort(key=getpos)
        Origin= DATAMESSAGE[1]
        #Set some helper variables 
        if DATAMESSAGE[4]==len(self.Dictonary.keys()) or len(Option)==0:
            message="{}: Message from {} to {} being forwarded through {}".format(Current_Place,Origin,Dest,Current_Place)
            print(message)
            message="{}: Message from {} to {} could not be delivered.".format(Current_Place,Origin,Dest)
            print(str(message))
            return
        #outputs message if delivery is impossible 
        Forward=Option[0]
        if Current_Place == Dest:
            tmp=self.getNode(Dest)
            if(tmp.isSensor()):
                for i in range(len(DATAMESSAGE)):
                    DATAMESSAGE[i]=str(DATAMESSAGE[i])
                tmp.Sock.send(" ".join(DATAMESSAGE).encode())
            else:
                message="{}: Message from {} to {} successfully received.".format(Current_Place, Origin, Current_Place)
                print(message)
        #Delivers message and prints the right output 
        else:
            message="{}: Message from {} to {} being forwarded through {}".format(Current_Place,Origin,Dest,Current_Place)
            tmp=self.getNode(Forward)
            tmp2=self.getNode(Current_Place)
            DATAMESSAGE.append(Current_Place)
            DATAMESSAGE[4]+=1
            DATAMESSAGE[2]=Forward
            if not tmp2.isSensor():
                print(message)
            if tmp.isSensor():
                for i in range(len(DATAMESSAGE)):
                    DATAMESSAGE[i]=str(DATAMESSAGE[i])
                tmp.Sock.send(" ".join(DATAMESSAGE).encode())
            elif tmp2.isSensor():
                for i in range(len(DATAMESSAGE)):
                    DATAMESSAGE[i]=str(DATAMESSAGE[i])
                tmp2.Sock.send(" ".join(DATAMESSAGE).encode())
            else:
                self.send(DATAMESSAGE)
        #forwards the message and prints the output 

KeepRunning=True
graph=Graph()
selector= selectors.DefaultSelector()
#start the selector and the graph and the variable to keep the code running 
def ProcessSensor(sock,mask):
    Data=sock.recv(1024)
    if(len(Data)==0):
        selector.unregister(sock)
        sock.close()
        return
    #Clean up on disconnect
    Data=Data.decode().split()
    #Parse Data
    if Data[0]=="UPDATEPOSITION":
        sense=graph.addSensor(Data[1], sock)
        sense.setReach(int(Data[2]))
        sense.setX(int(Data[3]))
        sense.setY(int(Data[4]))
        reachable=graph.getReachable(Data[1])
        reachable.sort()
        sock.send((("REACHABLE " +str(len(reachable))+ " ")+ " ".join(reachable)).encode())
    #Deals with the update position command
    if Data[0]=="WHERE":
        Node=graph.get(Data[1])
        sock.send(("THERE "+ Node.getBaseID()+" "+ str(Node.getX()) + " "+ str(Node.getY())).encode())
    #Deals with the where command
    if Data[0]=="DATAMESSAGE":
        Data[4]=int(Data[4])
        graph.send(Data)
    #Deals with the data command

def accept(sock,mask):
    conn,addr=sock.accept()
    selector.register(conn, selectors.EVENT_READ, ProcessSensor)
    #deals with accepting new connections 
    
def processstdin(stdin,mask):
    global KeepRunning
    Data=stdin.readline().strip()
    if Data=="QUIT":
        KeepRunning=False
    #Looks for the quit command


if __name__== '__main__':
    if len(sys.argv)!= 3:
        print("Wrong Number of Arguments")
        sys.exit(-1)
    lineNum=1
    Line=linecache.getline(sys.argv[2], lineNum)
    #Reads the file line by line
    while(Line != ''):
        ProcessLine=Line.strip().split()
        Node=graph.addNode(ProcessLine[0])
        Node.setX(int(ProcessLine[1]))
        Node.setY(int(ProcessLine[2]))
        NodeList=ProcessLine[4::]
        for N in NodeList:
            TempNode=graph.addNode(N)
            Node.addLink(TempNode)
        lineNum+=1
        linecache.clearcache()
        Line=linecache.getline(sys.argv[2], lineNum)
        #adds line to the graph
    port=int(sys.argv[1])
    InitSocket=socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    InitSocket.bind((socket.gethostname(),port))
    InitSocket.listen()
    selector.register(InitSocket, selectors.EVENT_READ, accept)
    selector.register(sys.stdin, selectors.EVENT_READ, processstdin)
    #Starts the socket and registers it 
    while KeepRunning:
        for key,mask in selector.select():
            callback=key.data
            callback(key.fileobj,mask)
    #event loop
    for i in graph.Dictonary.values():
        if i.isSensor():
            i.Sock.close()
    InitSocket.close()
    #clean up




