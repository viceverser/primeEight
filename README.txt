PrimeEight direct artifact upload fix

Copy the .github folder into the root of your existing PrimeEight repository, replacing the workflow file.
Do not copy README.txt.

This removes the failing 'Stage platform-specific plugin' step entirely.
GitHub Actions now uploads the compiled PrimeEight_CLAP.clap file directly from the build directory.

GitHub Desktop summary: Fix direct Windows CLAP download
Commit to main, then Push origin.
Check Actions for the newest run and its PrimeEight-Windows-x64-CLAP artifact.
