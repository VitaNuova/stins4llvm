import argparse
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('csource', help = 'C source file to protect. Ends with .c.')
parser.add_argument('connectivity_level', type = int, help = 'Integer specifying protection network connectivity level.')
parser.add_argument('sens_funcs', nargs='+', help = 'Names of sensitive functions. Strings separated with whitespace.')

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
   lines = out.split('\n')
   numtests = 0
   pairs = []
   next_pair = []
   counter = 0
   name = ''
   for line in lines:
      if line.startswith('num objects:'):
         numtests = numtests + 1
      name_idx = line.find('name:')
      if name_idx != -1:
         splitted = line.split(':')
	 name = splitted[2].strip("' ")
      data_idx = line.find('data:')
      if data_idx != -1:
         splitted = line.split(':')
         if name.startswith(('macke', 'model')) is False:
            next_pair.append(splitted[2].strip())
         if name == 'macke_result':
            next_pair.append(splitted[2].strip())
            pairs.append(next_pair)
            next_pair = []
            counter = 0
   print numtests
   print pairs      
