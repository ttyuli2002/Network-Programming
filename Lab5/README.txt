108.177.112.106  

TCP max seggment size before connecting 536
TCP recieve buffer size before connecting 60
TCP max seggment size before connecting 1412
TCP recieve buffer size before connecting 60

The reason that the recieve buffer size did not change is because the ip we are connecting to never made a change to that option on its socket so we are basing it of the default window size.

The reason that the MSS changes is because once the connection is established we need to make sure that we are under the maximum MSS for each router along our path.


128.113.0.2

TCP max seggment size before connecting 536
TCP recieve buffer size before connecting 60
TCP max seggment size before connecting 1460
TCP recieve buffer size before connecting 60

The reason that the recieve buffer size did not change is because the ip we are connecting to never made a change to that option on its socket so we are basing it of the default window size.

The reason that the MSS changes is because once the connection is established we need to make sure that we are under the maximum MSS for each router along our path.

127.0.0.1

TCP max seggment size before connecting 536
TCP recieve buffer size before connecting 60
TCP max seggment size before connecting 32747
TCP recieve buffer size before connecting 60


The reason that the recieve buffer size did not change is because the ip we are connecting to never made a change to that option on its socket so we are basing it of the default window size.

The reason that the MSS changes is because once the connection is established we need to make sure that we are under the maximum MSS for each router along our path.

Genral analysis 

The reason that recieve buffer size stays the same in all the IP adresses that we test is because neither party is altering that socket property so it just uses the default.

The reason that the MSS changes is because the path that the packets are taking is changin which means that the maximum MSS is changing and all parties have to agree to stay under the maximum MSS.
