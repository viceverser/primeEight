# PrimeEight approved GUI assets

The authoritative upload package is `PrimeEight_GitHub_GUI_Assets.zip` (provided by the project owner).

Expected paths:
- `GUI/assets/PrimeEight_Original_Ape8_Logo.svg` — exact protected user artwork; SHA-256 `771d47ba2b6b9c32782f3a01b63c0face67a79b5906aeade8f87334dfad57c34`
- `GUI/reference/PrimeEight_GUI_Approved_Reference.png` — approved flat GUI visual reference; SHA-256 `9d16460b5ecae13bc4649ed2e1fa07bad06c68afae89af815c3e84d0b5f2b671`
- `GUI/README_ASSETS.md` — original asset guidance

**Do not alter the SVG.** The PNG is a visual reference, not a bitmap UI background. Implement native scalable graphics and live controls; do not invent new branding or gradients.

The source binary assets must be uploaded unchanged from the ZIP to the above paths. This manifest is not a substitute for those assets.

## Native vector implementation decision (2026-10-16)
The original SVG is the authoritative runtime geometry source; do not rasterize it to PNG. The SVG contains 15 paths and a hidden Inkscape reference-image layer (`layer2`, `display:none`). The embedded image is NOT part of the visible logo and must not be shipped in the runtime representation. Preserve the SVG paths, group transforms, fills, and gradient semantics precisely. Implement native vector rendering from those visible paths; do not approximate or redraw them. The prior GDI+/PNG resource approach has been removed from the CLAP editor and CMake configuration. Do not reintroduce it.
