#!/usr/bin/python3.8

from dataclasses import dataclass
from collections import defaultdict
import os
from datetime import datetime
import time


@dataclass
class TestResult:
    permutation: str
    program: str
    status: str


def parseKnownFailingTests(path):
    knownFails = defaultdict(list)
    totalKnownFails = 0

    with open(path, 'r') as f:
        for line in f:
            line = line.strip()
            if line:
                test = line.split(' ')[0].split('/')
                permutation = test[0]
                program = test[1]
                knownFails[permutation].append(program)
                totalKnownFails += 1

    return knownFails, totalKnownFails


def getStatus(programPath):
    if not os.path.exists(programPath):
        return "passed"
    elif os.path.exists(os.path.join(programPath, "is_running.txt")):
        return "running"
    elif os.path.exists(os.path.join(programPath, "run_me.sh")):
        return "failed"
    else:
        return "running"  # actually waiting to run


def getResults(basePath):
    programNames = sorted([
        d for d in os.listdir(os.path.join(basePath, 'regression'))
        if os.path.isdir(os.path.join(basePath, 'regression', d))
    ])
    permutationDirs = [
        d for d in os.listdir(basePath)
        if d.startswith('regression_') and os.path.isdir(d)
    ]

    results = []

    for permutationDir in permutationDirs:
        permutationPath = os.path.join(basePath, permutationDir)
        if not os.path.isdir(permutationPath):
            continue

        for programName in programNames:
            status = getStatus(os.path.join(permutationPath, programName))
            results.append(TestResult(permutationDir, programName, status))

    return results


def setFailTypes(results, knownFails):
    for result in results:
        if result.status == "failed":
            if result.program in knownFails[result.permutation]:
                result.status = "expected_failed"
            else:
                result.status = "new_failed"


def filterStatus(results, status):
    return [result for result in results if result.status == status]


def getRunStartTime(basePath):
    return os.path.getmtime(
        os.path.join(basePath, "condor/regression.con_0.con"))


def formatTimestamp(t):
    return datetime.fromtimestamp(t).strftime('%H:%M')


def writeNewFails(newFailPath, results):
    with open(newFailPath, 'w') as f:
        for result in filterStatus(results, "new_failed"):
            print(f"{result.permutation}/{result.program}", file=f)


def makeFmtWithPercentOfTotal(total):
    return lambda n: f"{n:>5}\t({n/total:.2%})"


def makeColorizer(color):
    return lambda s: f"{color}{s}\033[0m"


def displayResults(results, numKnownFails, newFailPath):
    numTotalTests = len(results)
    if numTotalTests == 0:
        print("There are no current test results.")
        return

    numAndPercent = makeFmtWithPercentOfTotal(numTotalTests)
    red = makeColorizer('\033[1;31m')
    green = makeColorizer('\033[0;32m')

    numPassed = len(filterStatus(results, "passed"))
    numRunning = len(filterStatus(results, "running"))
    numExpectedFailed = len(filterStatus(results, "expected_failed"))

    newFailed = filterStatus(results, "new_failed")
    numNewFailed = len(newFailed)

    startTime = getRunStartTime(".")
    percentDone = 1 - (numRunning / numTotalTests)
    currentTime = time.time()
    elapsedTime = currentTime - startTime
    projectedDuration = (elapsedTime / percentDone)
    projectedEndTime = startTime + projectedDuration
    projectedRemaining = projectedEndTime - currentTime
    projectedRemainingHours, rem = divmod(projectedRemaining, 3600)
    projectedRemainingMinutes, _ = divmod(rem, 60)

    print(f"There are {numTotalTests} regression tests")
    print(f"  {numAndPercent(numKnownFails)} are known to fail")
    print(f"\nThe current run started at {formatTimestamp(startTime)}")
    print(f"Projected end time is {formatTimestamp(projectedEndTime)}",
          end=" ")
    print(
        f"({int(projectedRemainingHours)}h{int(projectedRemainingMinutes)}m)")

    print(f"\nTests running:\t\t {numAndPercent(numRunning)}")
    print(f"Tests failed (known):\t {numAndPercent(numExpectedFailed)}")
    print(f"Tests passed:\t\t {numAndPercent(numPassed)}")
    if numNewFailed == 0:
        print(f"\nNo new tests failed. {green('Good to go! (^.^)')}")
    else:
        print(
            f"\n{red('Tests failed (new):')}\t{red(numAndPercent(numNewFailed))}"
        )
        print(
            f"They have been written to {red(newFailPath)}\nGood luck! (x_x)")


if __name__ == "__main__":
    knownFails, numKnownFails = parseKnownFailingTests(
        "regression/failing_tests")
    results = getResults(".")
    setFailTypes(results, knownFails)
    newFailPath = os.path.join(".", "new_failing_tests.txt")
    writeNewFails(newFailPath, results)
    displayResults(results, numKnownFails, newFailPath)
