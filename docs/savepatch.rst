===================================
SavePatch File Format Documentation
===================================

The SavePatch file format is a text-based format used to define cheat codes, patches, and modifications for game save files.
This format is parsed by Apollo Save Tool to apply modifications to game save data.

.. contents::
   :depth: 3
   :local:

Basic Syntax Rules
------------------

1. **Case Sensitivity**: Most tags are case-insensitive except where noted
2. **Comments**: Lines starting with ``;`` are comments
3. **Line Endings**: Can use ``\n`` or ``\r\n`` (automatically normalized)
4. **Whitespace**: Trailing spaces are trimmed from lines
5. **Encoding**: Plain ASCII text (UTF-8 may work but not guaranteed)

Error Handling
~~~~~~~~~~~~~~

The parser is generally tolerant of formatting issues:

- Missing closing tags may cause parsing errors
- Malformed hex values may not be detected until patch application
- Empty or malformed lines are skipped
- Unknown directives are ignored

File Structure
--------------

SavePatch files are plain text files with a specific structure that includes file specifications, code definitions, options, and metadata. The format uses special markers and syntax to define different elements.

Header
~~~~~~

The file header (**optional**) consists of 3 lines with the following format:

**Syntax**::

   ;Game ID
   ;Game Title/Name
   ;Author/Source

**Example**::

   ;CUSA00542
   ;The Last of Us Remastered
   ;by Bucanero


File target Specification
~~~~~~~~~~~~~~~~~~~~~~~~~

The file must begin with a file specification that tells Apollo which save files the patch applies to.

The target Save file is **required** to be declared once, but can be declared multiple
times if needed to apply codes to different files. Wildcards are supported.

**Syntax**::

   :<folder_pattern\>{file_pattern}

The **Save Folder** part is optional, and it only needs the folder name.
The program uses it to disable the cheats if a
different folder name is used.

For instance, ``:PROFILE-FOLDER\SAVEFILE.DAT`` if the cheat only works in the profile folder. 

**Examples**::

   :BLES00001-SAVEDATA\SAVEDATA.DAT
   :*.DAT
   :SAVE?.BIN

**File Patterns**

- ``:`` - Colon indicates file specification line
- Exact filename - Apply to specific file only
- Wildcards - Use ``*`` for multiple characters, ``?`` for single character
- Path support - Can include directory paths (use ``\`` as separator)


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


Option Definitions Section
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Options allow users to select different variations of a patch (e.g., different amounts of money, different items).

**Syntax**::

   {option_tag}value1=name1;value2=name2;...{/option_tag}


**Examples**::

   {MONEY}000186A0=99,999;000F4240=1,000,000{/MONEY}
   {WEAPON}01=Iron Sword;02=Steel Sword;03=Diamond Sword{/WEAPON}

**Option Structure**

- Opening tag: ``{OPTION_NAME}``
- Value-Name pairs: ``value=Display Name`` separated by ``;``
- Values are zero-padded hex strings (e.g., "000186A0" for 0x186A0)
- Closing tag: ``{/OPTION_NAME}``
- Values must be fixed width (padded with zeros)

Code Definition Sections
------------------------

Each cheat code or patch is defined in its own section with a title and code lines.

**Code Title Syntax**::

   [CODE_TITLE]
   ; some comments
   code line 1
   code line 2
   ...

**Special Title Prefixes**

- ``[DEFAULT:...]`` - Code is auto-added by default when any code is selected
- ``[INFO:...]`` - Informational/alert message
- ``[PYTHON:...]`` - Python script code
- ``[GROUP:...]`` - Group header for organizing codes

**Code Lines**

Code lines follow the title and contain the actual patch data::

   [Money 9,999,999]
   80010004 12345678
   28000004 0098967F

**Code Types**

The parser detects code types automatically:

- `Game Genie / Save Wizard <#game-genie-save-wizard-codes>`__
- `BSD Script <#bsd-script-codes>`__
- `Python Script <#python-script-codes>`__  (when ``[PYTHON:...]`` prefix is used)

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

To define a `Python script code <index.html#python-libraries-and-micro-libraries>`__,
write the script lines after declaring the code name
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

Option Substitution in Codes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Options can be referenced within code lines using the option tag::
   
   80010004 12345678
   28000004 0{MONEY}

- The ``{MONEY}`` placeholder will be replaced with the selected value
- Multiple options can be used in a single code
- Options can appear anywhere in the code line


Using Cheat Status
~~~~~~~~~~~~~~~~~~~

Cheats Status flags are detected in the cheat code name.

- ``info:`` → If the cheat name starts with ``INFO:`` the text is shown with an alert icon.
-  ``default:`` → The patch is auto-added by default when the user selects any code.
- ``(Required)`` → If a patch code is mandatory to work (like a checksum update), adding the ``(Required)`` tag makes the code to be auto-added.

**Example**::

   [INFO: This Patch may not work] ;The text is shown with an alert icon.
   04000000 00000018

   [Unlock All Character Bios (Required)] ;The code is required and will be auto-added.
   94000000 00000018
   4A000000 00000001
   402A0004 00000000

Grouping Codes
--------------

Codes can be grouped together using group headers::

   [GROUP:Cheat Category 1]
   [Infinite Health]
   ...code lines...

   [Infinite Ammo]
   ...code lines...

   GROUP:Cheat Category 2
   [Unlock All Weapons]
   ...code lines...

- ``[GROUP:...]`` starts a group
- Subsequent codes belong to the group until next group header
- The current Group can be ended with a ``[Group:\]`` separator

**Example**::

   [group:Group #1]
   ; The patches that follow this label are grouped as childs.

   [default:Cheat 1]
   00000000 00000000

   [Another Cheat in Group #1]
   10000123 0000ABCD

   [group:Group #2]
   [Cheat in Group #2]
   00000000 00000000

   [group:\]
   ; Ends the current group.

   [info:Cheats below does not work]

   [Cheat 2 (partial working)]
   00000000 00000000

   [Cheat 3 (not working)]
   00000000 00000000

Complete Example
----------------

Here's a complete SavePatch file example::

   ; CUSA00001
   ; Example Game
   ; Author: Cheat Author
   ; ============================================

   :BLES00001-SAVEDATA\SAVEDATA.DAT

   {MONVAL}000186A0=99,999;000F4240=1,000,000{/MONVAL}
   {LV}0001=Level 1;000A=Level 10;0032=Level 50{/LV}

   [GROUP:Basic Cheats]

   [DEFAULT:Max Money]
   80010004 12345678
   28000004 {MONVAL}

   [INFO:This cheat gives you maximum money]
   ; This is just a comment showing info

   [Max Level]
   80010004 23456789
   28000004 0000{LV}

   [GROUP:Advanced Cheats]

   [Unlock All Weapons]
   80010004 34567890
   28000004 FFFFFFFF

   [Group:\]
   ; Ends the current group.

   [PYTHON:Custom CRC32 Script (Required)]
   # Python script embedded in savepatch
   import uhashlib

   # Custom patch logic to update checksum
   crc = uhashlib.crc32(savedata[0x10:])
   savedata[4:8] = crc

Notes
-----

- **Text Editors**: Any plain text editor can create/edit files

Best Practices
~~~~~~~~~~~~~~

1. **Always include file specification** as the first non-comment line
2. **Use consistent hex formatting** (uppercase, fixed width)
3. **Test options** with all possible values
4. **Include comments** for complex patches
5. **Use groups** to organize related cheats
6. **Validate** patch files with :doc:`Apollo CLI </cli>` before distribution

References
~~~~~~~~~~~~~

- `Apollo SDK documentation <https://deepwiki.com/bucanero/apollo-lib>`__
- :doc:`Save Wizard / Game Genie code format </savewizard>`
- :doc:`Bruteforce Save Data script syntax </bsd>`
- `Python scripting with Apollo <index.html#python-libraries-and-micro-libraries>`__
- `Game-specific cheat code databases <https://github.com/bucanero/apollo-patches>`__
