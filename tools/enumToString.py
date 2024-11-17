import os
import re
import argparse
import clang.cindex

libclang_path = os.getenv('LIBCLANG_PATH')
if libclang_path:
  clang.cindex.Config.set_library_file(libclang_path)

def dump_translation_unit(translation_unit):
  def dump_cursor(cursor, indent=0):
    print(' ' * indent + f'Cursor: {cursor.spelling}, Kind: {cursor.kind}')
    for child in cursor.get_children():
      dump_cursor(child, indent + 2)
  dump_cursor(translation_unit.cursor)

def generate_to_string_function(header_file, output_file, output_header, parse_args):
  index = clang.cindex.Index.create()
  args = ['-x', 'c++', '-fparse-all-comments'] + parse_args
  translation_unit = index.parse(header_file, args=args)
  for diagnostic in translation_unit.diagnostics:
    print(f"Diagnostic: {diagnostic.spelling}")
  # dump_translation_unit(translation_unit)

  str_pattern = re.compile(r'STR:"([^"]+)"')

  def process_cursor(cursor, out_file, out_header, namespace_stack, original_file):
    if cursor.location.file and cursor.location.file.name != original_file:
      return
    if cursor.kind == clang.cindex.CursorKind.NAMESPACE:
      namespace_stack.append(cursor.spelling)
      out_file.write(f'namespace {cursor.spelling} {{\n')
      out_header.write(f'namespace {cursor.spelling} {{\n')
      for child in cursor.get_children():
        process_cursor(child, out_file, out_header, namespace_stack, original_file)
      out_file.write('}\n')
      out_header.write('}\n')
      namespace_stack.pop()
    elif cursor.kind == clang.cindex.CursorKind.ENUM_DECL:
      enum_name = cursor.spelling
      out_header.write(f'const char *toString({enum_name} value);\n')
      out_file.write(f'const char *toString({enum_name} value) {{\n')
      out_file.write('    switch (value) {\n')

      for enum_value in cursor.get_children():
        if enum_value.kind == clang.cindex.CursorKind.ENUM_CONSTANT_DECL:
          comment = enum_value.raw_comment
          match = str_pattern.search(comment) if comment else None
          if match:
            comment = match.group(1)
          else:
            comment = enum_value.spelling
          out_file.write(f'        case {enum_name}::{enum_value.spelling}: return "{comment}";\n')

      out_file.write('        default: {\n')
      out_file.write('            static char buffer[32];\n')
      out_file.write('            ksnprintf(buffer, sizeof(buffer), "Unknown(%d)", static_cast<int>(value));\n')
      out_file.write('            return buffer;\n')
      out_file.write('        }\n')
      out_file.write('    }\n')
      out_file.write('}\n\n')

  with open(output_file, 'w') as out_file, open(output_header, 'w') as out_header:
    out_file.write('#include <cstdio>\n')
    out_file.write(f'#include "{header_file}"\n\n')
    out_header.write(f'#include "{header_file}"\n\n')
    namespace_stack = []
    for cursor in translation_unit.cursor.get_children():
      process_cursor(cursor, out_file, out_header, namespace_stack, header_file)

if __name__ == '__main__':
  parser = argparse.ArgumentParser(description='Generate toString function for enums.')
  parser.add_argument('header_file', help='The header file containing the enum definitions.')
  parser.add_argument('output_file', help='The output file to write the toString functions.')
  parser.add_argument('output_header', help='The output file to write the toString functions.')
  parser.add_argument('-I', '--include', action='append', dest='include_dirs', default=[], help='Include directories')
  parser.add_argument('-std', '--std', default='c++11', help='C++ standard')

  cliargs = parser.parse_args()
  parseArgs = [f'-I{incdir}' for incdir in cliargs.include_dirs] + [f'-std={cliargs.std}']
  generate_to_string_function(cliargs.header_file, cliargs.output_file, cliargs.output_header, parseArgs)
