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
| `ble.sigmf-data` | 14,999,920 | 2026-11-05 13:36 | release asset | `0f02a6aa39dffc2d71da9d5eb1fa889180a1f50052afb3b2dbe9fb486424939a` |
| `ble.sigmf-meta` | 600 | 2026-11-05 13:36 | in repo | `2dd2b512772c1751abe1e7f89212f8ffc4c6e5013bce81abc93560e6247bf0e7` |
| `u6b_60s.log` | 786 | 2026-11-05 13:34 | in repo | `c9c40a20e7222e9b959689ade57e170f319037172273bbcc382759c7c9c1bebe` |
