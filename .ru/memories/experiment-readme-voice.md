---
name: experiment-readme-voice
description: Read before writing or editing experiment READMEs, the experiments index, DATA.md files or other docs under experiments/ (owner's voice rules, 2026-11-09).
load: description
---
The owner's voice rules for experiment docs (given 2026-11-09 during the experiments restructure):

- Narrative parts (experiment README story, index sentences): first person singular "I", calm and clear, like someone explaining something complex (Feynman). Superseded on 2026-11-09: no "What I did / What happened / What it meant" headings and no "at most 2–3 numbers" limit. The story must be technical: the mechanism and the actual measurements (counts, rates, frequencies, timings, settings) in the prose, with the reasoning that connects them, in the order things happened, saying what was known then vs learned later and what the result does not show. No slogans, no vague summaries without numbers, no hype, no exclamation marks. Story, then figures with plain-label captions, then `---` and the reference footer. Approved samples: esp32-d8tx experiments 05–07 READMEs; full rules in the esp32-d8tx `docs-voice` memory.
- Data descriptions (DATA.md, table headers, figure captions, log headers): no person, factual labels ("Capture: 2412 MHz, 60 s, 1-bit samples").
- How to re-run: imperative commands. "Ran: dates, commits" stays a short factual footer.
- Never edit raw data or original logs.

**How to apply:** any new or changed text under `experiments/` in this repo or esp32-d8tx.
