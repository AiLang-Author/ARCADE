# Arcade progression plan

Inspired by Galaga, not a clone. Mix-match formations already ship.
Cross off features in order. Intro cinematic is parked.

## Locked decisions

- Small ships stay 1-tap. Difficulty is **mix + armor + heat**, not global HP.
- Enemy steel plate is a **modifier** (ring, grey when fully plated), not a new kind.
- Hero shield is **your** armor: 2 pips (full / half / gone), then hull.
- Boss every **10** waves: free-roam mothership ~4× a bee, escorts, emits with a cap.
- Extra lives: score is reliable; drops are jackpots; cap **5** lives.
- Capture craft is sparse; dual fighter is the toy; last life cannot be captured into a rescue.
- Intro scene: **later** (story locked below).
- Carrier as a later mechanic, not F6.

## Story (intro spine)

The hive opened a **tear near Earth** and pulled a small fleet through — including a **carrier**. You are a **fighter off that carrier**. The tear dumped you into **her galaxy**. She **holds the wound shut** and farms the fleet as **food**. The heroes are trying to **get home**.

How it maps:

| Beat | In-game |
|---|---|
| Tear | Warp/portal each wave (SVG/TVG strip) |
| Fighter off the carrier | Current hero ship; dual = a recovered wingman |
| Trapped as food | Capture craft hauls you toward the hive |
| Queen holds the wound | Decade queen; killing her is a step toward the tear |
| Get home | Long-term arc; carrier comes back as a later mechanic |

Title can be the idle rift + a few lines of this, then Enter = first pull. Wave spit = hive coughing food into the field.

## Warp / portal (next systems, after intro talk)

- TVG/SVG 8-frame rift strip (swirl → pull → spit → collapse).
- Player pulled through between waves.
- Enemies/queen emit from the mouth into HOME formation (replaces or shares side-gate convoys).
- Queen decades: bigger/tinted tear.
- Random queen palettes can tint the spit-frames.

## Hull / plate (F2)

| Class | Hull | Plate later |
|---|---|---|
| Scout / bee / dart / butter | 1 | some get 1 shell |
| Boss (mid) | 2 | shell more often |
| Heavy | 2–3 | shell common |
| Flag | 3 | often shelled |
| Stage boss | 12–20, +4/decade | always |

Hit order on enemies: **plate first, then hull**. Max ~1/3 of small ships plated even at high wave. Remove the old `wave>6` / `agg>=6` HP bumps once plate ships.

Steel look: ring while plate > 0; whole ship tints steel when plate == remaining HP; after last plate, original colors for the hull tap.

## Decades

| Wave | Field | Plate | Heat |
|---|---|---|---|
| 1–3 | small, almost no plate | 0% | 1–3 |
| 4–6 | heavies show | ~10% mid-size | 3–5 |
| 7–9 | mixed, some plated bees | ~25% | 5–7 |
| **10 / 20 / 30…** | boss stage | escorts plated | 7–9 |
| 11–19 | same mix-match, dice shift up one notch | | |

## Hero shield (F1)

- Pips **2 / 1 / 0** = full / 50% / gone. Next hit at 0 is hull (death or capture).
- Start and respawn at **full**.
- After last hit: **~2.2s** (132 ticks) then +1 pip if already at 1. From **0**, **~4s** (240 ticks) to half.
- Cyan ring around the ship; half = broken/flicker; gone = no ring.
- **Shield drop** instant full. Already full → small score bump, no stack.
- One shield drop on screen.

## Drops (F1 table, F3 adds 1-up)

Only **plated / heavy / flag / boss** (F1: heavy / flag / current BOSS kind). Never bees. ~10% of those kills.

| Roll | F1 | F3+ |
|---|---|---|
| common | shield | shield |
| uncommon | score nugget | score nugget |
| rare | — | 1-up (~15% of drop table) |

Fall slowly, scoop with AABB, expire off the bottom. One 1-up on screen when that exists.

## Extra lives (F3)

- **20k**, then every **+40k** (20 / 60 / 100 / …).
- Max **5** lives.
- Drop 1-up is the jackpot, not the farm.

## Capture (F4)

- Own silhouette, slow, obvious beam wind-up. **At most one** on the field.
- None on waves 1–4. After that ~one every 2–3 waves; one escort on boss decades.
- Beam ~1s; shoot capturer to break it.
- Connect: ship docks, **life −1**, respawn empty shield.
- Capturer leaves out the top.
- Kill before escape: **dual/wingman** + **life +1** (tax refunded). Net lives same, firepower up.
- Escape: life gone, no dual.
- Already dual: refund life only, no triple.
- Last life: capture is death, no rescue spawn.
- Dual dies with you; you come back single.

## Stage boss (F5)

- Not a fat FLAG in the grid. Free-roam over the top third.
- ~4× bee (~180–220 units wide).
- Escorts = normal mix-match convoy.
- Emits scout/bee every ~90 ticks, **max 5** live children.
- Slow fat bolts, not bullet hell.
- Wave clears only when every foe is gone (queen, escorts, drones).
- HP 96 on decade 1 (72+24), +24 each decade; 6 steel plate first.
- Mothership does **not** tractor; a capturer may ride with escorts.

## HUD

- WAVE, MIX/stamp name, HEAT, LIVES, SCORE stay.
- F1: shield pips.
- F3: extra-life ding.
- F5: boss HP when present.

## Feature checklist

- [x] **F1** Hero shield pips + regen + HUD pips + shield/nugget drop
- [x] **F2** Enemy steel plate (ring → grey when fully plated); drop table includes plated
- [x] **F3** Score extra lives (20k, +40k, max 5) + rare 1-up drop
- [x] **F4** Capture craft + dual recover
- [x] **F5** Decade boss (wave 10/20/…) that emits
- [x] **F6** Warp/portal (SVG/TVG) — pull hero, spit hive
- [x] **Later** Intro crawl on the idle rift
- [ ] **Later** Carrier mechanic (fleet home)

## Build notes

AILANG: flatten nested calls, ≤6 args. Kernel owns pixels; host plays audio (`sfx.bin`). New kinds/fields go in `App/State.ailang`. Player shield state lives on `Arcade` (not a 17th entity field).
