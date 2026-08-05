# Real tests, not report placeholders

This folder is a **separate** Qt project - it doesn't touch `NepTrek.pro`
or your main build, so there's zero risk of it breaking the app. It
exists purely to make Table 4.1 and Table 4.2 in the report true.

## Running it

```bash
cd tests
qmake tests.pro
make
./tests
```

## What to do with the output

### Table 4.2 (test scenarios)
Every row in that table now corresponds to an actual `QCOMPARE`/`QVERIFY`
check in `test_routeoptimizer.cpp`:

| Report row | Test function |
|---|---|
| TC-01 Password hashing | `test_TC01_PasswordHashingIsSha256` |
| TC-02 Offline fallback | `test_TC02_OfflineFallbackPopulatesBackend` |
| TC-03 AMS elevation limit | `test_TC03_AmsWarningTriggersOnSteepAscent`, `test_TC03_NoAmsWarningOnGentleRoute` |
| TC-04 Budget filter logic | `test_TC04_RouteBudgetPruning`, `test_TC04_SpotBudgetFilterExcludesExpensiveSpots` |
| TC-05 Cyclic graph handling | `test_TC05_CyclicGraphTerminatesWithCorrectPath` |

Run `./tests` and you'll get real `PASS`/`FAIL` lines for each. If a
professor asks "show me the test," you now have one to show.

### Table 4.1 (benchmarks)
`benchmarkDijkstraScaling` is a `QBENCHMARK`-based, data-driven test
that runs Dijkstra across synthetic graphs of 20 / 100 / 500 / 1,000 /
5,000 / 10,000 nodes - the exact sizes the report table claims to have
tested. Qt prints a real measured line per size, e.g.:

```
RESULT : TestRouteOptimizer::benchmarkDijkstraScaling():"1000":
    0.083 msecs per iteration (total: 83, iterations: 1000)
```

Copy those lines' timings into Table 4.1 in place of the current
numbers. They'll come out different from what's in the report now -
that's expected and correct, since the ones in the report right now
were never actually measured on any machine.

Note: this benchmark measures **execution time** honestly. Peak memory
(the second column in Table 4.1) isn't something QtTest measures for
you out of the box - if you want a real number there too, the
simplest honest option is to either drop that column, or measure it
separately with a tool like `valgrind --tool=massif` (Linux) or
Instruments (macOS) and report what it actually shows.
