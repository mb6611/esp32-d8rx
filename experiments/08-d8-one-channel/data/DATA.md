# Data files

Raw data as recorded. Never edit these files.
Times are file modification times (local time, EST), kept from the original files.

Files marked *release asset* are larger than 1 MB and are not in git. They are in [esp32-d8rx-experiments-data-v1.tar.zst](https://github.com/mb6611/esp32-d8rx/releases/download/experiments-data-v1/esp32-d8rx-experiments-data-v1.tar.zst) (GitHub release `experiments-data-v1`). To put them in place, run from the repo root:

```sh
experiments/fetch-data.sh
```

It downloads the archive, unpacks it at the repo root (into `experiments/*/data/`) and checks every file against `experiments/data-v1.sha256`.

| file | bytes | time | where | sha256 |
|---|---:|---|---|---|
| `ch_on.sigmf-data` | 9,999,864 | 2026-11-05 14:57 | release asset | `cb395775df2ea98895e0530a96c80aeda0ac26a9b105593f04cc125adb2d22d5` |
| `ch_on.sigmf-meta` | 670 | 2026-11-05 14:55 | in repo | `74bc40ff3208d98022032027f2fba8bcab525ef56f4a76d60b76fd49925fc0c5` |
