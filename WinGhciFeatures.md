#WinGhci features are described in this page
# Resume of WinGhci features #

WinGhci is quite similar to WinHugs, so hopefully most people will find it easy to use. Basically, GHCI is run inside a Windows Rich Edit control, so you can issue there GHCI commands.

In order to open an script you can navigate to it through `File-> Load` menu entry. Additionally, you can drag a file over WinGhci window. You can also add a module using `File->Add` (this will issue an :add command). Recently loaded modules are recorded on the File menu.

You can set GHCI options either directly from the command line or through `File->Options` menu entry. Using this dialog you can also change the Rich Edit control font. One extension with respect to GHCI options is that you can use the & character as a prefix when setting your editor, so that when you invoke the :edit command, GHCI will not block until you close your editor. You can also set additional command line parameters to GHCI through the `GHCI Startup` entry. Note that this will restart the interpreter. Please, do no use "ghci.exe" in your startup command but use "ghc.exe --interactive" instead. All options are recorded in the windows registry and are kept among different sessions.

A history of issued commands is kept by WinGhci. This history is also recorded in the registry, so that it is also kept among sessions. You can move through your command history by using the UP and DOWN keys. Additionally, you can search in your history. Simply type a text and press TAB repeatedly to search all previous commands containing that text. You can also clear the current command by pressing ESC.

There are also some shortcuts to access most usual commands:

  * ctrl+r will invoke ":reload"
  * ctrl+e will invoke ":edit"
  * ctrl+l will open the ":load" dialog
  * ctrl+a will open the ":add" dialog
  * ctrl+o will open the options dialog
  * ctrl+m will evaluate "main" expression
  * ctrl+c will stop the current evaluation
  * F1 will open GHC documentation

Additionally, ctrl+0 to ctrl+9 can be used to invoke user defined commands available through the `Tools` menu entry. These commands can be defined using the `Tools->Configure Tools` dialog. Note that the text "`<`fileName`>`" will be replaced by the name of the currently loaded file, and "`<`fileExt`>`" will be replaced by its extension. For instance, there is a default command to invoke GHC on the currently loaded file already defined as follows:

:! ghc --make "`<`fileName`>``<`fileExt`>`"