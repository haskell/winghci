#In this page, it is described how to install WinGhci
# Introduction #

Currently, there is no installation program for WinGhci. The way to manually install WinGhci is described below.

# Manual installation #

One obvious requirement to run WinGhci is to have GHC installed on your system. I have tested WinGhci with GHC 6.10.1, but it seems to work with previous versions of GHC.

Next, uncompress WinGhci binary distribution on a directory of your choice. There you will find the following files:

  * WinGhciFile.ico: an icon for Haskell source files. It includes icons of up to 256x256 pixels for Windows Vista.

  * Install.exe: run this program to associate ".hs" files with WinGhci. Addtionally, the previous icon will be associated. It is important to run this program from the directory where WinGhci will remain installed. If you move WinGhci, you must run this program again. Note that the whole process of association is optional.

  * WinGhci.exe: this file corresponds to WinGhci application. Run it to start WinGhci.

  * StartGHCI.exe: this is an auxiliary program used by WinGhi to start GHCI. You should not run it.

  * StartProcess.exe: this is an auxiliary program used by WinGhi to start invoke an editor withing GHCI without blocking GHCI. You should not run it.

  * license.txt: this is WinGhci license.

  * authors.txt: credits to WinGhci authors