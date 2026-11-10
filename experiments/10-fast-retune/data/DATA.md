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
| `hop_sweep_g50.log` | 6,766 | 2026-11-05 15:56 | in repo | `ffc2bd3fdefac260d766ce69d2481e7ae8e96a60484fbea8e4cc838ab04dc456` |
| `hop_sweep_g50.npz` | 18,749,241 | 2026-11-05 15:56 | release asset | `25c48121e75cc7db8bad3e914bbd125c2c985eac3be2496b486f60bb70c2d5ff` |
| `hop_sweep_ref2404_80.log` | 1,401 | 2026-11-05 16:04 | in repo | `ce8f4116576cb6d041441eefefa28f267a39e2b1fbcc2d1090af9844d5c3adcb` |
| `hop_sweep_ref2404_80_part1.log` | 1,401 | 2026-11-05 16:05 | in repo | `ce8f4116576cb6d041441eefefa28f267a39e2b1fbcc2d1090af9844d5c3adcb` |
| `hop_sweep_ref2404_80_part2.log` | 7,253 | 2026-11-05 16:10 | in repo | `5ba042026efe5ddf6c5dffb037da75e507eee44103d2a98e41af83bd1c00bdbb` |
| `hop_sweep_ref2404_80_part2.npz` | 13,171,187 | 2026-11-05 16:10 | release asset | `4973989c72f38f511b547a91a969874162469c42c2c1c14ab5e5e3a62130a960` |
| `hop_try.npz` | 1,960,430 | 2026-11-05 15:47 | release asset | `0b5fbe5005074dad42e6ea2cfea340f7e056a8ccc0741ddc9a2f6e35726ef776` |
| `s_big_m0-q.npz` | 1,179,545 | 2026-11-05 16:11 | release asset | `efbdbd5d760cf70ddce39ac2c87e3a1a09cb49b6fa5f97c9ddf03b3cf6ecd424` |
| `s_big_m1-q.npz` | 1,214,106 | 2026-11-05 16:11 | release asset | `804bfec1219cb43ed25827d6ec78eaa992a2f66b2e784b40630bcc2ec3020167` |
| `s_big_m2-q.npz` | 1,225,114 | 2026-11-05 16:11 | release asset | `7ab43e24e11f98629c6afe12573d6f93c08e87c6807cb617fd547eded0eea51a` |
| `s_small_m1-q.npz` | 1,156,464 | 2026-11-05 16:11 | release asset | `81ab96e6eab0e0762ba77d2483e3af997700ca430d786bb8d2b22970591fb2cd` |
| `settle-16msps.txt` | 1,063 | 2026-11-05 16:11 | in repo | `099cb78d2345f8f0b9d5d01bb04b4dd086df174eba2971f46671a544b2ac24f2` |
| `settle16_big_m0.npz` | 1,179,545 | 2026-11-05 15:59 | release asset | `efbdbd5d760cf70ddce39ac2c87e3a1a09cb49b6fa5f97c9ddf03b3cf6ecd424` |
| `settle16_big_m1.npz` | 1,214,106 | 2026-11-05 15:59 | release asset | `804bfec1219cb43ed25827d6ec78eaa992a2f66b2e784b40630bcc2ec3020167` |
| `settle16_big_m2-q.npz` | 1,225,114 | 2026-11-05 16:01 | release asset | `7ab43e24e11f98629c6afe12573d6f93c08e87c6807cb617fd547eded0eea51a` |
| `settle16_big_m2.npz` | 1,225,114 | 2026-11-05 16:00 | release asset | `7ab43e24e11f98629c6afe12573d6f93c08e87c6807cb617fd547eded0eea51a` |
| `settle16_small_m1-q.npz` | 1,156,464 | 2026-11-05 16:01 | release asset | `81ab96e6eab0e0762ba77d2483e3af997700ca430d786bb8d2b22970591fb2cd` |
| `settle16_small_m1.npz` | 1,156,464 | 2026-11-05 16:01 | release asset | `81ab96e6eab0e0762ba77d2483e3af997700ca430d786bb8d2b22970591fb2cd` |
| `settle40_big_m1-q.npz` | 1,217,455 | 2026-11-05 16:01 | release asset | `b89b11e4d97f94f0e5485c30e0f9b91db6f6afc19fa68cc1aa3397742dc997d1` |
| `settle_big_m0.npz` | 1,059,565 | 2026-11-05 15:58 | release asset | `cbd7563ad46fdc0ae700cc7a08ef11e12337400e3baa26c9adddcdb89ea0e919` |
| `settle_big_m1.npz` | 1,217,455 | 2026-11-05 15:57 | release asset | `b89b11e4d97f94f0e5485c30e0f9b91db6f6afc19fa68cc1aa3397742dc997d1` |
| `settle_small_m1.npz` | 1,148,456 | 2026-11-05 15:57 | release asset | `cfa58a164dbe2a2203ca8496b7d5227c2bf4b5f5aea6d2d259899d748d34b4f2` |
| `sweep-ref2404-80msps.log` | 8,507 | 2026-11-05 16:11 | in repo | `54298d03cff8f4b6e652e539f12c24219dff4b6f174d652d758080a1c2117180` |
| `sweep-ref2440-40msps.log` | 6,766 | 2026-11-05 16:11 | in repo | `ffc2bd3fdefac260d766ce69d2481e7ae8e96a60484fbea8e4cc838ab04dc456` |
