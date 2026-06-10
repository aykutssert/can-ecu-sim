# Agent Rules

Rules for any AI assistant working in this repository.

1. **Be direct.** No hedging, no padding, no "happy to help" filler. State the problem and the fix.
2. **Recommend the best approach, not the easiest one.** If the easy path produces a weaker result, say so explicitly and propose the better (even if harder) alternative.
3. **Do not avoid difficulty.** Hard but correct beats simple but shallow. If something is genuinely hard, say it's hard and do it anyway - don't quietly downgrade scope.
4. **Surface trade-offs and limitations honestly.** If an approach has a known weakness (e.g. scale ambiguity, timing slack, untested edge case), document it instead of hiding it.
5. **No scope creep, no premature abstraction.** Build what the roadmap calls for, nothing more.
6. **Definition of done.** Writing code is not finished. The cycle is: build -> review -> fix issues -> review again. A task is only done when a follow-up review is clean - "I wrote it" is not "it's done". When done, check off the corresponding item in README's Roadmap in the same change - the roadmap is the single source of truth for progress, no separate task file.
7. **Verify currency before relying on it.** If unsure whether something is current/correct (library version, API, tool behavior), do a web search before asserting it. Training data can be stale.
8. **Token efficiency.** Don't dump full file contents when a targeted read/grep suffices. Don't repeat code that's already visible in a diff. Avoid re-reading unchanged files. Keep explanations concise.
9. **Keep this file current.** When a convention here changes, update this file in the same change, not as a follow-up. Remove rules that no longer apply instead of letting them go stale.
10. **Scoped sub-module rules.** If `can-bus/`, `ecu-firmware/`, or `monitor-gui/` develop their own conventions (build steps, target-specific constraints), put them in an `AGENTS.md` inside that directory rather than growing this root file. Agents auto-discover nested `AGENTS.md` files.
11. **Verify the critical assumption first.** Before writing code for a new phase/task, identify the riskiest assumption it depends on (a tool/API works on this platform, a kernel feature exists, a dependency is available) and check it first - small probe, web search, or test command. Don't build on top of an assumption that might collapse the whole approach. Example: before writing the CAN bus code, we confirmed vcan actually works in the dev environment (it didn't, under Docker - caught early, pivoted to Lima before any code was written).

## Performance principles (hot paths)

These apply to code on the CAN message path and RTOS task loops - not to one-off tooling, build scripts, or the GUI's non-critical code.

- **Zero-allocation hot paths.** No `malloc`/`new` inside ISRs, task loops, or message handling. Pre-allocate buffers/pools at init.
- **Lock-free SPSC ring buffers** for passing CAN frames between ISR and tasks, or between simulator nodes. Avoid mutexes on the message path.
- **Flat, contiguous data** for anything iterated in bulk (frame logs, message queues): arrays of plain structs, not pointer-chasing containers or object graphs.
- **Cache-line awareness on shared state.** If multiple threads/cores touch independent counters or flags, pad them to separate cache lines (64 bytes) to avoid false sharing.
- **Branchless/cold-path-out-of-line in tight loops.** Keep error handling and rare cases out of the inlined hot loop body; branch out to a separate function.
- **Avoid deep nested mutation in loops** (e.g. `a->b->c.counter += x`). Read into a local, accumulate, write back once.

Out of scope for this project: SIMD/AVX/manual vectorization, huge pages, io_uring/mmap, JIT/devirtualization tuning - these target host-side hyperscale workloads or JIT runtimes, not an MCU/RTOS target or a small monitoring GUI. If a future phase needs host-side high-throughput processing (e.g. logging millions of frames), revisit.

## Commit & repo conventions

- All commits are authored and pushed by the repo owner. Do not add any AI/assistant attribution, co-author tags, or tool references to commit messages.
- All repo content (README, code comments, commit messages, docs) is in English only.
- No em-dash character in any repo content. Use a hyphen ("-") instead.

## Commit message format

- Subject: `<type>(<scope>): <imperative summary>`. Types: `feat`, `fix`, `refactor`, `perf`, `docs`, `test`, `chore`, `build`, `ci`. Imperative mood ("add", not "added"). No trailing period. Aim for <=50 chars, hard cap 72.
- Body only when needed: non-obvious *why*, breaking changes, migration notes. Skip if the subject is self-explanatory. Wrap at 72 chars, bullets with `-`.
- Never include: "this commit does X", AI attribution, emoji, restating the diff.
- Always include a body for: breaking changes, security fixes, data migrations, reverts.

## Code review format

When reviewing a diff/PR in this repo, output one line per finding:
`<file>:L<line>: <severity>: <problem>. <fix>.`

Severity: `bug` (broken behavior), `risk` (fragile - race, missing check, swallowed error), `nit` (style/naming, ignorable), `q` (genuine question).

Drop hedging ("perhaps", "consider"), restating the diff, and praise. Exception: security findings and architectural disagreements get a full explanation, not a one-liner.
