PrimeEight v127 - MuLab compatibility candidate

Changes: allow CLAP activate() with min_frames_count=0, as allowed by the CLAP API; updated activation regression test.

This is a hypothesis-driven fix, not a confirmed diagnosis of the MuLab failure.

22/22 Linux tests passed. Windows and MuLab validation are pending.

HOW TO APPLY (GitHub Desktop):
1. Extract this ZIP.
2. GitHub Desktop > Repository > Show in Explorer.
3. Copy the extracted clap and tests folders into your repository folder, replacing files when asked.
4. Commit summary: Allow zero-minimum CLAP activation
5. Click Commit to main, then Push origin.
6. Wait for the green GitHub Actions build. Download the new Windows artifact, replace the installed .clap file, restart MuLab and scan.

Keep the existing workflow and all other repository files unchanged.
