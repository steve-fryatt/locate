Locate
======

A flexible file search utility for RISC OS.


Introduction
------------

Locate is a file find utility for RISC OS. It will search a specified directory for files that match a set of conditions. These conditions include the filename (including wildcards), file size, modification date, filetype, attributes and contents.

Locate is loosely based on similar utilities available on other platforms: particularly the File Find tool that was once found on the '9x versions of Windows.

Note that with the arrival of Locate 2, there are now two distinct versions of Locate in circulation. While they share the same name and outward appearance, internally they are very different pieces of software. Locate 2 will load search settings saved from the original, but will currently not load old results (although it can load settings from those files so that searches can be repeated) -- this limitation might be removed in a future update. It will never be possible to load files saved from Locate 2 into the original version.

The source for the original Locate can be found in the original folder, although there are no guarantees that it can be built easily.


Building
--------

Locate consists of a collection of C and un-tokenised BASIC, which must be assembled using the [SFTools build environment](https://github.com/steve-fryatt). It will be necessary to have suitable Linux system with a working installation of the [GCCSDK](http://www.riscos.info/index.php/GCCSDK) to be able to make use of this.

With a suitable build environment set up, making Locate is a matter of running

	make

from the root folder of the project. This will build everything from source, and assemble a working !Locate application and its associated files within the build folder. If you have access to this folder from RISC OS (either via HostFS, LanManFS, NFS, Sunfish or similar), it will be possible to run it directly once built.

To clean out all of the build files, use

	make clean

To make a release version and package it into Zip files for distribution, use

	make release

This will clean the project and re-build it all, then create a distribution archive (no source), source archive and RiscPkg package in the folder within which the project folder is located. By default the output of `git describe` is used to version the build, but a specific version can be applied by setting the `VERSION` variable -- for example

	make release VERSION=1.23


Licence
-------

Locate is licensed under the EUPL, Version 1.2 only (the "Licence"); you may not use this work except in compliance with the Licence.

You may obtain a copy of the Licence at <http://joinup.ec.europa.eu/software/page/eupl>.

Unless required by applicable law or agreed to in writing, software distributed under the Licence is distributed on an "**as is**"; basis, **without warranties or conditions of any kind**, either express or implied.

See the Licence for the specific language governing permissions and limitations under the Licence.