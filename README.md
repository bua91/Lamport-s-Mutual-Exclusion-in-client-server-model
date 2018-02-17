# Lamport-s-Mutual-Exclusion-in-client-server-model
# ===========================================================================
# Its a distributed system following a client-server architecture. Here
# each file is replicated on all servers. READ operation from client is
# targetted to a particular server for a particular file, and the WRITE 
# operation on a perticular server file involves updating all replicas
# on each server for consistency.
# Implemented Lamport's mutual exclusion algorithm among clients so that
# no READ?WRITE violation can occur.
