PrimeEight Windows runtime fix

The previous workflow distributed a Debug build, which may require Microsoft's
Debug C++ runtime libraries that ordinary Windows PCs do not have. This fix
keeps Debug tests, but packages a separate Release CLAP using the static MSVC
runtime (/MT). It also loads the Release plugin on the Windows CI runner.

Copy the .github folder into your local PrimeEight repository, replacing the
existing workflow. In GitHub Desktop, commit with Summary:
Fix Windows CLAP runtime dependencies
Then Push origin. Download the NEW Windows artifact after the green build.

NOTE: The precise missing DLL on the user's computer has not been independently
identified; this fixes the most likely cause of Windows LoadLibrary error 126.
