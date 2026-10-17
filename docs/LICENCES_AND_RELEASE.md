# Third-party licences and release notes

PrimeEight original source: MIT, Copyright (c) 2026 viceverser. See ../LICENSE.

## Bundled dependencies
- CLAP SDK in vendor/clap-main: MIT, Copyright (c) 2021 Alexandre Bique. See ../vendor/clap-main/LICENSE.
- Reference example in vendor_reference/clap-c99-distortion.c: accompanied by MIT licence, Copyright 2022 Paul Walker and contributors. See ../vendor_reference/LICENSE.md.

## Optional experimental GUI build
- JUCE 8.0.10: downloaded by CI from juce-framework/JUCE. JUCE is dual-licensed; determine and comply with the applicable AGPL/commercial terms before distributing JUCE-linked binaries. Include its actual licence notices.
- Surge Synth Team sst-clap-helpers: CI checks out revision f17567a8166a50bfb50ddc9acca7e77c72e6096f. Verified at this exact revision: MIT, Copyright 2019–2020 Paul Walker. Preserve the upstream LICENSE.md notice with distributed source and binaries.

## Artwork and trademarks
The MIT source-code licence does NOT grant rights to the original PrimeEight logo, graphic assets, or trademarks. The original ape/8 logo, headings, and approved GUI artwork remain Copyright (c) 2026 viceverser, all rights reserved unless separately licensed. Artwork is provided only for identification and building authorized PrimeEight distributions. Clarify the scope of redistribution before public release.

## Publication gate
- Review git history and all files for credentials, private material and third-party asset provenance.
- Verify all dependency licence obligations, including JUCE-linked binaries.
- Confirm ownership and permissions for every contributed file.
- Confirm whether an open-source build can be produced without proprietary artwork.
- Add AppVeyor configuration and test Windows builds.
- Do not make repository public until owner explicitly approves.
