/*
 * Copyright 2016 - 2024  Angelo Matni, Sophia Boksenbaum, Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 of the Software, and to permit persons to whom the Software is furnished to do
 so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "arcana/gino/core/HELIXTask.hpp"

namespace arcana::gino {

HELIXTask::HELIXTask(FunctionType *taskSignature, Module &M)
  : Task{ taskSignature, M } {

  /*
   * Fetch the arguments of the function that implements the task.
   */
  auto argIter = this->F->arg_begin();
  this->envArg = (Value *)&*(argIter++);
  this->loopCarriedArrayArg = (Value *)&*(argIter++);
  this->ssPastArrayArg = (Value *)&*(argIter++);
  this->ssFutureArrayArg = (Value *)&*(argIter++);
  this->scylaxDataArg = (Value *)&*(argIter++);
  this->coreArg = (Value *)&*(argIter++);
  this->numCoresArg = (Value *)&*(argIter++);
  this->loopIsOverFlagArg = (Value *)&*(argIter++);

  /*
   * Set the task index.
   */
  this->instanceIndexV = coreArg;

  this->envArg->setName("env");
  this->loopCarriedArrayArg->setName("loopCarriedArray");
  this->ssPastArrayArg->setName("ssPastArray");
  this->ssFutureArrayArg->setName("ssFutureArray");
  this->scylaxDataArg->setName("scylaxData");
  this->coreArg->setName("coreID");
  this->numCoresArg->setName("numCores");
  this->loopIsOverFlagArg->setName("loopIsOverFlag");

  return;
}

} // namespace arcana::gino
