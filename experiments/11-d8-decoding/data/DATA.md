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
| `centred-decode/c031_m2.sigmf-data` | 19,999,728 | 2026-11-05 16:55 | release asset | `622a04cea1de852f5c6c6e9bab5f054ec2f7c68d0182d9edbf3466153a087c40` |
| `centred-decode/c031_m2.sigmf-meta` | 722 | 2026-11-05 16:55 | in repo | `0eafc350e77690ad2d4bee9c3526eededfd7f69cf2ded1ed618f130ef3fa6817` |
| `centred-decode/cap/lo2417_f140.npz` | 65,524,490 | 2026-11-05 16:55 | release asset | `92617eeef9a7525cd55bc02f9268d2a1c0badece4c03974b1bf0a9c618e55ad5` |
| `centred-decode/cap/lo2417_f140_ana.npz` | 12,996 | 2026-11-05 16:55 | in repo | `43cebac432a9b911abd7956070ac302a29ac4aaf147dbf836cecc265dc508d05` |
| `centred-decode/cap/lo2420_f0.npz` | 65,524,490 | 2026-11-05 16:50 | release asset | `53c4cd6ffcd01ee20d766f6623ceb7b6a8d4f44d9243ac124dda9272014e5651` |
| `centred-decode/cap/lo2420_f0_ana.npz` | 13,020 | 2026-11-05 16:50 | in repo | `8209643ea56836dcdebbce7ca54cc73c10a4e22f0b8d181eb8b65ce753e22942` |
| `centred-decode/cap/lo2421_f0.npz` | 65,524,490 | 2026-11-05 16:47 | release asset | `2185a46494a4bca28234422de557553a95df6447cacf71a9db73f933e36bf431` |
| `centred-decode/cap/lo2421_f0_ana.npz` | 12,420 | 2026-11-05 16:48 | in repo | `498343712f8f64d5167da85eae6423009ae64645a802d074a5aa7d1c42423f05` |
| `centred-decode/cap/lo2421_f0_avg.npy` | 131,168 | 2026-11-05 16:45 | in repo | `5a44262ad3a1050e72a679e838f975a232894839daaa598a9ee4f53e5f50c394` |
| `centred-decode/cap/lo2436_f140.npz` | 2,621,450 | 2026-11-05 16:51 | release asset | `2e5facd17215642db903df31cbf74f02ba79ad3b7000720c9d058a4082fe1af6` |
| `centred-decode/cap/lo2437_f-140.npz` | 2,621,450 | 2026-11-05 16:51 | release asset | `a8cd13ccc037edb6c667f4e7aed9635f54fce256b4fe6ca7109f73b18918e445` |
| `centred-decode/cap/lo2439_f0.npz` | 2,621,450 | 2026-11-05 16:51 | release asset | `8306c54da696a26e45d9e4897ab14ecee8e2f3251f9b4f2b679263c6e7243431` |
| `centred-decode/cap/lo2441_f0.npz` | 2,621,450 | 2026-11-05 16:51 | release asset | `d1af976f4f1ab7effef066eff59f07a1ea28fd2657c9d581f263e411fd0a220c` |
| `centred-decode/centred_m2.sigmf-data` | 39,999,456 | 2026-11-05 17:03 | release asset | `e895084af447b7942b3fb072867743fa6ec4b5a02aee4f926a38046086f50377` |
| `centred-decode/centred_m2.sigmf-meta` | 501 | 2026-11-05 17:03 | in repo | `7c3f54b1d1d00ecfb837fbf5b9c95d07c5b6711f4934564c03883cd1e7d1a08a` |
| `centred-decode/check.sigmf-data` | 9,999,360 | 2026-11-05 17:06 | release asset | `acb1b752c710acf9c3eafce4e4aa3133ec0f33a0e25e5276f9cda7fcd1d98bb1` |
| `centred-decode/check.sigmf-meta` | 501 | 2026-11-05 17:06 | in repo | `7c3f54b1d1d00ecfb837fbf5b9c95d07c5b6711f4934564c03883cd1e7d1a08a` |
| `centred-decode/g30.sigmf-data` | 11,999,232 | 2026-11-05 17:02 | release asset | `828737bb458af3edc2c3a322ab86b9d078ac68ac78fe4481d863030678bbc5d5` |
| `centred-decode/g30.sigmf-meta` | 501 | 2026-11-05 17:02 | in repo | `59bb370466cd2d3ab683378360efc6dbdd71abfffe8d71d04dfa2b842d656e26` |
| `centred-decode/g50.sigmf-data` | 11,999,232 | 2026-11-05 17:02 | release asset | `6608e1c7310631da632423005ae9a330930a8fde67da5bcc3989cb8638af713f` |
| `centred-decode/g50.sigmf-meta` | 501 | 2026-11-05 17:02 | in repo | `4fecbf2ff86677c6d429483784b398af146d22df83295ec579ec8e363cb42d2d` |
| `centred-decode/g70.sigmf-data` | 11,999,232 | 2026-11-05 17:02 | release asset | `5d9636155128d16846be70e130eea8a7757e271935216bdff322c364b4cf23ef` |
| `centred-decode/g70.sigmf-meta` | 501 | 2026-11-05 17:02 | in repo | `7b85c419e64886cfee806319f350541bdb5092f417cef3354d5ea4e9557e4d99` |
| `centred-decode/g81.sigmf-data` | 11,999,232 | 2026-11-05 17:02 | release asset | `c98fb8d1851465ea49dbeb968778a3ebc119fc1a44ac896430d07924e476ce7f` |
| `centred-decode/g81.sigmf-meta` | 501 | 2026-11-05 17:02 | in repo | `c23a16d65d629b46cf03b030ce0eaa6cbee061394dade1c0e9722256a388c5d6` |
| `centred-decode/r1_2439.990.sigmf-data` | 15,999,984 | 2026-11-05 16:57 | release asset | `cb42de6cffe453d78fc0839f93bf3bf66ce11404044fea7031dacc06331525d1` |
| `centred-decode/r1_2439.990.sigmf-meta` | 723 | 2026-11-05 16:57 | in repo | `d5d141514bc14831ca6a60b0fe569fdbd14229f65c85a4e692f692f3c449a78c` |
| `centred-decode/r1_2440.010.sigmf-data` | 15,999,984 | 2026-11-05 16:57 | release asset | `5ee54e07afa4b51154fbcadef22bcdc99e1e361beb5ed98fe8abdde637059d47` |
| `centred-decode/r1_2440.010.sigmf-meta` | 722 | 2026-11-05 16:57 | in repo | `125a1784c04a01613f4f536ebaa443014e03221b5cf6aa2a101ea146602cb841` |
| `centred-decode/ruler_2439.995.sigmf-data` | 1,249,920 | 2026-11-05 16:57 | release asset | `bf6b9ca8e39f3257fd59e330e77eb68baf1c599698e3e576d264ed0316bb5e97` |
| `centred-decode/ruler_2439.995.sigmf-meta` | 721 | 2026-11-05 16:57 | in repo | `cd2ccd7c70cd8d311afa88bb2f0c0bf54469bd744ebaccd14d6ab4d1688642fb` |
| `centred-decode/ruler_2440.005.sigmf-data` | 1,249,920 | 2026-11-05 16:57 | release asset | `167a0758c73affdccc34fbdabe94ee098e9608c7f556703672eb06ce43f044c9` |
| `centred-decode/ruler_2440.005.sigmf-meta` | 720 | 2026-11-05 16:57 | in repo | `debc49f517479abe11021ce2e01424faa51a9242f58aaab6dbc6e47681366bab` |
| `first-decode/bursts.npy` | 2,016 | 2026-11-05 15:41 | in repo | `eaaba6a6c392790fcd4be448757b14ffe5d9c74a389a06a503c892d2bd359713` |
| `first-decode/fits.pkl` | 82,636 | 2026-11-05 15:59 | in repo | `6b8a0f4351ae466eb08482cec5e16c11412c76c7f956cfd56434e1fd91342d36` |
| `first-decode/mlse_out.txt` | 3,731 | 2026-11-05 16:13 | in repo | `f3e349fbda91a4b705fb1bcf60433ea86230031c1d989c30f339fdb61cb23b8e` |
| `mode2-frequency/c140_m2.sigmf-data` | 15,999,984 | 2026-11-05 16:41 | release asset | `8b5e89e8ece2ddef2e975351d4cca2a6cfac43b37b01f79bfd460fa3b54e7473` |
| `mode2-frequency/c140_m2.sigmf-meta` | 723 | 2026-11-05 16:41 | in repo | `b9c4d1d69e307fb262ac4214f27c5de7d2d840c9a00cbbce87d3aee989947d18` |
| `mode2-frequency/c96_m2.sigmf-data` | 11,999,232 | 2026-11-05 16:41 | release asset | `b91a561aba7d988cfb326365d45d9bc2df04f4af13a76401849de9cd94271c45` |
| `mode2-frequency/c96_m2.sigmf-meta` | 722 | 2026-11-05 16:41 | in repo | `d72485f19c8415ce1be3013a51bc2ded89631be240209040aac0a223c21aa1e7` |
| `mode2-frequency/chk_m2_33.sigmf-data` | 11,999,232 | 2026-11-05 16:36 | release asset | `cb9365c00315a732839a3c46722f323ba7421377927d47bcda77e5a2db436e92` |
| `mode2-frequency/chk_m2_33.sigmf-meta` | 303 | 2026-11-05 16:36 | in repo | `e41e4be9d2d142fdcf5e383c6543d100e42112630fbb14dc833e7dd9826c4f61` |
| `mode2-frequency/chk_m2_96.sigmf-data` | 11,999,232 | 2026-11-05 16:36 | release asset | `4978e1e2c9765105424f977791ad51984f670f84258786eefd59a481967decc6` |
| `mode2-frequency/chk_m2_96.sigmf-meta` | 303 | 2026-11-05 16:36 | in repo | `901c314c26946ed0899277fc2def17231343c2c205b229c95de4577e20fd666e` |
| `mode2-frequency/old_m2.sigmf-data` | 15,999,984 | 2026-11-05 16:41 | release asset | `4cb9ad1680e9a96f463d4ff45a4d7eddb93d3f8df4594af677d218cb280fa47d` |
| `mode2-frequency/old_m2.sigmf-meta` | 722 | 2026-11-05 16:41 | in repo | `a0dfd531e26ab8ad45ef4bb8a11a97d023881cff9d5ac969b91d8246349c209f` |
| `mode2-vs-mode0/hl_m0.sigmf-data` | 39,999,456 | 2026-11-05 16:21 | release asset | `996f08ed42f78957e232dc18048c2629e283a2fb8828b14fa6b45bf45d3252ca` |
| `mode2-vs-mode0/hl_m0.sigmf-meta` | 303 | 2026-11-05 16:21 | in repo | `42ba6aace92033209828fc2f3026e0e04c488feb80c919a142aa7737251a4365` |
| `mode2-vs-mode0/hl_m0_r.npy` | 156,368 | 2026-11-05 16:22 | in repo | `be411ce02276d9c4a61bf983110f9a4dc33fdc631ebfd5e67f7b7b6317faa2d7` |
| `mode2-vs-mode0/hl_m2.sigmf-data` | 39,999,456 | 2026-11-05 16:21 | release asset | `e4a1c51260e83fdfb0219c8f23b6fda07008cf4c9ea6e32f80f32e2d47227551` |
| `mode2-vs-mode0/hl_m2.sigmf-meta` | 303 | 2026-11-05 16:21 | in repo | `e41e4be9d2d142fdcf5e383c6543d100e42112630fbb14dc833e7dd9826c4f61` |
| `mode2-vs-mode0/hl_m2_r.npy` | 156,368 | 2026-11-05 16:22 | in repo | `a02fa24f7d2abcab48cf07d18ac07430ea9f41832300d94d9b3dfa640e8b09d7` |
| `mode2-vs-mode0/m0_out.txt` | 214 | 2026-11-05 16:26 | in repo | `dcf2e1c66a39b4fe26e3d2dc1a0b160dc4ab9be9814c7b240704b96ca1da0842` |
| `mode2-vs-mode0/m2_out.txt` | 454 | 2026-11-05 16:26 | in repo | `add36909998ccaadd41f7aaa7fc25f2fcb5c218f06b7c86b9b61769549b19fb5` |
| `mode2-vs-mode0/union.pkl` | 30,199 | 2026-11-05 16:27 | in repo | `86320b8af8f3339d1ac80b9b096b9ac6088f0f3edca2cc05ea0eedeffac96d2a` |
