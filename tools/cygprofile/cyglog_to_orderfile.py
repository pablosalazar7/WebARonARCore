#!/usr/bin/python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Symbolizes a log file produced by cyprofile instrumentation.

Given a log file and the binary being profiled, creates an orderfile.
"""

import logging
import multiprocessing
import optparse
import os
import re
import string
import sys
import tempfile

import cygprofile_utils
import symbol_extractor


def _ParseLogLines(log_file_lines):
  """Parses a merged cyglog produced by mergetraces.py.

  Args:
    log_file_lines: array of lines in log file produced by profiled run
    lib_name: library or executable containing symbols

    Below is an example of a small log file:
    5086e000-52e92000 r-xp 00000000 b3:02 51276      libchromeview.so
    secs       usecs      pid:threadid    func
    START
    1314897086 795828     3587:1074648168 0x509e105c
    1314897086 795874     3587:1074648168 0x509e0eb4
    1314897086 796326     3587:1074648168 0x509e0e3c
    1314897086 796552     3587:1074648168 0x509e07bc
    END

  Returns:
    An ordered list of callee offsets.
  """
  call_lines = []
  vm_start = 0
  line = log_file_lines[0]
  assert 'r-xp' in line
  end_index = line.find('-')
  vm_start = int(line[:end_index], 16)
  for line in log_file_lines[3:]:
    fields = line.split()
    if len(fields) == 4:
      call_lines.append(fields)
    else:
      assert fields[0] == 'END'
  # Convert strings to int in fields.
  call_info = []
  for call_line in call_lines:
    addr = int(call_line[3], 16)
    if vm_start < addr:
      addr -= vm_start
      call_info.append(addr)
  return call_info


def _GroupLibrarySymbolInfosByOffset(lib_filename):
  """Returns a dict {offset: [SymbolInfo]} from a library."""
  symbol_infos = symbol_extractor.SymbolInfosFromBinary(lib_filename)
  return symbol_extractor.GroupSymbolInfosByOffset(symbol_infos)


class SymbolNotFoundException(Exception):
  def __init__(self, value):
    super(SymbolNotFoundException, self).__init__(value)
    self.value = value

  def __str__(self):
    return repr(self.value)


def _FindSymbolInfosAtOffset(offset_to_symbol_infos, offset):
  """Finds all SymbolInfo at a given offset.

  Args:
    offset_to_symbol_infos: {offset: [SymbolInfo]}
    offset: offset to look the symbols at

  Returns:
    The list of SymbolInfo at the given offset

  Raises:
    SymbolNotFoundException if the offset doesn't match any symbol.
  """
  if offset in offset_to_symbol_infos:
    return offset_to_symbol_infos[offset]
  elif offset % 2 and (offset - 1) in offset_to_symbol_infos:
    # On ARM, odd addresses are used to signal thumb instruction. They are
    # generated by setting the LSB to 1 (see
    # http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0471e/Babfjhia.html).
    # TODO(lizeb): Make sure this hack doesn't propagate to other archs.
    return offset_to_symbol_infos[offset - 1]
  else:
    raise SymbolNotFoundException(offset)


def _GetObjectFileNames(obj_dir):
  """Returns the list of object files in a directory."""
  obj_files = []
  for (dirpath, _, filenames) in os.walk(obj_dir):
    for file_name in filenames:
      if file_name.endswith('.o'):
        obj_files.append(os.path.join(dirpath, file_name))
  return obj_files


def _AllSymbolInfos(object_filenames):
  """Returns a list of SymbolInfo from an iterable of filenames."""
  pool = multiprocessing.Pool()
  # Hopefully the object files are in the page cache at this step, so IO should
  # not be a problem (hence no concurrency limit on the pool).
  symbol_infos_nested = pool.map(
      symbol_extractor.SymbolInfosFromBinary, object_filenames)
  result = []
  for symbol_infos in symbol_infos_nested:
    result += symbol_infos
  return result


def GetSymbolToSectionsMapFromObjectFiles(obj_dir):
  """Creates a mapping from symbol to linker section names by scanning all
     the object files.
  """
  object_files = _GetObjectFileNames(obj_dir)
  symbol_to_sections_map = {}
  symbol_warnings = cygprofile_utils.WarningCollector(300)
  symbol_infos = _AllSymbolInfos(object_files)
  for symbol_info in symbol_infos:
    symbol = symbol_info.name
    if symbol.startswith('.LTHUNK'):
      continue
    section = symbol_info.section
    if ((symbol in symbol_to_sections_map) and
        (not symbol_info.section in symbol_to_sections_map[symbol])):
      symbol_to_sections_map[symbol].append(section)
      # Check if this is the understood case of constructor/destructor
      # signatures. G++ emits up to three types of constructor/destructors:
      # complete, base, and allocating.  If they're all the same they'll
      # get folded together.
      if not (re.match('[CD][123]E', symbol) and
          (symbol_extractor.Demangle(symbol) ==
           symbol_extractor.Demangle(symbol_to_sections_map[symbol][0]
                                     .lstrip('.text.')))):
        symbol_warnings.Write('Symbol ' + symbol +
                              ' in conflicting sections ' +
                              ', '.join(symbol_to_sections_map[symbol]))
    elif not section.startswith('.text'):
      symbol_warnings.Write('Symbol ' + symbol +
                            ' in incorrect section ' + section)
    else:
      symbol_to_sections_map[symbol] = [ section ]
  symbol_warnings.WriteEnd('bad sections')
  return symbol_to_sections_map


def _WarnAboutDuplicates(offsets):
  """Warns about duplicate offsets.

  Args:
    offsets: list of offsets to check for duplicates

  Returns:
    True if there are no duplicates, False otherwise.
  """
  seen_offsets = set()
  ok = True
  for offset in offsets:
    if offset not in seen_offsets:
      seen_offsets.add(offset)
    else:
      ok = False
      logging.warning('Duplicate offset: ' + hex(offset))
  return ok


def _OutputOrderfile(offsets, offset_to_symbol_infos, symbol_to_sections_map,
                     output_file):
  """Outputs the orderfile to output_file.

  Args:
    offsets: Iterable of offsets to match to section names
    offset_to_symbol_infos: {offset: [SymbolInfo]}
    symbol_to_sections_map: {name: [section1, section2]}
    output_file: file-like object to write the results to
  """
  success = True
  unknown_symbol_warnings = cygprofile_utils.WarningCollector(300)
  symbol_not_found_warnings = cygprofile_utils.WarningCollector(300)
  output_sections = set()
  for offset in offsets:
    try:
      symbol_infos = _FindSymbolInfosAtOffset(offset_to_symbol_infos, offset)
      for symbol_info in symbol_infos:
        if symbol_info.name in symbol_to_sections_map:
          sections = symbol_to_sections_map[symbol_info.name]
          for section in sections:
            if not section in output_sections:
              output_file.write(section + '\n')
              output_sections.add(section)
        else:
          unknown_symbol_warnings.Write(
              'No known section for symbol ' + symbol_info.name)
    except SymbolNotFoundException:
      symbol_not_found_warnings.Write(
          'Did not find function in binary. offset: ' + hex(offset))
      success = False
  unknown_symbol_warnings.WriteEnd('no known section for symbol.')
  symbol_not_found_warnings.WriteEnd('symbol not found in the binary.')
  return success


def main():
  parser = optparse.OptionParser(usage=
      'usage: %prog [options] <merged_cyglog> <library> <output_filename>')
  parser.add_option('--target-arch', action='store', dest='arch',
                    choices=['arm', 'arm64', 'x86', 'x86_64', 'x64', 'mips'],
                    help='The target architecture for libchrome.so')
  options, argv = parser.parse_args(sys.argv)
  if not options.arch:
    options.arch = cygprofile_utils.DetectArchitecture()
  if len(argv) != 4:
    parser.print_help()
    return 1
  (log_filename, lib_filename, output_filename) = argv[1:]
  symbol_extractor.SetArchitecture(options.arch)

  obj_dir = cygprofile_utils.GetObjDir(lib_filename)

  log_file_lines = map(string.rstrip, open(log_filename).readlines())
  offsets = _ParseLogLines(log_file_lines)
  _WarnAboutDuplicates(offsets)

  offset_to_symbol_infos = _GroupLibrarySymbolInfosByOffset(lib_filename)
  symbol_to_sections_map = GetSymbolToSectionsMapFromObjectFiles(obj_dir)

  success = False
  temp_filename = None
  output_file = None
  try:
    (fd, temp_filename) = tempfile.mkstemp(dir=os.path.dirname(output_filename))
    output_file = os.fdopen(fd, 'w')
    ok = _OutputOrderfile(
        offsets, offset_to_symbol_infos, symbol_to_sections_map, output_file)
    output_file.close()
    os.rename(temp_filename, output_filename)
    temp_filename = None
    success = ok
  finally:
    if output_file:
      output_file.close()
    if temp_filename:
      os.remove(temp_filename)

  return 0 if success else 1


if __name__ == '__main__':
  logging.basicConfig(level=logging.INFO)
  sys.exit(main())
