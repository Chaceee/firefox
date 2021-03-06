// -*- mode: c++ -*-

// Copyright (c) 2010, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Original author: Jim Blandy <jimb@mozilla.com> <jimb@red-bean.com>

// dwarf_cfi_to_module.h: Define the DwarfCFIToModule class, which
// accepts parsed DWARF call frame info and adds it to a
// google_breakpad::Module object, which can write that information to
// a Breakpad symbol file.

#ifndef COMMON_LINUX_DWARF_CFI_TO_MODULE_H
#define COMMON_LINUX_DWARF_CFI_TO_MODULE_H

#include <cassert>
#include <string>
#include <vector>

#include "common/linux/module.h"
#include "common/dwarf/dwarf2reader.h"

namespace google_breakpad {

using dwarf2reader::CallFrameInfo;
using google_breakpad::Module;
using std::string;
using std::vector;

// A class that accepts parsed call frame information from the DWARF
// CFI parser and populates a google_breakpad::Module object with the
// contents.
class DwarfCFIToModule: public CallFrameInfo::Handler {
 public:

  // DwarfCFIToModule uses an instance of this class to report errors
  // detected while converting DWARF CFI to Breakpad STACK CFI records.
  class Reporter {
   public:
    // Create a reporter that writes messages to the standard error
    // stream. FILE is the name of the file we're processing, and
    // SECTION is the name of the section within that file that we're
    // looking at (.debug_frame, .eh_frame, etc.).
    Reporter(const string &file, const string &section)
      : file_(file), section_(section) { }
    virtual ~Reporter() { }

    // The DWARF CFI entry at OFFSET cites register REG, but REG is not
    // covered by the vector of register names passed to the
    // DwarfCFIToModule constructor, nor does it match the return
    // address column number for this entry.
    virtual void UnnamedRegister(size_t offset, int reg);

    // The DWARF CFI entry at OFFSET says that REG is undefined, but the
    // Breakpad symbol file format cannot express this.
    virtual void UndefinedNotSupported(size_t offset, const string &reg);

    // The DWARF CFI entry at OFFSET says that REG uses a DWARF
    // expression to find its value, but DwarfCFIToModule is not
    // capable of translating DWARF expressions to Breakpad postfix
    // expressions.
    virtual void ExpressionsNotSupported(size_t offset, const string &reg);

  protected:
    string file_, section_;
  };

  // Create a handler for the dwarf2reader::CallFrameInfo parser that
  // records the stack unwinding information it receives in MODULE.
  //
  // Use REGISTER_NAMES[I] as the name of register number I; *this
  // keeps a reference to the vector, so the vector should remain
  // alive for as long as the DwarfCFIToModule does.
  //
  // Use REPORTER for reporting problems encountered in the conversion
  // process.
  DwarfCFIToModule(Module *module, const vector<string> &register_names,
                   Reporter *reporter)
      : module_(module), register_names_(register_names), reporter_(reporter),
        entry_(NULL), return_address_(-1) { }
  virtual ~DwarfCFIToModule() { delete entry_; }

  virtual bool Entry(size_t offset, uint64 address, uint64 length,
                     uint8 version, const string &augmentation,
                     unsigned return_address);
  virtual bool UndefinedRule(uint64 address, int reg);
  virtual bool SameValueRule(uint64 address, int reg);
  virtual bool OffsetRule(uint64 address, int reg,
                          int base_register, long offset);
  virtual bool ValOffsetRule(uint64 address, int reg,
                             int base_register, long offset);
  virtual bool RegisterRule(uint64 address, int reg, int base_register);
  virtual bool ExpressionRule(uint64 address, int reg,
                              const string &expression);
  virtual bool ValExpressionRule(uint64 address, int reg,
                                 const string &expression);
  virtual bool End();

 private:
  // Return the name to use for register REG.
  string RegisterName(int i);

  // Record RULE for register REG at ADDRESS.
  void Record(Module::Address address, int reg, const string &rule);

  // The module to which we should add entries.
  Module *module_;

  // Map from register numbers to register names.
  const vector<string> &register_names_;

  // The reporter to use to report problems.
  Reporter *reporter_;

  // The current entry we're constructing.
  Module::StackFrameEntry *entry_;

  // The section offset of the current frame description entry, for
  // use in error messages.
  size_t entry_offset_;

  // The return address column for that entry.
  unsigned return_address_;
};

} // namespace google_breakpad

#endif // COMMON_LINUX_DWARF_CFI_TO_MODULE_H
