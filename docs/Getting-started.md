Table Of Contents
- [Getting Started With Sphere server](#getting-started-with-sphere-server)
  - [Downloading the Sphere server](#downloading-the-sphere-server)
    - [\[Linux Only\] Download dependencies](#linux-only-download-dependencies)
    - [\[Linux Only\] Copy binary into root folder](#linux-only-copy-binary-into-root-folder)
  - [Client Installation](#client-installation)
  - [Server Configuration](#server-configuration)
    - [Acknowledge the usage of a nightly build](#acknowledge-the-usage-of-a-nightly-build)
    - [Enable automatic account creations](#enable-automatic-account-creations)
    - [MUL files](#mul-files)
      - [A. Do not change the Sphere.ini file](#a-do-not-change-the-sphereini-file)
      - [B. Specify your client's installation folder](#b-specify-your-clients-installation-folder)
      - [C. Copy client's files into the server's folder](#c-copy-clients-files-into-the-servers-folder)
    - [Configure maps](#configure-maps)
  - [Setup Scripts](#setup-scripts)
    - [Download scripts](#download-scripts)
    - [Tweak the spheretables.scp](#tweak-the-spheretablesscp)
  - [Start the server](#start-the-server)
  - [Connect to your server](#connect-to-your-server)
  - [Making yourself an admin](#making-yourself-an-admin)
  - [Populating the world](#populating-the-world)
    - [World decorations](#world-decorations)
    - [World spawner](#world-spawner)
  - [Next steps](#next-steps)

# Getting Started With Sphere server
This tutorial is directed to those that have no experience whatsoever with
the Sphere Server. The objective is to just get started with a fully functional
shard which, later on, can be tweaked and customised as needed.

## Downloading the Sphere server
The first obvious step is to download the Sphere server. 

Go to the [SphereX](https://forum.spherecommunity.net/sshare.php?srt=4) 
Download page and download the latest nightly version for your platform.

Extract the zip or tgz into a new folder.

### [Linux Only] Download dependencies
In order for the server to work correctly, you'll need to install mariadb
client. Different distros name this library in a different way so, have a quick
check online to understand how to install it on your distro. For instance,
if you're using an Ubuntu-based system, you can install it by running

```
sudo apt install mariadb-client
```

### [Linux Only] Copy binary into root folder
Once you've extracted your archive, you'll need to move the server's executable
into the root directory of the server. The file is contained into the 
`build-xx` folder.

For instance, for a 64 bits linux installation, your root folder's structure 
should go from this:
```
- accounts/
- build-64/
  - bin-x86_64
    - SphereSvrX64_nightly
- logs/
- save/
- scripts
- sphere.ini
- sphereCrypt.ini
```

To this
```
- accounts/    
- logs/
- save/
- scripts
- sphere.ini
- sphereCrypt.ini
- SphereSvrX64_nightly // executable in root
```

## Client Installation
Now it's time to pick a client. Take the original installation of your Ultima
Online client and install it. If you don't have it... Google is your friend ;).
For this tutorial we're going to use `Ultima Online Classic 7.0.2.0` client.

SphereX is compatible with any client, so feel free to pick any version but,
be careful: in order to have the same experience across every participant, 
make sure that user is running exactly the same client!

## Server Configuration
Now it's time make some tweaks to the default server configuration to accomodate
our needs. 

The whole server configuration is contained in the file named `sphere.ini`. 
You can open this file with any text editor - notepad should do the job. The
structure of the file is very simple: let's analyse a snippet to explain how 
it's organised line by line.
```
[SPHERE]
// Name of your Sphere shard
ServName=MyShard
```

`[SPHERE]`: this is a section of the configuration. It is used to group
properties together.

`// Name of your Sphere shard`: this is a comment. It gets ignored by the 
server. This is used to add additional infos to humans :)

`ServName=MyShard`: this is a property. This is used to configure specific
behaviours for the server. For instance, this is used to set the name of the
shard.

### Acknowledge the usage of a nightly build
Before you can start fiddling with different things, you'll need to agree of
using a nightly build. A nightly build gets created automatically every night 
based on the latest changes made by the developers. This means it might contain
potential bugs. With this settings you agree to take full responsibility of all
the implications of this choice.

So, let's open the `sphere.ini` file and add a new line to the `SPHERE`
section with the text `AGREE=1`. After the changes your file should look like
this.

```
...
[SPHERE]
// Add this line
AGREE=1
// Name of your Sphere shard
ServName=MyShard
...
```

**Remember** to save the file before you start the server, otherwise the changes 
will not be picked up by the server.

### Enable automatic account creations
The next step is to allow people to easily join your server upon creating a new
account upon a login attempt.

Moving down into the configuration file, you'll find a property called `AccApp`.
Set this value to `2`, to allow everyone to login and create a new account.

```
//Code for servers account application process
//  0=Closed,       // Closed. Not accepting more.
//  2=Free,         // Anyone can just log in and create a full account.
//  3=GuestAuto,    // You get to be a guest and are automatically sent email with u're new password.
//  4=GuestTrial,   // You get to be a guest til u're accepted for full by an Admin.
//  6=Unspecified,  // Not specified.
// To enable auto account you must set this to 2
AccApp=2
```

You can change this value later on as per comments above this property. As
usual, remember to save the file before moving on.

### MUL files
MUL files are called like this because their extensions is `.mul`. These files
are vital for the correct execution of the client and the server because they
contain all the assets of the game. This includes (but not limited to):
- maps;
- animations;
- armors;
- vests;
- decorations;
- ...

These files are contained into the Ultima Online client's installation.

**WARNING:** If there a discrepancy between server's muls and client's muls 
players will experience serious bugs or crashes.

For this reason we need to make sure that the server is picking up the mul
files from the Ultima Online client's installation. There are different ways
to achieve this.

#### A. Do not change the Sphere.ini file
If you're using Windows and you've got just one Ultima Online Client installed 
through the official EA installer, you can just skip this step and move on with
the next step of this guide.

#### B. Specify your client's installation folder
Open your `sphere.ini` file and find the `MulFile` property. Uncomment it 
(remove the leading `//`) and change its value to the location of you're 
client's installation folder. For instance:

```
// UO INSTALLATION  -  Note that if it's not set ( or commented ), sphere will scan windows registry to auto-detect it.
// If windows can't find the dir, then it must point to install's directory.
//
// For Old installations (client less than 7.0.20) this need: map0.mul, statics0.mul,
//  staidx0.mul, multi.mul, multi.idx, hues.mul, tiledata.mul.
// Optional files: verdata.mul, mapX.mul/staticsX.mul/staidxX.mul for higher
//  maps support (Malas, etc).
//
// For newer installs this need the same files in .uop format
//
// A full install folder is recommended as required files may change over time.
MulFiles=C:/Program Files/EA Games/Ultima Online/
```

#### C. Copy client's files into the server's folder
Create a new folder into the server's root folder. For instance you can call it
`mul`. 

For simplicity, copy all the contents of your Ultima Online client's 
folder into the folder you've just created.

Now open your `sphere.ini` file and locate the `MulFile` property. Uncomment it
(remove the leading `//`) and change its value to the name of the folder where
you've just created. For instance:

```
// UO INSTALLATION  -  Note that if it's not set ( or commented ), sphere will scan windows registry to auto-detect it.
// If windows can't find the dir, then it must point to install's directory.
//
// For Old installations (client less than 7.0.20) this need: map0.mul, statics0.mul,
//  staidx0.mul, multi.mul, multi.idx, hues.mul, tiledata.mul.
// Optional files: verdata.mul, mapX.mul/staticsX.mul/staidxX.mul for higher
//  maps support (Malas, etc).
//
// For newer installs this need the same files in .uop format
//
// A full install folder is recommended as required files may change over time.
MulFiles=mul/
```

**NOTE**: You don't need all the files contained into the Ultima Online client's
installation. You just need all the `.mul` and `.idx` files.

**NOTE 2**: On linux the name of the files is case sensitive so, make sure all
the file names are lowercase!

### Configure maps
New that we've imported all the maps we need to set each the map's boundaries:
this way the server will know what is the actual playable area. For the 
purpose of this guide, we'll just use the maps of Falucca and Trammel.

As usual, open your `sphere.ini` file and locate all the `MapX` properties.
Leave `Map0` (Felucca) and `Map1` (Trammel) as they are and add a leading `//`
to `Map2`, `Map3`, `Map4` and `Map5`.

This is how it should look like.
```
// Map of Felucca
//Map0=6144,4096,-1,0,0		//Old size
Map0=7168,4096,-1,0,0		//ML size

// Map of Trammel
//Map1=6144,4096,-1,1,1		//Old size
Map1=7168,4096,-1,1,1		//ML size

// Map of Ilshenar
//Map2=2304,1600,-1,2,2

// Map of Malas
//Map3=2560,2048,-1,3,3

// Map of Tokuno Islands
//Map4=1448,1448,-1,4,4

// Map of Ter Mur
//Map5=1280,4096,-1,5,5
```

## Setup Scripts
We've got a nice server. But it is completely brainless: it doesn't know what
is a vendor, an npc and what are their behaviours. In order to give it a brain
we should add some scripts. If we were to write all of them, it would take 
ages. For this reason Sphere comes with a pre-made set of scripts to get you
started in no time.

### Download scripts
Head out to the 
[SphereX Github page](https://github.com/Sphereserver/Scripts-X) click on `Code`
and hit `Download ZIP`. 

Unzip the content of the package into the server's `scripts` folder.

### Tweak the spheretables.scp
Now we need to tweak few settings according to our `sphere.ini` configuration.
Go ahead and open with your text editor the file `scripts/spheretables.scp`.

Let's comment out all the configurations related to all non Felucca and 
Trammel maps, respectively `map0` and `map1`.

Here is the before:
```
// maps
maps/map0/map0_starts.scp
maps/map0/map0_moongates.scp
maps/map0/map0_areas.scp
maps/map0/map0_areas_ml.scp
maps/map0/map0_areas_sa.scp
maps/map0/map0_areas_hs.scp
maps/map0/map0_rooms.scp
maps/map0/map0_teleports.scp
maps/map0/map0_teleports_ml.scp
maps/map0/map0_teleports_hs.scp
maps/map0/map0_teleports_tol.scp

maps/map1/map1_moongates.scp
maps/map1/map1_areas.scp
//maps/map1/map1_areas_aos_old_haven.scp
maps/map1/map1_areas_ml_new_haven.scp
maps/map1/map1_areas_ml.scp
maps/map1/map1_areas_sa.scp
maps/map1/map1_areas_hs.scp
maps/map1/map1_rooms.scp
maps/map1/map1_teleports.scp
maps/map1/map1_teleports_ml.scp
maps/map1/map1_teleports_ml_new_haven.scp
maps/map1/map1_teleports_hs.scp
maps/map1/map1_teleports_tol.scp

maps/map2/map2_areas.scp
maps/map2/map2_areas_ml.scp
maps/map2/map2_teleports.scp
maps/map2/map2_teleports_ml.scp
maps/map2/map2_moongates.scp

maps/map3/map3_areas.scp
maps/map3/map3_areas_ml.scp
maps/map3/map3_areas_se.scp
maps/map3/map3_teleports.scp
maps/map3/map3_teleports_ml.scp
maps/map3/map3_teleports_se.scp
maps/map3/map3_moongates.scp

maps/map4/map4_areas.scp
maps/map4/map4_teleports.scp
maps/map4/map4_moongates.scp

maps/map5/map5_areas.scp
maps/map5/map5_areas_tol.scp
maps/map5/map5_teleports.scp
maps/map5/map5_teleports_tol.scp
maps/map5/map5_moongates.scp
maps/map5/map5_moongates_tol.scp

```

and this is how the file should look like after our amendments:
```
// maps
maps/map0/map0_starts.scp
maps/map0/map0_moongates.scp
maps/map0/map0_areas.scp
maps/map0/map0_areas_ml.scp
maps/map0/map0_areas_sa.scp
maps/map0/map0_areas_hs.scp
maps/map0/map0_rooms.scp
maps/map0/map0_teleports.scp
maps/map0/map0_teleports_ml.scp
maps/map0/map0_teleports_hs.scp
maps/map0/map0_teleports_tol.scp

maps/map1/map1_moongates.scp
maps/map1/map1_areas.scp
//maps/map1/map1_areas_aos_old_haven.scp
maps/map1/map1_areas_ml_new_haven.scp
maps/map1/map1_areas_ml.scp
maps/map1/map1_areas_sa.scp
maps/map1/map1_areas_hs.scp
maps/map1/map1_rooms.scp
maps/map1/map1_teleports.scp
maps/map1/map1_teleports_ml.scp
maps/map1/map1_teleports_ml_new_haven.scp
maps/map1/map1_teleports_hs.scp
maps/map1/map1_teleports_tol.scp

//maps/map2/map2_areas.scp
//maps/map2/map2_areas_ml.scp
//maps/map2/map2_teleports.scp
//maps/map2/map2_teleports_ml.scp
//maps/map2/map2_moongates.scp

//maps/map3/map3_areas.scp
//maps/map3/map3_areas_ml.scp
//maps/map3/map3_areas_se.scp
//maps/map3/map3_teleports.scp
//maps/map3/map3_teleports_ml.scp
//maps/map3/map3_teleports_se.scp
//maps/map3/map3_moongates.scp

//maps/map4/map4_areas.scp
//maps/map4/map4_teleports.scp
//maps/map4/map4_moongates.scp

//maps/map5/map5_areas.scp
//maps/map5/map5_areas_tol.scp
//maps/map5/map5_teleports.scp
//maps/map5/map5_teleports_tol.scp
//maps/map5/map5_moongates.scp
//maps/map5/map5_moongates_tol.scp
```

Now you can save the file and close your editor.

## Start the server
It's now time to start the server by running `SphereSvrX64_nightly.exe`.

If everything's fine, should see on the console this confirmation message:

```
Startup complete (items=0, chars=0, Accounts = 0)
Use '?' to view available console commands
```

## Connect to your server
Now that you've configured the server, you're finally ready to start playing!

By default the Ultima Online Client will connect to the official game servers.
However, what we want to do is to connect to our server. There are multiple
tools you can use like Razor - just Google a bit ;). 

For the purpose of this guide we're going to use ClassicUO. So head out to
[their website](https://www.classicuo.eu/) and download the client for your 
platform. Extract it into a new folder on your desktop and open the 
`ClassicUOLauncher`.

Create a new profile and fill the fields as follows:
- Profile name: pick a name of your choice
- Server IP: `127.0.0.1` - provided you're running client and server on the
same machine
- Port: `2593`
- Use Encryption: enabled
- UO Path: the installation folder of your Ultima Online client (e.g. `C:/Program Files/EA Games/Ultima Online/`)

Now save your profile and hit back. On the main screen make sure that the 
profile you've just created is selected and hit Play. If you've done everything
correctly, the Ultima Online login screen should pop up. As per previous 
configuration you can now try to login with any username and password: this
operation will automatically create a new user for you.

So, pick a username and password and try to login. Create a new character et 
voila': you're the first player of your own personal Ultima Online Shard!

## Making yourself an admin
Well, since this is your server, I guess it's only fair to set yourself as an
admin of the server. This will allow you to do some special operations that
noone else can do like populating the world with new creatures, items, ban
players, broadcast messages, ...

Go ahead and open you server's window. If you're on Windows, you'll se that at
the bottom there a grey stripe on which you can type stuff. To promote yourself
as an admin you can just type

```
ACCOUNT <your_account_username> PLEVEL 7
```
And hit enter.

So, for instance, if your account name (**not player name!**) is `admin`, you
should just type
```
ACCOUNT admin PLEVEL 7
```
and hit enter.

To check whether it works, go back to your game and type in chat

```
.gm
```

This command is to transform yourself into a GM (Game Master). If you'll see
a message `GM ON` everything has worked out!

## Populating the world
I guess you've already noticed that Britain has never been so empty, right?

We've already got a solution for you to get started straight away!!!

### World decorations

Go into your game and type in the game's chat

```
.add i_world_decorator
```
And hit enter.

The cursor will tranform into a target. Place the item on the floor right next
to you and double-click it. A new window (aka "gump") will appear.

Under `Basic Items` hit:
- Generate Doors;
- Generate Signs;
- Generate Moongates;
- Generate Book;

Under Decoration hit:
- Felucca
- Trammel

remember? We have disabled all the other maps, so it's useless to try and
generate decorations for those maps.

Now that you're done, you can remove world decorator object. Just type on your
chat

```
.remove
```
The cursor will transform into a target. Choose the world decorator so make it
disappear from the game.

Once you've completed this step, go back to your server window and type

```
##
```
And hit enter. This will save both, the new account you've just created and
all the objects that have been placed in the game. It will take a few seconds,
so, be patient.

### World spawner
Last but not least, the world is sad without vendors, mobs, animals, mounts, ...
As usual: there is a solution for this as well.

Head out to your game window and type in the chat
```
.add i_world_spawner
```
Place the item right close to you and double-click it.

As before, hit Felucca and, in the submenu, click all the spawn items **once**.

Do the same thing for Trammel.

Same steps as before:
```
.remove
```
and click on the world spawner object.

Finally save all your changes by going back to the server window and issue the
```
##
```
command.

## Next steps
Congratulations: you've just created your fully functional shard from scratch!

However, this is just the beginning of your journey: if you want to know more
about Sphere server, access the 
[wiki page](https://wiki.spherecommunity.net/index.php?title=Main_Page)

If you need further help, get into the 
[Sphere discord server](https://discord.gg/ZrMTXrs).

Have fun!