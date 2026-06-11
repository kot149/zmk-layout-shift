# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

For detailed project overview, features, and usage instructions, please refer to [README.md](README.md).

## ZMK Development Guidelines

**CRITICAL PREREQUISITES - MANDATORY BEFORE ANY IMPLEMENTATION CHANGES:**

1. **Consult the ZMK Repository Expert**: Query the Deepwiki MCP server (repoName: `zmkfirmware/zmk`) to understand the current implementation and identify potential issues.

2. **Consult the Zephyr Repository Expert**: Since ZMK is Zephyr-based, also query the Deepwiki MCP server (repoName: `zephyrproject-rtos/zephyr`) for Zephyr-specific implementation details when needed.

3. **Share Context**: Provide the MCP server with:
   - Current implementation details
   - Identified problems or requirements
   - Your proposed implementation approach

4. **Validate Implementation Strategy**: Confirm your implementation plan with the ZMK expert before proceeding.

5. **Update Documentation**: After making any changes, always update the README.md with relevant information about new features, usage instructions, or implementation status.

**Why This Process is Essential:**
- **Pre-trained Knowledge Limitations**: LLM training data may not reflect the latest ZMK architecture, APIs, or best practices
- **Build Failure Prevention**: ZMK has specific conventions and dependencies that may not be obvious from general knowledge
- **Implementation Accuracy**: The ZMK codebase has evolved significantly, and outdated approaches will likely fail

**NO ASSUMPTIONS**: Always verify current ZMK practices before implementation. What worked in older versions may not work in current releases.

**EXPERT FIRST**: Consult `zmkfirmware/zmk` and `zephyrproject-rtos/zephyr` repository experts before coding, not after encountering build failures.

## Build Verification

**MANDATORY BUILD CHECK**: After any implementation or modification, verify that the build passes by running the following command:

```bash
just build roBa_R -S zmk-usb-logging
```

If build fails, fix the issue and try again until it passes.

Important: Build may take several minutes to complete. Make sure to wait for the build to finish completely before determining if the build is successful.

## Test

Tests are located under `tests/` and use ZMK's native_posix_64 test framework. Each test case consists of `native_posix_64.keymap` (keymap and mock event definitions), `events.patterns` (sed patterns for log filtering), and `keycode_events.snapshot` (expected output).

Run from the workspace root (`zmk-workspace/`):

```bash
# Run a single test
ZMK_EXTRA_MODULES="$(pwd)/modules/zmk-layout-shift" just test modules/zmk-layout-shift/tests/layout-shift/<test-name>

# Auto-accept snapshot (when adding or updating tests)
ZMK_EXTRA_MODULES="$(pwd)/modules/zmk-layout-shift" just test modules/zmk-layout-shift/tests/layout-shift/<test-name> --auto-accept

# Re-run without rebuilding (when no code changes were made)
ZMK_EXTRA_MODULES="$(pwd)/modules/zmk-layout-shift" just test modules/zmk-layout-shift/tests/layout-shift/<test-name> --no-build
```

**Test result interpretation**: `just test` succeeds silently — if the `diff` at the end produces no output and exit code is 0, the test passed. To see the actual keycode events, add `--verbose`. To confirm pass/fail explicitly, add `--verbose` or check the exit code.

**Running all tests**:

```bash
cd zmk-workspace
for t in modules/zmk-layout-shift/tests/layout-shift/*/; do
  ZMK_EXTRA_MODULES="$(pwd)/modules/zmk-layout-shift" just test "$t" --no-build --verbose
done
```

**Writing new tests**: Write the expected `keycode_events.snapshot` by hand based on HID keycode analysis, then run the test to verify. Alternatively, run with `--auto-accept` to generate the snapshot from actual output, then review the generated snapshot for correctness.

## Branch Strategy

- For compatibility for user's west.yml, no breaking changes should be made without updating the branch name.
- Current stable branch is `v1`.
- When making breaking changes, bump the branch name to `v2`.
- For changes with no breaking changes, new branches should be created from `v1` branch with prefix `v1-`.
