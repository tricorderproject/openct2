SCALA Installation Instructions for Raspberry Pi 2
======

Note: These instructions have been adapted from Pavel Fatin's great instructions at: https://pavelfatin.com/install-scala-on-raspberry-pi/

These instructions assume using the Raspbian Jessie (2015-11-21) distribution. 

# Installing Java

Raspbian Jessie comes pre-installed with OpenJDK 7, but we'll need to replace this with the Oracle JDK 8 for the ARM architecture, which is much faster. 

To verify the currently installed version of Java, use: 

```
java -version
```

Now we have to download the Oracle ARM JDK version 8. On the Pi, open a web browser and head to http://www.oracle.com/technetwork/java/javase/downloads/jdk8-downloads-2133151.html .  The current version as of this writing is 8u65, with the archive name jdk-8u65-linux-arm32-vfp-hflt.tar.gz (about 70Mb).  The Oracle license agreement has to be accepted before you're able to download.  This typically downloads to ~/Downloads/ .

Unpack the archive to /usr/lib/jvm/ :
```
sudo tar -xf jdk-8u65-linux-arm32-vfp-hflt.tar.gz -C /usr/lib/jvm
```

We can now remove the archive, to free up some space:
```
rm jdk-8u65-linux-arm32-vfp-hflt.tar.gz 
```

The new Oracle ARM JDK should install to /usr/lib/jvm/jdk-8-oracle-arm-vfp-hflt , or something similar.  You can list the directory to verify this: 
```
ls /usr/lib/jvm/
```

Now we need to update which Java the system uses.  The following command should give a list of Java versions: 
```
sudo update-java-alternatives -l
```

One of the alternatives should be the new Oracle JDK 8 for ARM that we just installed:
> jdk-8-oracle-arm-vfp-hflt 318 /usr/lib/jvm/jdk-8-oracle-arm-vfp-hflt

We can select it using the following command:
```
sudo update-java-alternatives -s jdk-8-oracle-arm-vfp-hflt
```

To verify that it is the default java selected, we can again run the version command:
```
java -version
```

And the output should look something like this:
> java version "1.8.0"
>
> Java(TM) SE Runtime Environment (build 1.8.0-b132)
>
> Java HotSpot(TM) Client VM (build 25.0-b70, mixed mode)

# Installing Scala
The latest version of Scala can be downloaded from http://www.scala-lang.org/download/ .  The latest is currently 2.11.7, which we can directly download (about 27Mb):

```
wget https://downloads.typesafe.com/scala/2.11.7/scala-2.11.7.tgz
```

Then unpack the file:
```
sudo mkdir /usr/lib/scala
sudo tar -xf scala-2.11.7.tgz -C /usr/lib/scala
rm scala-2.11.7.tgz
```

Add symbolic links that point to the install location:
```
sudo ln -s /usr/lib/scala/scala-2.11.7/bin/scala /bin/scala
sudo ln -s /usr/lib/scala/scala-2.11.7/bin/scalac /bin/scalac
```

To verify the installation, check the version of Scala with: 
```
scala -version
```
It should read something similar to:
> Scala code runner version 2.11.7 -- Copyright 2002-2013, LAMP/EPFL

# Install SBT
The Scala Build Tool (SBT) helps handle compilation.  The latest version can be downloaded from http://www.scala-sbt.org/download.html . The latest as of this writing is 0.13.9 .

First we have to download the sbt-launch.jar file (about 1Mb):
```
wget https://repo.typesafe.com/typesafe/ivy-releases\
/org.scala-sbt/sbt-launch/0.13.9/sbt-launch.jar
```

Move it to a new location:
```
sudo mkdir /usr/lib/sbt
sudo mv sbt-launch.jar /usr/lib/sbt
```

Create a shell script to run the sbt launcher:
``` 
sudo nano /bin/sbt
```

And place the following contents into that file:
```
#!/bin/sh
java -server -Xmx512M -jar /usr/lib/sbt/sbt-launch.jar "$@"
```

Make the script executable:
```
sudo chmod +x /bin/sbt
```

On first launch, SBT will download it's dependencies, which will take some time.  We can begin this process by checking the version:
```
sbt --about
```

Where SBT will let us know that it's downloading:
> Getting org.scala-sbt sbt 0.13.9 ... 

Then eventually: 
> [info] This is sbt 0.13.9
>
> [info] No project is currently loaded
>
> [info] sbt, sbt plugins, and build definitions are using Scala 2.10.5
>
> [info] Set current project to downloads (in build file:/home/pi/Downloads/


Once this is complete, type ``exit`` to return to the console. 

# Install IntelliJ IDEA

IntelliJ is an excellent IDE for Java, with a Scala plugin available.  A list of recent versions can be found at: https://confluence.jetbrains.com/display/IntelliJIDEA/Previous+IntelliJ+IDEA+Releases .

Here we'll download a recent version (14.1.6), which is about 200Mb in size: 
```
wget https://download.jetbrains.com/idea/ideaIC-14.1.6.tar.gz
```

And extract the archive: 
```
tar -xf ideaIC-14.1.6.tar.gz -C /usr/share/
rm ideaIC-14.1.6.tar.gz
```

Then we can create a symbolic link to launch IDEA from the command line:
```
sudo ln -s /usr/share/idea-IC-141.3056.4/bin/idea.sh /bin/idea
```

Update the native file watcher to a more efficient version.  Pavel Fatin has been kind enough to supply a pre-built binary:
```
wget pavelfatin.com/files/fsnotifier
chmod +x fsnotifier
sudo cp fsnotifier /usr/share/idea-IC-141.3056.4/bin
sudo cp fsnotifier /usr/share/idea-IC-141.3056.4/bin/fsnotifier64
rm fsnotifier
```

We can then run the selftest to ensure that it's working correctly:
```
cd /usr/share/idea-IC-141.3056.4/bin
./fsnotifier --self-test
```

The output should complete without any warnings or errors. 

From here, we can run the IntelliJ IDEA from the command line: 
```
idea
```

IntelliJ will ask a number of first-time configuration questions.  When at the 'Customize IDEA' dialog, don't forget to install the Scala plugin!  Then click 'Start using IntelliJ IDEA'. 

IntelliJ should now be installed, and accessible in X Windows from ```Menu > Programming > IntelliJ Community Edition```. 


