# SoftwareDefinedRouting

Implemented a simplified version of the Distance Vector routing protocol over simulated routers.

Implemented Distance Vector Algorithm using Bellman Ford equation on 5 servers to find shortest path among them using UDP Protocol.

Implemented File transfer from one server to another using TCP Protocol.

Implemented Control and Data Plane Dual Functionality

When launched, this application will listen for control messages on the control port. The very first control message received (of type INIT) will actually initialize the network. Apart from the list of neighbors and link-costs to them, it will contain the router and data port numbers. After the receipt of this initialization message, the application starts listening for routing updates/messages on the router port and data packets on the data port. From this point onwards, the application will listen for messages/connections on the router (UDP), control (TCP), and data (TCP) ports simultaneously (using the select() system call) and respond to those messages.
