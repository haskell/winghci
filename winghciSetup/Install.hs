{-# OPTIONS -optl-mwindows #-}


import Monad
import System.Win32
import Graphics.Win32.Misc
import Graphics.Win32
import Bits
import Maybe

createKey key path sam
 = regCreateKeyEx key path "" rEG_OPTION_NON_VOLATILE sam Nothing
 

setValue key path var buf =
 do (root,res) <- createKey key path kEY_WRITE 
    withTString buf $ \lptstr ->
            do
             regSetValueEx root var rEG_SZ lptstr (2*length buf)
             regCloseKey root
             return True


writeHKCRString wher var val =
  setValue hKEY_CLASSES_ROOT wher var val

haskellScript = "winghci_haskell"
haskellLiterateScript = "winghci_literate_haskell"


regExt haskellScript ext def =
  do winGhciPath <- getWinGhciPath
     writeHKCRString ext "" haskellScript
     writeHKCRString haskellScript "" def
     writeHKCRString (haskellScript++"\\DefaultIcon") "" (winGhciPath++"WinGhciFile.ico,0") 
     writeHKCRString (haskellScript++"\\shell") "" ""
     writeHKCRString (haskellScript++"\\shell\\Open") "" ""
     writeHKCRString (haskellScript++"\\shell\\Open\\command") "" ""

     writeHKCRString (haskellScript++"\\shell\\Open\\command") "" ("\""++winGhciPath++"winghci.exe\" \"%1\"")


getWinGhciPath = 
 do
   prog <- getModuleFileName nullPtr
   return . reverse . dropWhile (/='\\') . reverse $ prog


main =
 do resp <- messageBox nullPtr "Dou you want to associate haskell files with WinGhci?" "WinGhci" (mB_YESNO .|. mB_SYSTEMMODAL)
    when (resp==iDYES) $
     do
      regExt haskellScript ".hs" "Haskell Script"
      writeHKCRString ".hs\\ShellNew" "FileName" ""

      regExt haskellLiterateScript ".lhs" "Literate Haskell Script"
      writeHKCRString ".lhs\\ShellNew" "" ""

      messageBox nullPtr "Haskell files have been associated with WinGhci" "WinGhci" (mB_OK .|. mB_SYSTEMMODAL)
      return ()

