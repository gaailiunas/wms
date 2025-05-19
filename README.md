# WMS
A minimal, peer-to-peer warehouse management system that works offline over a local network. Each device maintains its own local copy of the inventory database and synchronizes with other peers over LAN.

# Design
## When a New Device Joins
1. A new device connects to the network.
2. This device sends a "HELLO" message to everyone using UDP multicast.
3. The "HELLO" message contains the new device's IP address.

## How Existing Devices Respond
1. All devices on the network receive the "HELLO" message.
2. Each device adds the new device's IP to its list of known peers.
3. Each existing device connects directly to the new device using TCP.
4. They send a "GOSSIP" message back to the new device.
5. This message contains up to 5 randomly selected peers they know about.

## How the New Device Builds Its Network View
1. The new device collects all responses from existing devices.
2. It combines all the peer information it receives.
3. From this combined list, it randomly selects up to 5 peers.
4. These selected peers become its initial peer list.
5. The new device now has a starting point to communicate with the network.

## Regular Network Maintenance
1. Every 10 seconds, each device connects to its known peers.
2. It asks each peer for their current peer lists.
3. When a device receives peer lists from others, it updates its own list.
4. For peers already in the list, it updates the "last_seen" timestamp.
5. For new peers not in the list, it adds them if there's space available.
6. If the list is full, it replaces the oldest peer (the one with oldest "last_seen" time).
7. If a peer hasn't been seen for too long (40-50 second timeout), it's marked as "stale" and can be removed.

## Material
- https://en.wikipedia.org/wiki/Gossip_protocol

