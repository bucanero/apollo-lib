=================================
Savepatch file format Reference
=================================

.. contents::
   :depth: 3
   :local:

Header
~~~~~~

The file header (**optional**) consists of 3 lines with the following format:

::

   ;Game ID
   ;Game Title/Name
   ;Author/Source

**Example**::

   ;CUSA00542
   ;The Last of Us Remastered
   ;by Bucanero


Set path and target Save file
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The target Save file is **required** to be declared once, but can be declared multiple
times if needed to apply codes to different files. Wildcards are supported.

::

   :<SAVE-FOLDER\>{SAVEFILE.DAT}


The **Save Folder** part is optional, and it only needs the folder name.
The program uses it to disable the cheats if a
different folder name is used.

For instance, if the cheat only works in the profile folder. 

**Example**::

   :BLES00049PROFILE002\EM_GAME.DAT

Here, ``BLES00049PROFILE002`` is the **Save Folder**, and
``EM_GAME.DAT`` is the target Save Data file name.


If only the file name is used, the folder is not validated.

**Example**::

   :EM_GAME.DAT

If the code is valid for multiple Save files, you can use the asterisk ``*`` wildcard.

**Example**::

   :*.SAV

Game Genie / Save Wizard Codes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To define a :doc:`Save Wizard / Game Genie code </savewizard>`, simply write the code lines
after declaring the code name between square brackets. (Example: ``[Cheat Name]``)

**Example**::

   :PLAY.DAT

   [Max HP & SP Points]
   2000006C 0001869F
   20000170 0001869F


Python Script Codes
~~~~~~~~~~~~~~~~~~~

To define a Python script code, write the script lines after declaring the code name
between square brackets and the ``[python:Cheat Name]`` tag.

**Example**::

   :SAVE*.DAT

   [Python:99 Upgrade points]
   import apollo

   pos = apollo.search(savedata, "VoiceOfThePeople")
   savedata[pos + 32] = 0x63

   print("Written value:", savedata[pos + 32])

BSD Script Codes
~~~~~~~~~~~~~~~~

To define a :doc:`BSD script code </bsd>`, write the script lines after declaring the code name
between square brackets. (Example: ``[Cheat Name]``)

**Example**::

   :SAVE.DAT

   [BSD Example 1]
   ;Searches for the word "difficulty" as text.
   search "difficulty" 

   ;Overwrites with the text at 0 bytes after the found offset.
   ; in this case "next 0" is at the found offset.
   ; "next (10)" or "next 0xA" would write 10 bytes after the found offset.
   write next 0: "VeryEasy" 

   [BSD Example 2]
   ;Addresses enclosed in parenthesis are treated as decimal.
   ; Like (99) = 0x63 
   write at (99): 0x446966666963756C7479

   ;The following 2 lines are equivalent:

   write at (99): 0x446966666963756C7479
   write at 0x63: "Difficulty"


Using Cheat Status
~~~~~~~~~~~~~~~~~~~~~~

“Cheats Status” is detected in the cheat name.

If the cheat name starts with ``INFO:`` the text is displayed in bold

[STRIKEOUT:If it contains the text “partial working” the text is
displayed in orange If it contains the text “not working” the text is
displayed in red If it contains the text “working” the text is displayed
in blue]

Otherwise the text is displayed in black

::

   [INFO: Patches may not be supported by BSD yet] ;The text is displayed in bold

   [Unlock All Character Bios (Not Working)] ;The text is displayed in red.
   94000000 00000018
   4A000000 00000001
   402A0004 00000000

   [Unlock All Character Bios (Partial Working)] ;The text is displayed in Orange.
   94000000 00000018
   4A000000 00000001
   402A0004 00000000

   [Unlock All Character Bios (Working)] ;The text is displayed in Blue.
   94000000 00000018
   4A000000 00000001
   402A0004 00000000

Using Labels and Groupings
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

-  ``group:`` -> The labels of patches that follow this label are
   grouped as child.
-  ``default:`` -> The patch is checked by default.

Example :

::

   [group:orange:Group #1]
   [default:Cheat 1]
   00000000 00000000

   [red:info:Cheat below does not work]

   [Cheat 2 (partial working)]
   00000000 00000000

   [Cheat 3 (not working)]
   00000000 00000000
