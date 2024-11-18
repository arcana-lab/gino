# Gino: An (automatic) parallelizing compiler

[![Active Development](https://img.shields.io/badge/Maintenance%20Level-Actively%20Developed-brightgreen.svg)](https://gist.github.com/cheerfulstoic/d107229326a01ff0f333a1d3476e068d)

<p><img src="doc/figs/arcana_logo.jpg" align="right" width="350" height="120"/></p>

- [Description](#description)
- [Mascot](#mascot)
- [Version](#version)
- [Status](#status)
- [Prerequisites](#prerequisites)
- [Building and Installing](#building-and-installing)
- [Testing](#testing)
- [Uninstalling](#uninstalling)
- [Repository structure](#repository-structure)
- [Projects built upon Gino](#Projects-built-upon-Gino)
- [Contributing](#contributing)
- [License](#license)

## Description
GINO is an automatic parallelizing compiler that works at the LLVM IR level.
It relies on the abstractions provided by [NOELLE](https://github.com/arcana-lab/noelle) (e.g. PDG, SCCDAG, ecc), [VIRGIL](https://github.com/arcana-lab/virgil), and [LLVM](http://llvm.org).
GINO supports DOALL and provides an implementation of [DSWP](https://liberty.princeton.edu/Publications/micro05_dswp.pdf) and [HELIX](https://users.cs.northwestern.edu/~simonec/files/Research/papers/HELIX_CGO_2012.pdf).

The following material composes the **documentation** available at the moment:
- An introductory [video](https://www.youtube.com/watch?v=whORNUUWIjI)
- The [CGO 2022 Paper](http://www.cs.northwestern.edu/~simonec/files/Research/papers/HELIX_CGO_2022.pdf)
- The slides used during [Advanced Topics in Compilers](http://www.cs.northwestern.edu/~simonec/ATC.html) at Northwestern

[We](https://users.cs.northwestern.edu/~simonec/Team.html) released GINO's source code in the hope of enriching the resources available to the research community and compiler developers.
You are kindly asked to acknowledge usage of the tool by citing the following paper:
```
@inproceedings{NOELLE,
    title={{NOELLE} {O}ffers {E}mpowering {LL}VM {E}xtensions},
    author={Angelo Matni and Enrico Armenio Deiana and Yian Su and Lukas Gross and Souradip Ghosh and Sotiris Apostolakis and Ziyang Xu and Zujun Tan and Ishita Chaturvedi and David I. August and Simone Campanoni},
    booktitle={International Symposium on Code Generation and Optimization, 2022. CGO 2022.},
    year={2022}
}
```

## Mascot
Gino is the name of the cat Simone and his wife adopted from [Tree House Cat Shelter](https://treehouseanimals.org) when they moved to Chicago.
It was surprisingly hard to find a picture of Gino by himself. 
He's always playing and cuddling with his brother Gigi.

<div align="center">
    <img src="doc/figs/gino.jpg" align="center" width="45%" height="auto"/>
    <img src="doc/figs/gino_gigi.jpg" align="center" width="45%" height="auto"/>
</div>
<br>
A future project may or may not be called Gigi...

## Version
The latest stable version is 14.1.0 (tag = `v14.1.0`).

### Version Numbering Scheme
The version number is in the form of \[v _Major.Minor.Revision_ \]
- **Major**: Each major version matches a specific LLVM version (e.g., version 9 matches LLVM 9, version 11 matches LLVM 11)
- **Minor**: Starts from 0, each minor version represents either one or more API replacements/removals that might impact the users OR a forced update every six months (the minimum minor update frequency)
- **Revision**: Starts from 0; each revision version may include bug fixes or incremental improvements

#### Update Frequency
- **Major**: Matches the LLVM releases on a best-effort basis
- **Minor**: At least once per six months, at most once per month (1/month ~ 2/year)
- **Revision**: At least once per month, at most twice per week (2/week ~ 1/month)

## Status
Next is the status of Gino for different LLVM versions.

| LLVM    | Gino's branch   | NOELLE's branch | Regression tests failed out of 31087 tests | Performance tests failed out of 23 tests    | Latest version | Maintained         |
| ------: | --------------: | --------------: | -----------------------------------------: | ------------------------------------------: | -------------: | :----------------: |
|  14.0.6 | master          | master          |                                        614 |                                           0 |         14.1.0 | :white_check_mark: |
|   9.0.0 | v9              | v9              |                                        797 |                                           0 |          9.3.0 |                :x: |


## Prerequisites
- LLVM 14.0.6
- NOELLE 14.1.0

### Northwestern users
Those who have access to the Zythos cluster at Northwestern can source LLVM 14.0.6 from any node of the cluster with:
```
source /project/extra/llvm/14.0.6/enable
```
and Noelle with:
```
source /project/extra/noelle/9.14.1/enable
```
Check out the Zythos cluster guide [here](http://www.cs.northwestern.edu/~simonec/files/Research/manuals/Zythos_guide.pdf) for more.

## Building and Installing

To build and install GINO you need to configure it first, unless the [default configuration](config.default.cmake) is satisfactory.
From the root directory:
```
source /path/to/noelle/enable   # make NOELLE available
make menuconfig                 # to customize the installation
make                            # set the number of jobs with JOBS=8 (default is 16)
```

## Testing
To run all tests in parallel using [Condor](https://htcondor.org/), invoke the following commands:
```
cd tests
make clean
make condor
```
To monitor test progress use `cd tests ; make condor_watch`.

To monitor test failures use `cd tests ; make condor_check`.

`make condor` creates a `regression_X` sub-directory per configuration (e.g., `regression_42`).
Each single test within a given `regression_X` sub-directory will contain a `run_me.sh` script, which is automatically generated by `make condor`
and can be used to re-produce the compilation for that specific configuration.

```
cd tests/regression_42
cd test_of_your_interest
./run_me.sh
```

To run only the autotuned performance tests using Condor, invoke the following commands:
```
cd tests
make clean
make condor_autotuner
```
The speedup results will be collected in `tests/performance/speedups_autotuner.txt`.

## Uninstalling

In this order:

Run `make uninstall` to uninstall without cleaning the build files.

Run `make clean` to reset the repository to its initial state.
For generality, the install directory is not removed.

## Repository structure

- `bin` contains the scripts through which the user will run all analyses and transformations
- `doc` contains the documentation
- `src` contains the C++ source of the framework
- `src/core` contains all the main abstractions
- `src/tools` contains a set of tools built on top of the core. All tools are independent from one another
- `tests` contains regression and performance tests


## Projects built upon Gino

Several projects have already been built successfully upon Gino.
These projects are (in no particular order):
- [HBC](https://github.com/arcana-lab/heartbeatcompiler)
- [SPLENDID](https://dl.acm.org/doi/10.1145/3582016.3582058)
- [CCK](https://github.com/arcana-lab/cck)


## Contributing
We welcome contributions from the community to improve this framework and evolve it to cater for more users.

GINO uses `clang-format` to ensure uniform styling across the project's source code.
To format all `.cpp` and `.hpp` files in the repository run `make format` from the root.
`clang-format` is run automatically as a pre-commit git hook, meaning that when you commit a file `clang-format` is automatically run on the file in-place.

Since git doesn't allow for git hooks to be installed when you clone the repository,
cmake will install the pre-commit git hook upon installation.


## Contributions
We welcome contributions from the community to improve this framework and evolve it to cater for more users.
If you have any trouble using this framework feel free to reach out to us for help (contact simone.campanoni@northwestern.edu).

## License

Gino is licensed under the [MIT License](./LICENSE.md).

