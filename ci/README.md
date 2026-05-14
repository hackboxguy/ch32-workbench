# CI helper scripts

Scripts here are invoked by `.github/workflows/ci.yml` AND are runnable
locally so contributors can sanity-check before opening a PR.

| Script | Purpose |
|---|---|
| `size-report.sh` | Builds every (app × board), captures `riscv64-unknown-elf-size` output, prints a markdown table. In CI the table is appended to `$GITHUB_STEP_SUMMARY`. Exits non-zero if any combination fails to build. |

Run any of them from the repo root:

```bash
./ci/size-report.sh
```

All scripts are shellcheck-clean; CI re-runs `shellcheck` over them in
the lint job.
