# Resources

[Русская версия](resources.ru.md)

Files of a project that the game uses: scenes now, meshes, textures,
materials, shaders and sounds with their steps. Where they lie and the file
formats are in [project and file formats](project.md).

Status marks: see [README](README.md).

## Resource IDs [done]

- A resource is referred to by a `ResourceId`, a 64-bit number, not by its
  path: a renamed or moved file keeps all references to it (the startup
  scene, the mesh of a `MeshRenderer`).
- The ID lives in a `.meta` file next to the resource (`level.yscene.meta`
  holds `{"id": "..."}`), so it travels with the file in version control.
- `ResourceDatabase` keeps both directions, ID to path and path to ID, all
  paths relative to the project folder.
- A new ID is 64 random bits, so two projects practically never give the
  same ID; it is always at least `builtin::kFirstProjectId`.
- **Built-in resources** (the cube, sphere and plane meshes) have small
  constant IDs below `kFirstProjectId` and no file; a project ID never
  collides with them.
- In a property the reference is a `ResourceRef`; 0 means "none".

## Keeping IDs right [done]

- `scan` walks the project folder when the project opens. A file without a
  `.meta` gets one with a new ID. If an ID repeats (a file copied together
  with its `.meta`), the copy gets a new one. Hidden files and the reserved
  names (`project.yqlora`, `settings/`) are not resources.
- The editor tells the database about its own changes: `add` for a new file,
  `moved` after a rename or a move (the `.meta` follows the file, IDs stay),
  `removed` after a delete (the `.meta` goes away, the ID is forgotten).
- Changes made outside the editor are picked up by the next `scan`.

## Planned

- Loading and caching of resources instead of the static lookup of built-in
  meshes; meshes and textures from files **[planned, step 4]**.
- The window "create a file" in the Files panel (the name is asked before the
  file appears; renaming to the same name is not an error)
  **[planned, step 4]**.
