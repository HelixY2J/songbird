# songbird

A network protocol that enables communication between two programs.The Nightswatch application serves as a TCP server,using inotify to monitor file modifications.Song acts as a TCP client,establishing a connection with Nightswatch to receive notifications about file modifications

## How it works

Before the communication begin,both applications agree on the structure defined by the songbird protocol by specifying fields i.e. actions, status, data

The file watcher application,nightswatch, utilizes inotify to monitor the files for modifications. Upon detecting file modifications, it sends a message to the user facing application 'song' by serializing and trasmitting over the network

And when song recieves the message from nightswatch ,it deserializes to extract the information, interprets it based on its action and option fields.So if the action indicates a file deletion, song processes the messagee and displays a desktop notification to the user


## Installation

Clone the songbird repository

```bash
git clone https://github.com/HelixY2J/songbird.git
```

Compile the core source code 

```bash
cd core
make
```
Compile the nightswatch source code 

```bash
cd nightswatch
make
```
## Usage

To start nightwatch,run the commnad

```bash
./build/nightswatchh
```

And then to start song in different terminal

```bash
./build/songg 127.0.0.1 nightswatch.c
```

Song will establish a TCP connection with Nightswatch and begin receiving file modification notifications.

And then fire up another terminal,make some modification in a file to recieve the notifications

### Dependencies

`libnotify`: A library for sending desktop notifications.

We can either build with gcc -o songg `pkg-config --cflags --libs libnotify` song.c

For more information please check https://wiki.archlinux.org/title/Desktop_notifications

If you get error related to <glib.h> then we need to install *-dev packages.

```bash
 sudo apt-get install libnotify-dev
```

Still if error persists we might need to manually specify the information about glib2.0 in our makefile. Run following command -

```bash
 pkg-config --cflags --libs glib-2.0
```

Include the output in makefile.
