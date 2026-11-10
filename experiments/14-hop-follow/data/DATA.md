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
| `d8follow-630s-g40.log` | 2,401 | 2026-11-05 18:23 | in repo | `e49f267c8bfd6708aa082fa5ef6cd12d9f5238c4fa377e637a175163aaf98530` |
| `d8live-follow-159s.log` | 1,438 | 2026-11-05 17:52 | in repo | `51bc9ec721df1ce3f0759837ca1b523181665dd694a16bf629b53df3bb4c367b` |
| `d8live_follow.log` | 1,438 | 2026-11-05 17:51 | in repo | `51bc9ec721df1ce3f0759837ca1b523181665dd694a16bf629b53df3bb4c367b` |
| `d8shot/cast.mjs` | 647 | 2026-11-05 18:40 | release asset | `c4beddb885a8007e85487cde6a3bacd8c1308c7b3ab7b35cc9194ea4c4b51a9f` |
| `d8shot/cast2.mjs` | 589 | 2026-11-05 18:40 | release asset | `5111bf8af5bf84e945b5b049349611ccd1d76dcc93ddb3b6a60acda870a5a958` |
| `d8shot/ch.gif` | 6,378,965 | 2026-11-05 18:41 | release asset | `61bbcef8aed38a1306f3be24a42647c1954a71eacc6cde24d18a3182df84fd05` |
| `d8shot/ch.png` | 51,577 | 2026-11-05 18:36 | release asset | `05785077d1cd0f20305b8232ff4394457972ffa15ef4b258dd7dadc1e55b29d3` |
| `d8shot/ch.webm` | 24,662,075 | 2026-11-05 18:41 | release asset | `4f71b13d05fcd4aeac7e06c79faf07df62d69d07d3e4b0154928b9f614ad777f` |
| `d8shot/ch2.gif` | 2,771,331 | 2026-11-05 18:49 | release asset | `82df43cf0f112e5317aace229acd5fd41fd4839ea8be746fddccb5224fe13797` |
| `d8shot/ch2.webm` | 18,642,923 | 2026-11-05 18:48 | release asset | `cedba2da8bca64ef556a08bac2d6ff14f86bb52626414e37dfe2f42cc148eb0d` |
| `d8shot/ch2f.png` | 80,216 | 2026-11-05 18:49 | release asset | `e952d5cc17080795054919ceea694530ce98d486cd797b72feaa1f71b63629fe` |
| `d8shot/chf.png` | 114,513 | 2026-11-05 18:41 | release asset | `e1b97d5f3055e0950abe19292087274313dfb04fbf10cd713a420243b1f42202` |
| `d8shot/cons.mjs` | 509 | 2026-11-05 18:15 | release asset | `ef50eb0ac801e22d65e8647575fa33bc3e0b8da0001db3e49a36975579672fdc` |
| `d8shot/dbg.mjs` | 916 | 2026-11-05 18:25 | release asset | `54581f376875b4665698eca0f34fc84e228644393431fa2d9e438d86abf473cb` |
| `d8shot/hm_check.png` | 353,408 | 2026-11-05 18:44 | release asset | `dd9dc892b2371c3d66043fe7fcc8cab8dcae1a1427adf42aef9f4c96a7003e25` |
| `d8shot/package-lock.json` | 35,840 | 2026-11-05 18:01 | release asset | `af0c2852631eaddff71bf8eced4210e0169efa083dfd086ce6fe23938cd7c884` |
| `d8shot/package.json` | 300 | 2026-11-05 18:01 | release asset | `b3d78fb2d99124f56c3d11483ef922947f958a28acab98ab9878a4f48aac82f8` |
| `d8shot/perf.mjs` | 895 | 2026-11-05 18:16 | release asset | `ae67ef98244a57acb330cec6cce13a1bc469761e8262f864722d5170f5a0480f` |
| `d8shot/replay.log` | 15 | 2026-11-05 18:48 | release asset | `798b9c371b5bae8cc27d157b9b66085d61e595e0aeae20a6c5e9a2a7e4998faa` |
| `d8shot/replay.py` | 607 | 2026-11-05 18:40 | release asset | `8449be9011e8d1d2ccdf0f7dcd57ea9b9fa06d1315363a3684719b3943388295` |
| `d8shot/rp.png` | 71,070 | 2026-11-05 18:40 | release asset | `c01488a02c6967377f99044d95e2b0a09ad052a9b1579d62900d8f53dd05206e` |
| `d8shot/select.mjs` | 6,602 | 2026-11-05 18:43 | release asset | `803d39434937ed75f18d88633e89f2b3d366ab28dcc94890b37da92d0a35d9be` |
| `d8shot/shot.mjs` | 563 | 2026-11-05 18:01 | release asset | `2c064c088715f2964f1dd7b2f11384934af66465629a7d3a9148e6c68368001f` |
| `d8shot/shot_wf.mjs` | 631 | 2026-11-05 18:36 | release asset | `9c391af14c7c33ce21dc6246239cc35576c40eb810d1ea853f730a7d3c21be09` |
| `d8shot/sim.log` | 12 | 2026-11-05 18:20 | release asset | `f92f91cc36a7ac456d2a4d53881ef38ca5d483478f1a25bbaf8839f757f1a088` |
| `d8shot/sim.py` | 2,210 | 2026-11-05 18:15 | release asset | `051b0b91c5d3ad90fd7040bdeecf9d9c5e063fea549f34220add6a6263498bc1` |
| `d8shot/stats.png` | 78,135 | 2026-11-05 18:36 | release asset | `78e7099f3dfdb0a0fcd9222cdbbd7cad5f324eeea2ca12dab9c98cc203eba4dc` |
| `d8shot/still.mjs` | 544 | 2026-11-05 18:43 | release asset | `32e7b7b4814d15ac66c4a140650d4f30c529ed711b11203940222677caca8ca9` |
| `d8shot/sts.png` | 212,872 | 2026-11-05 18:43 | release asset | `b2586564a47b29165492cba549d468849b68dfa6cfcc83cb9bb029d5d8ac28f2` |
| `d8shot/sts2.png` | 212,872 | 2026-11-05 18:44 | release asset | `b2586564a47b29165492cba549d468849b68dfa6cfcc83cb9bb029d5d8ac28f2` |
| `d8shot/sts_p.png` | 136,619 | 2026-11-05 18:44 | release asset | `7fede6d7246e6e50c469541733a0bf2b1a17c37cb8c592cb26efbd139fd3107b` |
| `d8shot/t_12_680_32_12.gif` | 3,274,382 | 2026-11-05 18:42 | release asset | `267a9ab0b5df237a022d3f6618b9f888c09fe43be9bf7dbbc492d19c03551b56` |
| `d8shot/t_15_640_32_10.gif` | 2,867,481 | 2026-11-05 18:42 | release asset | `d68130a44a0592585b088a39cf76b63dd8403fee089786f5d3b15e9a035afd79` |
| `d8shot/t_15_680_48_12.gif` | 4,248,528 | 2026-11-05 18:42 | release asset | `66605f930226ae15194cb1e66ea445c9b16d44f91e4dd2ef2cb85bef1b699467` |
| `d8shot/tf.png` | 65,362 | 2026-11-05 18:42 | release asset | `a627d4e38911d04d5d891606b04f001aafda40da0bc39bad41c5c425b62f40c8` |
| `d8shot/tips.mjs` | 4,741 | 2026-11-05 18:37 | release asset | `4cfb21e61329dd18e8b70adf77cd69417aec7f8471af3fc5f3102039cf02f3aa` |
| `d8shot/u_10_12.gif` | 2,993,006 | 2026-11-05 18:42 | release asset | `b6d13907f72075d6a1c4690234977dc03e6634eddb565da523b2be7b193bb3ad` |
| `d8shot/u_12_10.gif` | 2,856,838 | 2026-11-05 18:42 | release asset | `be7d4729e7c9f54789c1cb21c022b77d6115946c84895ce50aa714201d62521d` |
| `d8shot/uf.png` | 84,960 | 2026-11-05 18:42 | release asset | `6740501cde65eaa72bae842fa889d5b38cd84e64bce54caae11f000149d609b1` |
| `d8shot/wf.png` | 72,330 | 2026-11-05 18:36 | release asset | `70042a7a81042b77e87dc8a4bfcebf355a8e2beac483bd7a7461ca8c4a84c145` |
| `d8shot/wf.webm` | 17,806,912 | 2026-11-05 18:43 | release asset | `faea783585207fe00dbe05d0c21ab2ebde2ea0a280a43c2cea13d9db3c2fbce6` |
| `d8shot/wf30.png` | 199,449 | 2026-11-05 18:37 | release asset | `d8bff3636029e8f0c8d3cee8c2cc33dbda0e075a65eb536f99fe8ec67c37e017` |
| `d8shot/wf30h.png` | 304,808 | 2026-11-05 18:37 | release asset | `939270378b5d0234a37a5754cdfd4488f4a384c5c4d4aeb7c89cf014b8b5cec8` |
| `d8shot/wfa.gif` | 2,168,849 | 2026-11-05 18:43 | release asset | `05166883dd1186be087e698ded581d861837d623f376def4d7524089d24b20c6` |
| `d8shot/wfaf.png` | 95,532 | 2026-11-05 18:43 | release asset | `8e72f29f85dc838042095a40b127fdc6dbfce2c82fce34b8c847171dea7a7c99` |
| `d8shot/wfb.gif` | 3,116,197 | 2026-11-05 18:43 | release asset | `3cb352d11ae0a001ef234288913b4f6dc3070bd4e26b27be66425bf16bcebbc2` |
| `d8shot/wfdemo.log` | 33 | 2026-11-05 18:15 | release asset | `9555a9fe0b667a886444a20db2f01c7c472c4276afb4223edd20dea8db3b7120` |
| `d8shot/wff.png` | 478,262 | 2026-11-05 18:43 | release asset | `f78ea3deaad78fd8363efdf491fe214412fa9a27fbbf377971ce3bdff4a2b43d` |
| `d8shot/wffc.png` | 212,188 | 2026-11-05 18:43 | release asset | `1da4234d8028b83044cfdbd770c5ee72feb4c39bdb6117e29d8f6a10187d8ddd` |
| `d8shot/wfs.png` | 365,128 | 2026-11-05 18:44 | release asset | `23627819c9ff4b92ec544cf45e05ab68fc8569e1cbcdf3b5780247f4fd49c2a8` |
| `d8shot/wfs2.png` | 365,128 | 2026-11-05 18:44 | release asset | `23627819c9ff4b92ec544cf45e05ab68fc8569e1cbcdf3b5780247f4fd49c2a8` |
| `d8shot/wfs_p.png` | 440,983 | 2026-11-05 18:44 | release asset | `a0ce1090565ac654ca452dfdb2126f5fd77ec7c90034cd8bd6c995c028bca65c` |
| `follow_ws.json` | 1,035,415 | 2026-11-05 18:39 | release asset | `6444b40e9f00a7b83911f29c45a56c467f40bd6dd0e07fb8ab783b6ce6ae0ecd` |
| `retune-in-stream.txt` | 2,113 | 2026-11-05 17:52 | in repo | `c6c0d4053d470b663e8775bd32b3830346c0c095ec579969c33c61d8fd9b9178` |
| `test_ws.json` | 103,959 | 2026-11-05 18:38 | in repo | `70cb5c47c4775bf7788b672927a29d1a53a638f7c1fe39e72789b025a8cffe39` |
