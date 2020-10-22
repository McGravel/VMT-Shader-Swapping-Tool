# VMT Shader Swapping Tool
#### An attempt at figuring out some C++ by creating a small, practical tool.
#### Intended as a small command-line tool for quickly generating VMT files with the first line changed to swap the Shader used to allow easier usage of Parallax-Corrected Cubemaps, as well as swapping LightmappedGeneric to VertexLitGeneric in the potential case of Propper.
---
Can be used on the command-line by specifying each file one by one if you wish, and is only current way to batch convert from separate folders:  

`vmt_shader_swap_tool.exe "folder_name/file_name.vmt"`

You can also drag-and-drop the desired files onto the EXE for a more convenient way of batch converting them.

The tool will simply add a corresponding suffix the new VMT file to distinguish it, so there's no need to name the original VMT in a certain way:
| VMT Output                 |Filename suffix|
| ---------------------------|---------------|
| Parallax-Corrected Cubemap | `_pcc`        |
| LightmappedGeneric         | `_lmg`        |
| VertexLitGeneric           | `_vlg`        |
