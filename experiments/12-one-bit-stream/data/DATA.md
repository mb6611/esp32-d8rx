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
| `iqs1_cli.sigmf-data` | 39,999,456 | 2026-11-05 16:18 | release asset | `1457d980d5c6df96eb637f8c0cf837a12c32a286bed205f9e3319ff0fdd84c77` |
| `iqs1_cli.sigmf-meta` | 669 | 2026-11-05 16:18 | in repo | `24ac7d63473751891b7cb91b7ffc42e89f0006ecc6adc2e1c7f5c1aa9319ce57` |
| `iqs1_d64.npy` | 40,001,600 | 2026-11-05 16:17 | release asset | `f7b522d22b3c536ac452b7f75e914c3e1cecbe22d1f292563612144a7b24ffc0` |
| `iqs_reg.sigmf-data` | 2,499,960 | 2026-11-05 16:19 | release asset | `c3cf1baeac40d4b97646457d7200be946f922bcea4cc7069749cd06eac1df041` |
| `iqs_reg.sigmf-meta` | 668 | 2026-11-05 16:19 | in repo | `ce340753e230b914a81703d2458a63a1542bac8f211e3f317e7b15cd30e98b5a` |
