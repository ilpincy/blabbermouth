# BlabberMouth

BlabberMouth is a software hub for various types of connections. The
typical use case is a set of devices that must be communicate to each
other. BlabberMouth connects to each device, and when a device sends
data to BlabberMouth, the latter sends to it to all the other devices.

# Usage

    ./blabbermouth <-s SIZE> [-f FILE]... [STREAM]...
    ./blabbermouth scan
data repeater on various types of connections.

# operational modes

BlabbermMuth has two operational modes: streaming and scanning.

## Streaming

In streaming mode, BlabberMouth connects to each `STREAM` passed as
command line parameter and/or in `FILE`. Every time a message is sent
by one of the peers over a stream, BlabberMouth collects the data and
sends it over the other streams.

Each message managed by BlabberMouth must be exactly `SIZE` bytes
long, so the `-s` option is required.

The syntax for stream descriptors is: `ID:TYPE:VERBOSE:DATA`, where
`ID` is a unique identifier for the stream; `TYPE` is a case-sensitive
string such as `tcp`, `udp`, `bt`, or `xbee`; `VERBOSE` is a flag
(`0`/`1`) to establish whether BlabberMouth should log when
sending/receiving messages on the console; and `DATA` is a
colon-separated string of fields that specify how to connect to the
stream.

Supported stream descriptors:

    ID:tcp:VERBOSE:SERVER:PORT   A TCP connection to SERVER on PORT
    ID:udp:VERBOSE:SERVER:PORT   A UDP connection to SERVER on PORT
    ID:bt:VERBOSE:rfcomm:CHANNEL An RFComm Bluetooth connection on CHANNEL

Options:

    -s SIZE | --size SIZE   The size (in bytes) of a message
    -f FILE | --file FILE   A file containing one stream descriptor per line

## Scanning

In scanning mode, Blabbermouth looks for Bluetooth devices to connect to and
prints a list of available devices. BlueZ must be installed for Bluetooth to be
supported.

# Testing

To make sure BlabberMouth works, you could try the following tests.

## TCP connection tests

Open three terminals. In the first write:

    nc -l 12345

In the second write:

    nc -l 12346
    
In the third write:

    ./blabbermouth -s 5 1:tcp:1:localhost:12345 2:tcp:1:localhost:12346

Now type a five-character string in either of the first two terminals,
and press enter. The other terminal where `nc` is running should show
what was typed in the other terminal.

## UDP connection tests

Open three terminals. In the first write:

    nc -ul 12345

In the second write:

    nc -ul 12346
    
In the third write:

    ./blabbermouth -s 5 1:udp:1:localhost:12345 2:udp:1:localhost:12346

Now type a five-character string in either of the first two terminals,
and press enter. The other terminal where `nc` is running should show
what was typed in the other terminal.

## TCP/UDP connection tests

Open three terminals. In the first write:

    nc -l 12345

In the second write:

    nc -ul 12346
    
In the third write:

    ./blabbermouth -s 5 1:tcp:1:localhost:12345 2:udp:1:localhost:12346

Now type a five-character string in either of the first two terminals,
and press enter. The other terminal where `nc` is running should show
what was typed in the other terminal.
