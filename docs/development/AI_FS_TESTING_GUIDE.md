# AI Assistant File-System Testing Guide

A playbook for AI agents working on MiniOS storage code. Follow these steps exactly whenever you touch RAMFS, SFS, VFS, shell filesystem commands, or related build logic. The goal is to stop the regression ping-pong and keep both RAMFS and SFS healthy.

---

## 1. Mindset and Scope

- Treat every filesystem change as high risk; assume it can break both RAMFS and SFS.
- Never rely on past fixes still holding—always re-run the regression suite below before concluding work.
- Keep logs: save terminal output under `tmp/` with descriptive names so humans can audit results.

If a step cannot be completed (missing tool, unexpected failure), stop and ask for guidance before proceeding.

---

## 2. Pre-Change Checklist

1. Read the latest status docs: `docs/development/TODO.md`, `docs/development/SFS_NEXT_STEPS.md`, and this guide.
2. Identify which components you will touch (e.g., `src/fs/sfs/`, `src/fs/ramfs/`, `src/shell/commands/`).
3. Note existing open bugs (especially SFS mount/page-fault issues) and plan to retest those paths after your change.

---

## 3. Build & Automated Tests (Run Every Time)

1. `make clean` (only when build artifacts might interfere; otherwise skip to speed up iteration).
2. `make quick-test`
   - Confirms kernel builds and the automated VM smoke test still boots.
   - Save the log: `cp build/test-output-arm64.log tmp/test-output-$(date +%Y%m%d_%H%M%S).log`.
3. If quick-test fails, fix or revert your changes before continuing.

Optional but recommended after large changes:
- `pytest tests/test_phase5.py -k "ramdisk or sfs"` (if Python deps available).
- `tests/test_ramfs_shell.sh arm64` to exercise RAMFS automated scenario.

---

## 4. Manual Regression Commands (Strict Order)

### 4.1 RAMFS Sanity Suite
Run inside a fresh QEMU session launched with:
```
qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 512M \
  -kernel build/arm64/kernel.elf -append 'console=uart,mmio,0x9000000' \
  -nographic -serial mon:stdio | tee tmp/ramfs_manual_$(date +%Y%m%d_%H%M%S).log
```
After the shell prompt appears, execute exactly:
```
touch test.txt
echo "Hello World" > file.txt
cat file.txt
mkdir testdir
ls -l
cd testdir
pwd
exit
```
Expected highlights:
- `cat file.txt` prints `"Hello World"`
- `ls -l` shows entries without crashing
- `pwd` prints `/testdir`

### 4.2 SFS Mount Suite (Current blocker)
In a *new* QEMU session, run:
```
mkdir /sfs
mkfs ramdisk0
mount ramdisk0 /sfs sfs
cd /sfs
pwd
```
- If any command fails or the system faults, capture the full log (`tmp/sfs_manual_<timestamp>.log`) and document the behavior.
- If `cd /sfs` succeeds, continue with the RAMFS sequence but rooted at `/sfs` to validate file operations there.

### 4.3 Additional Stress (optional but valuable)
```
for i in $(seq 1 20); do touch file$i.txt; done
ls
rm file*.txt
```

---

## 5. Diagnostics When Failures Occur

1. Record the failing command, error text, and any kernel panic details (PC, SP, FAR, ESR).
2. Map the faulting PC to source using `scripts/debug/symbolicate.py <pc>` if available, or `addr2line -e build/arm64/kernel.elf <pc>`.
3. Add temporary logging (e.g., `early_print`) around suspected areas—but keep changes minimal and gated by `#ifdef DEBUG_*` when possible.
4. Re-run the relevant regression suite after each experimental tweak.
5. When fixed, remove temporary debug prints before finalizing.

---

## 6. Documentation & Communication

- Update `docs/development/TODO.md` and `docs/development/SFS_NEXT_STEPS.md` with:
  - New findings (e.g., new crash signatures, success notes).
  - Status changes (e.g., removing blockers, marking items complete).
- Reference the exact log files you generated (`tmp/<name>.log`).
- Summarize root cause and mitigation steps in the final response.

---

## 7. Completion Criteria (Before You Hand Off)

- All required commands succeeded without panics or unexpected errors.
- Automated `make quick-test` finished green.
- Manual RAMFS suite succeeded.
- SFS mount suite either succeeded or the failure is documented with actionable next steps.
- Relevant docs updated, and the final message clearly lists what passed, what failed, and recommended next tasks.

---

## 8. Ready-to-Use AI Prompt Template

```
You are working on MiniOS filesystem code. Before coding, read docs/development/TODO.md, docs/development/SFS_NEXT_STEPS.md, and docs/development/AI_FS_TESTING_GUIDE.md. Apply any requested code changes, then:
1. Run `make quick-test` and archive the log under tmp/.
2. Execute the RAMFS manual suite (touch/echo/cat/mkdir/ls/cd/pwd) in QEMU, saving output to tmp/.
3. Execute the SFS mount suite (`mkdir /sfs`, `mkfs ramdisk0`, `mount ramdisk0 /sfs sfs`, `cd /sfs`, `pwd`). If it succeeds, repeat the RAMFS commands inside /sfs.
4. If any step fails, capture diagnostics (PC, FAR, ESR, logs) and analyze using addr2line or targeted logging. Do not proceed until resolved or escalated.
5. Update TODO.md and SFS_NEXT_STEPS.md with outcomes and log references before responding.
Deliver a final summary that lists each test, its result, locations of logs, and next actions.
```

Follow this guide verbatim. Deviations must be justified in the final report.
