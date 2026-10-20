# Deadlands interaction prototype

This branch introduces a reusable C++ interaction interface and a replicated light switch.

The player character will perform a short camera trace on F and send an interaction request to the authoritative server. The server independently validates target, range, and line of sight. The switch's replicated bIsOn property drives its point light on every client.

This prototype does not implement persistence yet. Test replication before adding save/load.

After compiling, place a Blueprint subclass of DeadlandsLightSwitch in L_TestLab and assign a visible static mesh to SwitchMesh. The light is a child component of the switch actor.
