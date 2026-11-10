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
| `strobe_off.npz` | 6,888,708 | 2026-11-05 15:03 | release asset | `bc11c335d63d701350c4e2ec8cf64c7abc2a080824daead3fc42503a6bbf7ad1` |
| `strobe_on.npz` | 6,864,108 | 2026-11-05 15:00 | release asset | `1167286459546ba29d3d8a9bdbed4434dac9edc99c379750b8811ef8798b7cc6` |
