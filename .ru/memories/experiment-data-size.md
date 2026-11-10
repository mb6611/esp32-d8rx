---
name: experiment-data-size
description: Read before adding or moving data files under experiments/ (what may be committed vs. what goes into the release asset), as of 2026-11-09.
load: description
---
Keep the repo light: experiment data files over 1 MB (and directories of many recordings, like screenshots/videos) are never committed. They go into the GitHub release asset `experiments-data-v1` (staged under `~/Developer/personal/backups/release-assets/esp32-d8rx-experiments-data-v1/`, archive layout mirrors `experiments/NN-name/data/...`), are listed in that experiment's `data/DATA.md` with size and sha256, in `experiments/data-v1.sha256`, and in `.gitignore`. `experiments/fetch-data.sh` downloads and unpacks them.

**Why:** the owner decided on 2026-11-09 to keep committed data small (target under 20 MB) after the restructure staged ~288 MB of data.
**How to apply:** new large data means a new archive (and a new sha in fetch-data.sh, or a v2 release); ask the owner before publishing a release.
