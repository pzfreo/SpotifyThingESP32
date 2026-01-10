# Testing Strategy for SpotifyThingESP32

## Overview

This document outlines a multi-layered testing approach for the SpotifyThingESP32 project. Since this is an embedded system, we'll use a combination of:

1. **Native Unit Tests** - Run on your development machine (no hardware needed)
2. **Mock-based Integration Tests** - Test component interactions
3. **Simulation Tests** - Using roo_testing emulator (optional)
4. **Hardware Tests** - On-device validation (manual + automated)

---

## Test Framework Selection

### PlatformIO Native Testing
- **Framework**: Unity (built into PlatformIO) + GoogleTest for more complex tests
- **Advantage**: Runs on host machine, fast iteration
- **Setup**: `pio test -e native`

### Why This Approach?
- No need for physical hardware during development
- Fast feedback loop (seconds vs minutes for flashing)
- Easy CI/CD integration (GitHub Actions)
- Pure logic testing at full speed

---

## Phase 1: Extract Testable Logic

### 1.1 Pure Functions to Extract

| Function | Current Location | New Location | Testable |
|----------|-----------------|--------------|----------|
| Time formatting | `updateDisplay()` | `src/utils/time_utils.h` | ✓ |
| Progress bar width | `updateDisplay()` | `src/utils/progress_utils.h` | ✓ |
| Volume clamping | `spotifyTask()` | `src/utils/volume_utils.h` | ✓ |
| Hex formatting | `gen_random_hex()` | `src/utils/string_utils.h` | ✓ |
| Seek logic | `spotifyTask()` | `src/logic/seek_logic.h` | ✓ |
| Combo detection | `loop()` | `src/logic/combo_detector.h` | ✓ |
| State comparison | `updateDisplay()` | `src/logic/state_diff.h` | ✓ |

### 1.2 Interfaces for Mocking

| Component | Interface | Mock |
|-----------|-----------|------|
| Display | `IDisplayManager` | `MockDisplayManager` |
| HTTP/API | `ISpotifyAPI` | `MockSpotifyAPI` |
| Buttons | `IButtonHandler` | `MockButtonHandler` |
| Storage | `IPreferences` | `MockPreferences` |
| Time | `ITimeProvider` | `MockTimeProvider` |

---

## Phase 2: Test Categories

### 2.1 Unit Tests (No Hardware)

```
test/
├── test_native/                    # Native platform tests
│   ├── test_time_utils/
│   │   └── test_time_formatting.cpp
│   ├── test_progress_utils/
│   │   └── test_progress_bar.cpp
│   ├── test_volume_utils/
│   │   └── test_volume_control.cpp
│   ├── test_string_utils/
│   │   └── test_hex_format.cpp
│   ├── test_seek_logic/
│   │   └── test_seek_decision.cpp
│   ├── test_combo_detector/
│   │   └── test_button_combos.cpp
│   └── test_state_diff/
│       └── test_state_comparison.cpp
```

### 2.2 Integration Tests (With Mocks)

```
test/
├── test_integration/
│   ├── test_display_rendering.cpp   # Mock DisplayManager
│   ├── test_spotify_commands.cpp    # Mock HTTP responses
│   ├── test_task_logic.cpp          # Mock timers + API
│   └── test_state_machine.cpp       # Full state transitions
```

### 2.3 Embedded Tests (On Device)

```
test/
├── test_embedded/
│   ├── test_display_hardware.cpp    # Actual display operations
│   ├── test_wifi_connection.cpp     # Network connectivity
│   └── test_button_hardware.cpp     # Physical button reading
```

---

## Phase 3: Test Cases

### 3.1 Time Formatting Tests

| Test Case | Input | Expected Output |
|-----------|-------|-----------------|
| Zero progress | (0, 180000) | "00:00 / 03:00" |
| Halfway | (90000, 180000) | "01:30 / 03:00" |
| Near end | (179000, 180000) | "02:59 / 03:00" |
| Long track | (3600000, 7200000) | "60:00 / 120:00" |
| Zero duration | (100, 0) | "00:00 / 00:00" |

### 3.2 Progress Bar Tests

| Test Case | Progress | Duration | Width | Expected |
|-----------|----------|----------|-------|----------|
| Empty | 0 | 100000 | 480 | 0 |
| Half | 50000 | 100000 | 480 | 240 |
| Full | 100000 | 100000 | 480 | 480 |
| Over 100% | 120000 | 100000 | 480 | 480 (clamped) |
| Zero duration | 5000 | 0 | 480 | 0 |

### 3.3 Volume Control Tests

| Test Case | Current | Delta | Expected |
|-----------|---------|-------|----------|
| Normal increase | 50 | +10 | 60 |
| Normal decrease | 50 | -10 | 40 |
| Clamp at max | 95 | +10 | 100 |
| Clamp at min | 5 | -10 | 0 |
| Already at max | 100 | +10 | 100 |
| Already at min | 0 | -10 | 0 |

### 3.4 Seek Logic Tests

| Test Case | Progress | Is Playing | Elapsed | Should Seek |
|-----------|----------|------------|---------|-------------|
| Early in track (paused) | 5000 | false | 0 | false |
| Early (playing) | 5000 | true | 1000 | false |
| Boundary (9.9s) | 9000 | true | 900 | false |
| Just over (10.1s) | 9100 | true | 1000 | true |
| Well into track | 30000 | true | 0 | true |

### 3.5 Combo Detection Tests

| Test Case | Hold Time (ms) | Expected Action |
|-----------|----------------|-----------------|
| Quick tap | 500 | NONE |
| Short hold | 1500 | NONE |
| Logout range start | 2001 | LOGOUT_PENDING |
| Logout range | 5000 | LOGOUT_PENDING |
| Reset range start | 10001 | RESET_PENDING |
| Reset range | 15000 | RESET_PENDING |
| Factory reset | 20001 | FACTORY_RESET |

### 3.6 State Comparison Tests

| Test Case | Old State | New State | Expected Diff |
|-----------|-----------|-----------|---------------|
| Track change | "Song A" | "Song B" | trackChanged=true |
| Volume change | vol=50 | vol=60 | volumeChanged=true |
| Play state | playing | paused | playStateChanged=true |
| No change | same | same | all false |
| Multiple changes | old | new | multiple flags |

---

## Phase 4: Implementation Plan

### Step 1: Create Test Infrastructure (Day 1)
- [ ] Add native test environment to platformio.ini
- [ ] Create test directory structure
- [ ] Set up Unity test framework
- [ ] Create first passing test

### Step 2: Extract Pure Functions (Day 2-3)
- [ ] Create `src/utils/time_utils.h`
- [ ] Create `src/utils/progress_utils.h`
- [ ] Create `src/utils/volume_utils.h`
- [ ] Create `src/utils/string_utils.h`
- [ ] Update SpotifyThing.cpp to use new utils

### Step 3: Write Unit Tests (Day 4-5)
- [ ] Time formatting tests
- [ ] Progress bar tests
- [ ] Volume control tests
- [ ] Hex formatting tests

### Step 4: Create Interfaces for Mocking (Day 6-7)
- [ ] `IDisplayManager` interface
- [ ] `ISpotifyAPI` interface
- [ ] `ITimeProvider` interface
- [ ] Mock implementations

### Step 5: Integration Tests (Day 8-10)
- [ ] Display rendering tests with mock
- [ ] API command tests with mock responses
- [ ] State machine tests

### Step 6: CI/CD Setup (Day 11)
- [ ] GitHub Actions workflow
- [ ] Automated test runs on PR
- [ ] Coverage reporting

---

## Phase 5: CI/CD Integration

### GitHub Actions Workflow

```yaml
name: PlatformIO CI

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/cache@v4
        with:
          path: |
            ~/.cache/pip
            ~/.platformio
          key: ${{ runner.os }}-pio
      - uses: actions/setup-python@v5
        with:
          python-version: '3.11'
      - name: Install PlatformIO
        run: pip install platformio
      - name: Run Native Tests
        run: pio test -e native
      - name: Build Firmware
        run: pio run
```

---

## Phase 6: Coverage Goals

### Target Coverage by Component

| Component | Target | Priority |
|-----------|--------|----------|
| Utils (pure functions) | 95% | High |
| Logic (state machines) | 80% | High |
| API integration | 70% | Medium |
| Display rendering | 60% | Medium |
| Hardware interaction | Manual | Low |

### Metrics to Track
- Line coverage
- Branch coverage
- Test execution time
- Flaky test rate

---

## Tools & Resources

### Required
- PlatformIO Core
- Unity Test Framework (included with PlatformIO)

### Optional
- GoogleTest (for more advanced mocking)
- gcov/lcov (coverage reports)
- GitHub Actions (CI/CD)

### Useful Commands
```bash
# Run all native tests
pio test -e native

# Run specific test
pio test -e native -f test_time_utils

# Run with verbose output
pio test -e native -v

# Generate coverage (requires gcov setup)
pio test -e native --coverage
```

---

## Next Steps

1. Review this plan and adjust priorities
2. Start with Phase 1 (infrastructure setup)
3. Iterate through phases, testing as we go
4. Add more tests as bugs are found/fixed
