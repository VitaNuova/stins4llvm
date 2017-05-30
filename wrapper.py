import argparse

parser = argparse.ArgumentParser()
parser.add_argument('csource', help = 'C source file to protect. Ends with .c.')
parser.add_argument('connectivity_level', type = int, help = 'Integer specifying protection network connectivity level.')
parser.add_argument('sens_funcs', nargs='+', help = 'Names of sensitive functions. Stringsseparated with whitespace.')

args = parser.parse_args()
if args.csource.endswith('.c') is False:
  parser.print_usage()
  parser.print_help()
  parser.exit('Error: C source file should end with .c.')
print args.csource
print args.connectivity_level
print args.sens_funcs

