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
| `first_unsure.sigmf-data` | 5,000,056 | 2026-11-05 14:10 | release asset | `80588106a389e43155f2641229b5997eef28321cd25116d23e203319b01675b5` |
| `first_unsure.sigmf-meta` | 668 | 2026-11-05 14:10 | in repo | `24f35304585f5a3f1bf2649f39c4b61f4b77bdf049121e97a254c2d53fe24870` |
| `off.sigmf-data` | 4,999,928 | 2026-11-05 14:11 | release asset | `a6d94632f886cad5b0c0d2c394660c1ed65b87a979fd9c9756a5924ec15aaed7` |
| `off.sigmf-meta` | 668 | 2026-11-05 14:11 | in repo | `4db9be4cf1576aac0a6912795dd410f7a9b5a34563ab651d45298177d5ccf324` |
| `on.sigmf-data` | 4,999,928 | 2026-11-05 14:12 | release asset | `db6d0757fa39e7f1bc092ea5d7860b3502282d7041039f798facf75f649f6235` |
| `on.sigmf-meta` | 668 | 2026-11-05 14:12 | in repo | `2c8be5cd24052a4e2365546daac5270dc1e8ce5a744d7cc06606ccb025ad0167` |
