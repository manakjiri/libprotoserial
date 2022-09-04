# Ports

Unlike with TCP or UDP protocols, where ports are associated with one interface, in libprotoserial we handle ports more like service endpoints that can be device-wide. Ports are managed by the ports_handler class, which serves as node through which data of all registered interfaces and services flows. 

