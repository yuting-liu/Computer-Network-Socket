# Learning Switch

1. This file should be implemented with Switchyard environment, see the readme.md in the root folder to find the link to Switchyard.

2. Learning switch provides the core logic of Ethernet switch. The switch could learn from the incoming packets about their source host's MAC address, so that next time if another packet towards the previous sending host comes, it could send the packet directly out from the specific port, instead of flooding it all over the local network. Also, it has to set up a TTL for each entry in order to delete the outdated entries and save some memory.

3. Sounds like a quite simple assignment, isn't it? :)

