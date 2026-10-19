//===--- ModulesActivity.h - What the module support is busy with -*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// The module support does work that the user waits for: the units of a project
// are scanned for their imports, and the module files (BMIs) that a file needs
// are built. ModulesActivity counts that work and tells a listener (the LSP
// server, which shows it as the progress of the language server) about it.
//
// Only work that takes time is shown: a batch of items whose results were all
// cached (nothing was scanned, every module file was up to date) never reaches
// the listener, so typing in a project does not make a progress bar flash.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANGD_MODULESACTIVITY_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANGD_MODULESACTIVITY_H

#include "llvm/ADT/StringRef.h"
#include <functional>
#include <mutex>
#include <string>

namespace clang {
namespace clangd {

class ModulesActivity {
public:
  enum class Phase {
    /// Reading the imports and the declared module of the source files.
    Scanning = 0,
    /// Building module files.
    Building = 1,
  };
  static constexpr unsigned NumPhases = 2;

  struct State {
    /// The items of all the batches that have not been completed yet, and the
    /// ones that have, since the phase was idle.
    unsigned Enqueued = 0;
    unsigned Completed = 0;
    /// What was started last.
    std::string Current;
    /// Whether anything that takes time was started: only then is the state
    /// reported.
    bool Shown = false;
    bool idle() const { return Completed >= Enqueued; }
  };

  /// Called with the state of a phase whenever it changes (and is shown).
  /// The phase is finished when State::idle(), the call is the last one of it.
  using Listener = std::function<void(Phase, const State &)>;

  void setListener(Listener L) {
    std::lock_guard<std::mutex> Lock(Mu);
    this->L = std::move(L);
  }

  /// N items of work of a phase, of which the first that takes time is shown.
  /// Complete every item with done(), or let the destructor complete the
  /// remaining ones. A null \p Activity does nothing.
  class Batch {
  public:
    Batch(ModulesActivity *Activity, Phase P, unsigned N)
        : Activity(Activity), P(P), Remaining(N) {
      if (Activity && N)
        Activity->add(P, N);
    }
    Batch(const Batch &) = delete;
    Batch &operator=(const Batch &) = delete;
    ~Batch() {
      if (Activity && Remaining)
        Activity->finish(P, Remaining);
    }

    /// The next item takes time: \p What is what it works on.
    void begin(llvm::StringRef What) {
      if (Activity)
        Activity->begin(P, What);
    }
    /// The item is complete (also the ones that took no time).
    void done() {
      if (Activity && Remaining) {
        --Remaining;
        Activity->finish(P, 1);
      }
    }

  private:
    ModulesActivity *Activity;
    Phase P;
    unsigned Remaining;
  };

private:
  void add(Phase P, unsigned N) {
    std::lock_guard<std::mutex> Lock(Mu);
    Phases[static_cast<unsigned>(P)].Enqueued += N;
  }

  void begin(Phase P, llvm::StringRef What) {
    std::lock_guard<std::mutex> Lock(Mu);
    State &S = Phases[static_cast<unsigned>(P)];
    S.Current = What.str();
    S.Shown = true;
    notify(P, S);
  }

  void finish(Phase P, unsigned N) {
    std::lock_guard<std::mutex> Lock(Mu);
    State &S = Phases[static_cast<unsigned>(P)];
    S.Completed += N;
    if (S.Shown)
      notify(P, S);
    if (S.idle())
      S = State();
  }

  void notify(Phase P, const State &S) {
    if (L)
      L(P, S);
  }

  std::mutex Mu;
  State Phases[NumPhases];
  Listener L;
};

} // namespace clangd
} // namespace clang

#endif
