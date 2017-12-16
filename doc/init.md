Sample init scripts and service configuration for xc3d
==========================================================

Sample scripts and configuration files for systemd, Upstart and OpenRC
can be found in the contrib/init folder.

    contrib/init/xc3d.service:    systemd service unit configuration
    contrib/init/xc3d.openrc:     OpenRC compatible SysV style init script
    contrib/init/xc3d.openrcconf: OpenRC conf.d file
    contrib/init/xc3d.conf:       Upstart service configuration file
    contrib/init/xc3d.init:       CentOS compatible SysV style init script

1. Service User
---------------------------------

All three startup configurations assume the existence of a "blocknetdx" user
and group.  They must be created before attempting to use these scripts.

2. Configuration
---------------------------------

At a bare minimum, xc3d requires that the rpcpassword setting be set
when running as a daemon.  If the configuration file does not exist or this
setting is not set, xc3d will shutdown promptly after startup.

This password does not have to be remembered or typed as it is mostly used
as a fixed token that xc3d and client programs read from the configuration
file, however it is recommended that a strong and secure password be used
as this password is security critical to securing the wallet should the
wallet be enabled.

If xc3d is run with "-daemon" flag, and no rpcpassword is set, it will
print a randomly generated suitable password to stderr.  You can also
generate one from the shell yourself like this:

bash -c 'tr -dc a-zA-Z0-9 < /dev/urandom | head -c32 && echo'

Once you have a password in hand, set rpcpassword= in /etc/xcurrency/xcurrency.conf

For an example configuration file that describes the configuration settings,
see contrib/debian/examples/xcurrency.conf.

3. Paths
---------------------------------

All three configurations assume several paths that might need to be adjusted.

Binary:              /usr/bin/xc3d
Configuration file:  /etc/xcurrency/xcurrency.conf
Data directory:      /var/lib/xc3d
PID file:            /var/run/xc3d/xc3d.pid (OpenRC and Upstart)
                     /var/lib/xc3d/xc3d.pid (systemd)

The configuration file, PID directory (if applicable) and data directory
should all be owned by the blocknetdx user and group.  It is advised for security
reasons to make the configuration file and data directory only readable by the
blocknetdx user and group.  Access to xc3-cli and other xc3d rpc clients
can then be controlled by group membership.

4. Installing Service Configuration
-----------------------------------

4a) systemd

Installing this .service file consists on just copying it to
/usr/lib/systemd/system directory, followed by the command
"systemctl daemon-reload" in order to update running systemd configuration.

To test, run "systemctl start xc3d" and to enable for system startup run
"systemctl enable xc3d"

4b) OpenRC

Rename xc3d.openrc to xc3d and drop it in /etc/init.d.  Double
check ownership and permissions and make it executable.  Test it with
"/etc/init.d/xc3d start" and configure it to run on startup with
"rc-update add xc3d"

4c) Upstart (for Debian/Ubuntu based distributions)

Drop xc3d.conf in /etc/init.  Test by running "service xc3d start"
it will automatically start on reboot.

NOTE: This script is incompatible with CentOS 5 and Amazon Linux 2014 as they
use old versions of Upstart and do not supply the start-stop-daemon uitility.

4d) CentOS

Copy xc3d.init to /etc/init.d/xc3d. Test by running "service xc3d start".

Using this script, you can adjust the path and flags to the xc3d program by
setting the BlocknetDXD and FLAGS environment variables in the file
/etc/sysconfig/xc3d. You can also use the DAEMONOPTS environment variable here.

5. Auto-respawn
-----------------------------------

Auto respawning is currently only configured for Upstart and systemd.
Reasonable defaults have been chosen but YMMV.
