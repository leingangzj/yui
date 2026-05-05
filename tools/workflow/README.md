# Phase 4.13 + Phase 5 — workflow protocol

Sequential job queue for the consolidation sweep (4.13.x) and Lab Mode
build-out (5.x). One job at a time. Checkpoint after each completion.

## Files

- `jobs.jsonl` — append-only spec of every job. One JSON object per
  line. Immutable history of intent.
- `checkpoint.json` — mutable cursor. Tracks current job, completion
  log, and any blockers.

## Protocol

For each iteration:

1. **Read** `checkpoint.json` → find `current` job id.
2. **Read** `jobs.jsonl` → grep for that id, parse the spec.
3. **Verify deps** — every id in `depends_on` must appear in
   `completed[]`. If not, halt and surface the blocker.
4. **Execute** the job per its spec:
   - create / edit / delete the listed files
   - migrate listed tests, add new ones
   - run `pio test -e native` and `pio run -e cardputer_adv`
   - all `acceptance` items must pass
5. **Commit** with message format:
   `refactor: Phase <id> — <title>` (or `feat:` for v5.x adds)
6. **Push** to both remotes.
7. **Update** `checkpoint.json`:
   - move `current` into `completed[]` with commit SHA + ISO timestamp
   - set `current` to the next id from `remaining[]`
   - drop that id from `remaining[]`
8. **Stop** and report progress to the operator. Wait for "next" before
   starting the next iteration.

## Resuming cold

Anyone (or any future agent) can resume by reading the two files. The
spec entry is self-contained — file paths, test names, acceptance
criteria, dependencies, effort estimate. No conversation context
required.

## Status format in `checkpoint.json`

```json
{
  "id": "4.13.X",
  "commit": "<short-sha>",
  "completed_at": "<ISO-8601>",
  "tests_before": <int>,
  "tests_after": <int>,
  "launcher_before": <int>,
  "launcher_after": <int>
}
```

Optional fields per job: `notes` (one-line gotcha), `followups` (array
of new ids the work surfaced — append to `jobs.jsonl` if non-empty).

## Editing rules

- `jobs.jsonl` is **append-only after the queue is locked**. If a spec
  is wrong, write a new entry with `id` suffixed `-rev2` and update
  the checkpoint to reference the new id. Never silently rewrite a
  shipped spec.
- `checkpoint.json` is rewritten in place each cycle.
- Both files are under git; every change lands in the same commit as
  the work it describes.
