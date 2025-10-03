# CToggler
Simple and minimal single-file systemd daemon that switches between wireless connection (with IWD) and wired.

# How to build:

To build this program, you have to install package with systemd libs (for Ubuntu it may be called "libsystemd-dev") and compiler.

To start building, you can just use this command:
<pre> 
  clang++ -lsystemd -O3 daemon.cpp -o ctoggler
</pre>
Or this:
<pre>
  g++ -lsystemd -O3 daemon.cpp -o ctoggler
</pre>

# How to install:

First you need to install some dependencies: dhcpcd and iwd. Then you have to move some files: ctoggler.service to /usr/lib/systemd/system, ctoggler executable to /usr/bin, ctoggler.conf to /etc.

# How to use:

Install the app and configure it. Write to /etc/ctoggler.conf:
<pre>
eth=Your wired interface name
wifi=Your wireless interface name
</pre>
Then enable the daemon:
<pre>
  systemctl enable --now  ctoggler.service
</pre>
