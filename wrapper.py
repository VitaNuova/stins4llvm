import argparse
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('csource', help = 'C source file to protect. Ends with .c.')
parser.add_argument('connectivity_level', type = int, help = 'Integer specifying protection network connectivity level.')
parser.add_argument('sens_funcs', nargs='+', help = 'Names of sensitive functions. Stringsseparated with whitespace.')

args = parser.parse_args()
if args.csource.endswith('.c') is False:
  parser.print_usage()
  parser.print_help()
  parser.exit('Error: C source file should end with .c.')
print "Source file passed: " + args.csource
print "Connectivity level passed: " + str(args.connectivity_level)
print "Sensitive functions passed: " + str(args.sens_funcs)

for func in args.sens_funcs:
   klee_output = open('klee_output', 'w')
   p = subprocess.Popen(["python3", "/home/sip/klee/syminputC.py", func, args.csource], stdout = subprocess.PIPE)
   out, err = p.communicate()
   print out
